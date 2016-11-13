#include <kernel/syscall.h>
#include <arch/x86/task.h>
#include <lib/lib.h>

/* IMPORTANT
 * 
 * descrepencies between function arguments will likely give a runtime error
 *
 * for example, rtc_write as of now takes one argument htz but is called
 * by convention with 3 arguments. every syscall should probably take the same
 * arguments even if they are not used (like for the rtc)
 */

uint32_t KEYBOARD_FT 	= 0xAAAAAAAA; // TODO value of file type
uint32_t FILE_FT 		= 2;
uint32_t RTC_FT 		= 0;
uint32_t TERMINAL_FT 	= 0xDDDDDDDD; // TODO value of file type
uint32_t DIRECTORY_FT 	= 1;

int32_t syscall_open(uint32_t esp, const uint8_t *filename) {
	
	// find the lowest index in the file array that is free
	int32_t fd = -1, idx;
	for(idx = 0 ; idx < 8 ; idx++) {
		if(PCB->fa[idx]->flags == 0) {
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

	// get the program 
	pcb_t *PCB = get_pcb_from_esp((void*) esp);

	// fill in table pointer in file array at the index of the file descriptor
	switch(res->file_type) {
		case KEYBOARD_FT:
			PCB->fa[fd]->fops->open  = NULL; // TODO
			PCB->fa[fd]->fops->close = NULL;
			PCB->fa[fd]->fops->read  = NULL;
			PCB->fa[fd]->fops->write = NULL;
			break;
		case FILE_SYSTEM_FT:
			PCB->fa[fd]->fops->open  = file_open;
			PCB->fa[fd]->fops->close = file_close;
			PCB->fa[fd]->fops->read  = file_read;
			PCB->fa[fd]->fops->write = file_write;
			break;
		case RTC_FT:
			PCB->fa[fd]->fops->open  = rtc_open;
			PCB->fa[fd]->fops->close = rtc_close;
			PCB->fa[fd]->fops->read  = rtc_read;
			PCB->fa[fd]->fops->write = rtc_write;
			break;
		case TERMINAL_FT:
			PCB->fa[fd]->fops->open  = NULL; // TODO
			PCB->fa[fd]->fops->close = NULL;
			PCB->fa[fd]->fops->read  = NULL;
			PCB->fa[fd]->fops->write = NULL;
			break;
		case DIRECTORY_FT:
			PCB->fa[fd]->fops->open  = dir_open;
			PCB->fa[fd]->fops->close = dir_close;
			PCB->fa[fd]->fops->read  = dir_read;
			PCB->fa[fd]->fops->write = dir_write;
			break;
		default:
			return -1;
	}

	// copy the inode over to the file array at the index of the file descriptor
	PCB->fa[fd]->inode = res->inode_num;

	// copy the file position over to the file array at the index of the file descriptor
	PCB->fa[fd]->file_position = dentry;

	// mark this file descriptor as taken
	PCB->fa[fd]->flags = 1;

	// call the close function
	PCB->fa[fd]->fops->open(filename);

	// return file descriptor
	return fd;
}

int32_t syscall_close(uint32_t esp, int32_t fd) {

	// get the process control block
	pcb_t *PCB = get_pcb_from_esp((void*) esp);

	// clear the file descriptor
	PCB->fa[fd]->flags = 0;

	// call the close function
	return PCB->fa[fd]->fops->close(fd);

}

int32_t syscall_read(uint32_t esp, int32_t fd, void *buf, int32_t nbytes) {

	// get the process control block
	pcb_t *PCB = get_pcb_from_esp((void*) esp);

	// increment the file position
	PCB->fa[fd]->file_position += nbytes;

	// call the read function
	return PCB->fa[fd]->fops->read(fd, buf, nbytes);

}

int32_t syscall_write(uint32_t esp, int32_t fd, const void *buf, int32_t nbytes) {

	// get the process control block
	pcb_t *PCB = get_pcb_from_esp((void*) esp);

	// increment the file position
	PCB->fa[fd]->file_position += nbytes;

	// call the read function
	return PCB->fa[fd]->fops->write(fd, buf, nbytes);

}

int32_t syscall_execute(uint32_t esp, const int8_t *command, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, uint32_t eip, uint32_t cs, uint32_t eflags) {
	static uint32_t pid = 1;

	pcb_t *parent_pcb = get_pcb_from_esp((void*) esp);
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

	// Save args in PCB
	parse_args(command, child_pcb->args);

	// Process paging
	setup_process_paging(child_pcb->process_pd, get_process_page_from_slot(child_pcb->slot_num));
	enable_paging(child_pcb->process_pd);

	// Prepare for context switch
	set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));

	// Switch to user mode

	// Come back here after halt, return retval

	return -1;
}

int32_t syscall_halt(uint32_t esp, uint8_t status) {
	return -1;
}

int32_t syscall_getargs(uint32_t esp, uint8_t *buf, int32_t nbytes) {
	return -1;
}

int32_t syscall_vidmap(uint32_t esp, uint8_t **screen_start) {
	return -1;
}

int32_t syscall_set_handler(uint32_t esp, int32_t signum, void *handler_address) {
	return -1;
}

int32_t syscall_sigreturn(uint32_t esp) {
	return -1;
}