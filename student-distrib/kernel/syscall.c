#include <syscall.h>

uint32_t syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, uint32_t ebp) {
	switch(eax) {
		case SYSCALL_READ:
			return syscall_read((int32_t) ebx, (void*) ecx, (int32_t) edx);
		case SYSCALL_WRITE:
			return syscall_write((int32_t) ebx, (void*) ecx, (int32_t) edx);
		case SYSCALL_OPEN:
			return syscall_open((uint8_t*) ebx);
		case SYSCALL_CLOSE:
			return syscall_close((int32_t) ebx);

		/* Currently unimplemented */
		case SYSCALL_HALT:
		case SYSCALL_EXECUTE:
		case SYSCALL_GETARGS:
		case SYSCALL_VIDMAP:
		case SYSCALL_SET_HANDLER:
		case SYSCALL_SIGRETURN:
		default:
			return SYSCALL_EINVAL;
	}
}

uint32_t syscall_open(const uint8_t *filename) {
	return -1;
}

uint32_t syscall_read(int32_t fd, void *buf, int32_t nbytes) {
	return -1;
}

uint32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}

uint32_t syscall_close(int32_t fd) {
	return -1;
}
