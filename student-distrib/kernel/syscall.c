#include <kernel/syscall.h>
#include <arch/x86/task.h>
#include <fs/fs.h>
#include <fs/ece391_fs.h>
#include <lib/lib.h>
#include <drivers/rtc.h>

/* IMPORTANT
 * 
 * descrepencies between function arguments will likely give a runtime error
 *
 * for example, rtc_write as of now takes one argument htz but is called
 * by convention with 3 arguments. every syscall should probably take the same
 * arguments even if they are not used (like for the rtc)
 */

#define KEYBOARD_FT 0xAAAAAAAA // TODO value of file type
#define FILE_FT 2
#define RTC_FT 0
#define TERMINAL_FT 0xDDDDDDDD // TODO value of file type
#define DIRECTORY_FT 1

int32_t syscall_open(const uint8_t *filename) {
	
	// get the process control block
	pcb_t *PCB = get_current_pcb();

	// find the lowest index in the file array that is free
	int32_t fd = -1, idx;
	for(idx = 0 ; idx < MAX_FILE_DESCRIPTORS ; idx++) {
		if(PCB->fa[idx].flags == 0) {
			fd = idx;
			break;
		}
	}

	// if file array is full
	if(fd == -1) return -1;

	// find file in file system
	dentry_t dentry;
	int32_t res = read_dentry_by_name(filename, &dentry);

	// DNE
	if(res < 0) return -1;

	// fill in table pointer in file array at the index of the file descriptor
	switch(dentry.file_type) {
		case FILE_FT:
			PCB->fa[fd].fops.open  = file_open;
			PCB->fa[fd].fops.close = file_close;
			PCB->fa[fd].fops.read  = file_read;
			PCB->fa[fd].fops.write = file_write;
			break;
		case RTC_FT:
			PCB->fa[fd].fops.open  = rtc_open;
			PCB->fa[fd].fops.close = rtc_close;
			PCB->fa[fd].fops.read  = rtc_read;
			PCB->fa[fd].fops.write = rtc_write;
			break;
		case DIRECTORY_FT:
			PCB->fa[fd].fops.open  = dir_open;
			PCB->fa[fd].fops.close = dir_close;
			PCB->fa[fd].fops.read  = dir_read;
			PCB->fa[fd].fops.write = dir_write;
			break;
		default:
			return -1;
	}

	// copy the inode over to the file array at the index of the file descriptor
	PCB->fa[fd].inode = dentry.inode_num;

	// set file position to 0
	PCB->fa[fd].file_position = 0;

	// mark this file descriptor as taken
	PCB->fa[fd].flags = 1;

	// call the close function
	PCB->fa[fd].fops.open(filename);

	// return file descriptor
	return fd;
}

int32_t syscall_close(int32_t fd) {

	// get the process control block
	pcb_t *PCB = get_current_pcb();

	// clear the file descriptor
	PCB->fa[fd].flags = 0;

	// call the close function
	return PCB->fa[fd].fops.close(fd);

}

int32_t syscall_read(int32_t fd, void *buf, int32_t nbytes) {

	// get the process control block
	pcb_t *PCB = get_current_pcb();

	// increment the file position
	PCB->fa[fd].file_position += nbytes;

	// call the read function
	return PCB->fa[fd].fops.read(fd, buf, nbytes);

}

int32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes) {

	// get the process control block
	pcb_t *PCB = get_current_pcb();

	// increment the file position
	PCB->fa[fd].file_position += nbytes;

	// call the read function
	return PCB->fa[fd].fops.write(fd, buf, nbytes);

}

int32_t syscall_execute(const int8_t *command) {
	static uint32_t pid = 1;

	pcb_t *parent_pcb = get_current_pcb();
	pcb_t *child_pcb = NULL;

	// Find a free PCB slot
	int i;
	for(i = 0; i < MAX_PROCESSES; i++) {
		pcb_t *some_pcb = get_pcb_from_slot(i);
		if(!some_pcb->in_use) {
			child_pcb = some_pcb;
			child_pcb->slot_num = i;
			break;
		}
	}

	// No free PCB slots available
	if(child_pcb == NULL) return -1;

	// Load executable and check validity
	uint32_t entrypoint = load_program_into_slot(command, child_pcb->slot_num);
	if(entrypoint == NULL) return -1;

	// Set up PCB
	child_pcb->parent = parent_pcb;
	child_pcb->child = NULL;
	parent_pcb->child = child_pcb;
	child_pcb->in_use = 1;
	child_pcb->pid = pid++;
	open_stdin_and_stdout(child_pcb);

	// Save args in PCB
	parse_args(command, child_pcb->args);

	// Process paging
	setup_process_paging(child_pcb->process_pd, get_process_page_from_slot(child_pcb->slot_num));
	enable_paging(child_pcb->process_pd);

	// Prepare for context switch
	set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));
	asm volatile(
		"mov %%esp, %0\r\n"
		"mov %%ebp, %1\r\n"
		: "=g"(parent_pcb->regs.esp), "=g"(parent_pcb->regs.ebp)
		:
		: "memory"
	);

	// Switch to user mode
	switch_to_ring_3(PROCESS_LINK_START, entrypoint);

	// We'll never come back here
	return -1;
}

int32_t syscall_halt(uint8_t status) {
	pcb_t *child_pcb = get_current_pcb();
	pcb_t *parent_pcb = child_pcb->parent;

	child_pcb->in_use = 0;
	int i;
	for(i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
		if(child_pcb->fa[i].flags) {
			child_pcb->fa[i].fops.close(i);
			child_pcb->fa[i].flags = 0;
		}
	}

	if(parent_pcb == NULL) {
		void *current_kernel_stack_base = get_current_kernel_stack_base();
		const int8_t cmd[] = "shell\0";
		static pd_entry temp_pd[NUM_PD_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));

	    initialize_paging_structs(temp_pd);
	    enable_paging(temp_pd);

		asm volatile(
			"mov %0, %%esp\r\n"
			"push %1\r\n"
			"push $0\r\n"
			"jmp %2\r\n"
			:
			: "r"(current_kernel_stack_base), "r"(cmd), "r"(kernel_run_first_program)
			: "memory"
		);
	}

	enable_paging(parent_pcb->process_pd);
	set_kernel_stack(get_kernel_stack_base_from_slot(parent_pcb->slot_num));

	asm volatile(
		"mov %0, %%esp\r\n"
		"push %2\r\n"
		"mov %1, %%ebp\r\n"
		"pop %%eax\r\n"
		"leave\r\n"
		"ret\r\n"
		:
		: "r"(parent_pcb->regs.esp), "r"(parent_pcb->regs.ebp), "r"((uint32_t) status)
		: "memory", "cc");

	return -1;
}

int32_t syscall_getargs(uint8_t *buf, int32_t nbytes) {
	return -1;
}

int32_t syscall_vidmap(uint8_t **screen_start) {
	return -1;
}

int32_t syscall_set_handler(int32_t signum, void *handler_address) {
	return -1;
}

int32_t syscall_sigreturn() {
	return -1;
}