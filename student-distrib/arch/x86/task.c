#include <arch/x86/x86_desc.h>
#include <arch/x86/task.h>

void kernel_run_first_program(const int8_t* command) {
	// Clear kernel PCBs
	int i;
	for(i = 0; i < MAX_PROCESSES; i++) {
		pcb_t *current_pcb = get_pcb_slot(i);
		current_pcb->in_use = 0;
	}

	// Initialize terminal for programs to use
	terminal_open("stdin");
}

void set_kernel_stack(const void *stack) {
    tss.esp0 = (uint32_t) stack;
    ltr(KERNEL_TSS);
}

uint32_t get_executable_entrypoint(const void *executable) {
	const uint8_t *ptr = (const uint8_t*) executable;

	// Check for ELF header
	if(!strncmp(ptr, ELF_MAGIC_HEADER, 4)) {
		return NULL;
	}

	// Read entry point address;
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
	return buf;
}

uint32_t load_program_into_slot(const int8_t *filename, uint32_t pcb_slot) {
	void *process_page = get_process_page_from_slot(pcb_slot);
	read_file_by_name(filename, process_page + PROCESS_LINK_OFFSET, PROCESS_PAGE_SIZE - PROCESS_LINK_OFFSET);

	return get_executable_entrypoint(process_page + PROCESS_LINK_OFFSET);
}

inline pcb_t *get_pcb_from_esp(void *process_esp) {
	return (pcb_t *) ((uint32_t) process_esp & PCB_BITMASK);
}

inline pcb_t *get_pcb_from_slot(uint32_t pcb_slot) {
	return (pcb_t*) ((KERNEL_PAGE_END - (KERNEL_STACK_SIZE * (pcb_slot + 1))) & PCB_BITMASK);
}

inline void *get_current_kernel_stack_base() {
	// Trick: Allocate variable on stack, then take its address
	// and use bitmask to get the top of the current stack
	// Then we can add 8kB to the value to get the bottom of the stack
	int stack_var = 0;
	return (void*) (((uint32_t) &stack_var & PCB_BITMASK) + KERNEL_STACK_SIZE);
}

inline void *get_process_page_from_slot(uint32_t task_slot) {
	return (void*) (KERNEL_PAGE_END + (PROCESS_PAGE_SIZE * task_slot));
}