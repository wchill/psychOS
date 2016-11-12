#include <kernel/syscall.h>
#include <arch/x86/task.h>
#include <lib/lib.h>

static int test_arr[32];

int32_t syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
	switch(eax) {
		case SYSCALL_READ:
			return syscall_read((int32_t) ebx, (void*) ecx, (int32_t) edx);
		case SYSCALL_WRITE:
			return syscall_write((int32_t) ebx, (void*) ecx, (int32_t) edx);
		case SYSCALL_OPEN:
			return syscall_open((uint8_t*) ebx);
		case SYSCALL_CLOSE:
			return syscall_close((int32_t) ebx);
		case SYSCALL_EXECUTE:
			return syscall_execute((uint8_t*) ebx);
		case SYSCALL_HALT:
			return syscall_halt((uint8_t) ebx);

		/* Currently unimplemented */
		case SYSCALL_GETARGS:
		case SYSCALL_VIDMAP:
		case SYSCALL_SET_HANDLER:
		case SYSCALL_SIGRETURN:
		default:
			return SYSCALL_EINVAL;
	}
}

int32_t syscall_open(const uint8_t *filename) {
	return -1;
}

int32_t syscall_read(int32_t fd, void *buf, int32_t nbytes) {
	return -1;
}

int32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}

int32_t syscall_close(int32_t fd) {
	return -1;
}

int32_t syscall_execute(const uint8_t *command) {
	/* Parse args */
	int index = 0;
	int start = 0;
	int len = 0;
	uint8_t ch;
	while((ch = command[index++]) != '\0') {
		if (ch == ' ') {
			while(ch == ' ') {
				ch = command[index++];
			}
			start = index;
			break;
		}
	}
	if(start > 0) {
		len = strlen((int8_t*) &command[start]);
	}
	len++;

	//get_executable_entrypoint

	return -1;
}

int32_t syscall_halt(uint8_t status) {
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

int32_t syscall_sigreturn(void) {
	return -1;
}