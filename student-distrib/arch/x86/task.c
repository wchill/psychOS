#include <arch/x86/x86_desc.h>
#include <arch/x86/task.h>

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

pcb_t *get_task_pcb(void *process_esp) {
	return (pcb_t *) ((uint32_t) process_esp & PCB_BITMASK);
}

inline void *get_current_kernel_stack_base() {
	// Trick: Allocate variable on stack, then take its address
	// and use bitmask to get the top of the current stack
	// Then we can add 8kB to the value to get the bottom of the stack
	int stack_var = 0;
	return (void*) (((uint32_t) &stack_var & PCB_BITMASK) + 8192);
}