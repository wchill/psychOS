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

int32_t syscall_read(int32_t fd, void *buf, int32_t nbytes);
int32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes);
int32_t syscall_open(const uint8_t *filename);
int32_t syscall_close(int32_t fd);
int32_t syscall_execute(const int8_t *command);
int32_t syscall_halt(uint8_t status);
int32_t syscall_getargs(uint8_t *buf, int32_t nbytes);
int32_t syscall_vidmap(uint8_t **screen_start);
int32_t syscall_set_handler(int32_t signum, void *handler_address);
int32_t syscall_sigreturn();

#endif
