#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "interrupt.h"

uint32_t syscall_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

#endif
