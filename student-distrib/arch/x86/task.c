#include <arch/x86/x86_desc.h>
#include <lib/lib.h>
#include <arch/x86/task.h>
#include <fs/fs.h>
#include <tty/terminal.h>

void open_stdin_and_stdout(pcb_t *pcb) {
	// stdin
	pcb->fa[0].flags = 1;
	pcb->fa[0].fops.open = NULL;
	pcb->fa[0].fops.close = terminal_close;
	pcb->fa[0].fops.read = terminal_read;
	pcb->fa[0].fops.write = NULL;
	pcb->fa[0].file_position = 0;

	// stdout
	pcb->fa[1].flags = 1;
	pcb->fa[1].fops.open = NULL;
	pcb->fa[1].fops.close = terminal_close;
	pcb->fa[1].fops.read = NULL;
	pcb->fa[1].fops.write = terminal_write;
	pcb->fa[1].file_position = 0;
}

void kernel_run_first_program(const int8_t* command) {
	// Clear kernel PCBs
	int i;
	for(i = 0; i < MAX_PROCESSES; i++) {
		pcb_t *current_pcb = get_pcb_from_slot(i);
		current_pcb->in_use = 0;
		current_pcb->slot_num = i;
	}

	pcb_t *child_pcb = get_pcb_from_slot(0);

	// Process paging
	setup_process_paging(child_pcb->process_pd, get_process_page_from_slot(child_pcb->slot_num));
	enable_paging(child_pcb->process_pd);

	// Initialize terminal for programs to use
	terminal_open("stdin");

	uint32_t entrypoint = load_program_into_slot(command, child_pcb->slot_num);
	if(entrypoint == NULL) return;

	// Set up PCB
	child_pcb->parent = NULL;
	child_pcb->child = NULL;
	child_pcb->in_use = 1;
	child_pcb->pid = 0;
	open_stdin_and_stdout(child_pcb);

	parse_args(command, child_pcb->args);

	// Prepare for context switch
	set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));

	open_stdin_and_stdout(child_pcb);

	switch_to_ring_3(PROCESS_LINK_START, entrypoint);
}

void set_kernel_stack(const void *stack) {
    tss.ss0 = KERNEL_DS;
    tss.esp0 = (uint32_t) stack;

    seg_desc_t the_tss_desc;
    the_tss_desc.granularity    = 0;
    the_tss_desc.opsize         = 0;
    the_tss_desc.reserved       = 0;
    the_tss_desc.avail          = 0;
    the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
    the_tss_desc.present        = 1;
    the_tss_desc.sys            = 0;
    the_tss_desc.type           = 0x9;

    // Make accessible from ring 3
    the_tss_desc.dpl            = 0x3;

    SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);

    tss_desc_ptr = the_tss_desc;

    ltr(KERNEL_TSS);
}

uint32_t get_executable_entrypoint(const void *executable) {
	const int8_t *ptr = (const int8_t*) executable;

	// Check for ELF header
	if(strncmp(ptr, ELF_MAGIC_HEADER, 4)) {
		return NULL;
	}

	// Read entry point address
	uint32_t addr = *((uint32_t*) &ptr[24]);

	return addr;
}

int32_t parse_args(const int8_t *command, int8_t *buf) {
	// Parse args
	int index = 0;
	int start = 0;
	int len = 0;
	int8_t ch;
	while((ch = command[index++]) != '\0') {
		// Find the first character after space
		if (ch == ' ') {
			while(ch == ' ') {
				ch = command[index++];
			}
			start = index;
			break;
		}
	}

	// If there were any args, then get their length and copy
	if(start > 0) {
		len = strlen(&command[start]);
		memcpy(buf, &command[start], len);
	}

	// Null terminate the string
	buf[len] = '\0';
	return len;
}

uint32_t load_program_into_slot(const int8_t *filename, uint32_t pcb_slot) {
	void *process_page = get_process_page_from_slot(pcb_slot);
	read_file_by_name(filename, process_page + PROCESS_LINK_OFFSET, PROCESS_PAGE_SIZE - PROCESS_LINK_OFFSET);

	return get_executable_entrypoint(process_page + PROCESS_LINK_OFFSET);
}

inline pcb_t *get_pcb_from_esp(void *process_esp) {
	return (pcb_t *) ((uint32_t) (process_esp - 1) & PCB_BITMASK);
}

inline pcb_t *get_pcb_from_slot(uint32_t pcb_slot) {
	return (pcb_t*) (KERNEL_PAGE_END - (KERNEL_STACK_SIZE * (pcb_slot + 1)));
}

inline pcb_t *get_current_pcb() {
	int stack_var = 0;
	return get_pcb_from_esp(&stack_var);
}

inline void *get_current_kernel_stack_base() {
	// Trick: Allocate variable on stack, then take its address
	// and use bitmask to get the top of the current stack
	// Then we can add 8kB to the value to get the bottom of the stack
	int stack_var = 0;
	return (void*) (((uint32_t) &stack_var & PCB_BITMASK) + KERNEL_STACK_SIZE);
}

inline void *get_kernel_stack_base_from_slot(uint32_t pcb_slot) {
	return (void*) (KERNEL_PAGE_END - (KERNEL_STACK_SIZE * (pcb_slot + 1)) + KERNEL_STACK_SIZE);
}

inline void *get_process_page_from_slot(uint32_t task_slot) {
	return (void*) (KERNEL_PAGE_END + (PROCESS_PAGE_SIZE * task_slot));
}