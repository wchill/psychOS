#include <kernel/syscall.h>
#include <arch/x86/task.h>
#include <lib/lib.h>

int32_t syscall_open(uint32_t esp, const uint8_t *filename) {
	return -1;
}

int32_t syscall_read(uint32_t esp, int32_t fd, void *buf, int32_t nbytes) {
	return -1;
}

int32_t syscall_write(uint32_t esp, int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}

int32_t syscall_close(uint32_t esp, int32_t fd) {
	return -1;
}

int32_t syscall_execute(uint32_t esp, const uint8_t *command) {
	static uint32_t pid = 0;

	pcb_t *parent_pcb = get_task_pcb(esp);
	pcb_t *child_pcb = NULL;

	// Find a free PCB slot
	int i;
	for(i = 0; i < MAX_PROCESSES; i++) {
		pcb_t *some_pcb = get_pcb_slot(i);
		if(!some_pcb->in_use) {
			child_pcb = some_pcb;
			break;
		}
	}

	// No free PCB slots available
	if(child_pcb == NULL) return -1;

	// Save args in PCB
	parse_args(command, child_pcb->args);

	// TODO: finish

	// Load executable

	// Get entrypoint
	//get_executable_entrypoint()

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