#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "interrupt.h"

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

uint32_t syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx, uint32_t esi, uint32_t edi, uint32_t ebp);
extern void syscall_handler_wrapper(void);

uint32_t syscall_read(int32_t fd, void *buf, int32_t nbytes);
uint32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes);
uint32_t syscall_open(const uint8_t *filename);
uint32_t syscall_close(int32_t fd);

#endif
