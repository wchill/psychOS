#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <arch/x86/interrupt.h>

#define SYSCALL_HALT 1
#define SYSCALL_EXECUTE 2
#define SYSCALL_READ 3
#define SYSCALL_WRITE 4
#define SYSCALL_OPEN 5
#define SYSCALL_CLOSE 6
#define SYSCALL_GETARGS 7
#define SYSCALL_VIDMAP 8
#define SYSCALL_SET_HANDLER 9
#define SYSCALL_SIGRETURN 10

#define SYSCALL_EINVAL -1

extern void syscall_handler_wrapper(void);

int32_t syscall_read(uint32_t esp, int32_t fd, void *buf, int32_t nbytes);
int32_t syscall_write(uint32_t esp, int32_t fd, const void *buf, int32_t nbytes);
int32_t syscall_open(uint32_t esp, const uint8_t *filename);
int32_t syscall_close(uint32_t esp, int32_t fd);
int32_t syscall_execute(uint32_t esp, const int8_t *command, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, uint32_t eip, uint32_t cs, uint32_t eflags);
int32_t syscall_halt(uint32_t esp, uint8_t status);
int32_t syscall_getargs(uint32_t esp, uint8_t *buf, int32_t nbytes);
int32_t syscall_vidmap(uint32_t esp, uint8_t **screen_start);
int32_t syscall_set_handler(uint32_t esp, int32_t signum, void *handler_address);
int32_t syscall_sigreturn(uint32_t esp);

#endif
