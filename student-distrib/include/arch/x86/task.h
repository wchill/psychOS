#ifndef _PCB_H
#define _PCB_H

#include <types.h>

#define PCB_BITMASK (~0x1FFF)

typedef struct pcb_t pcb_t;

struct pcb_t {
	uint32_t pid;
	int32_t fd[8];
	uint32_t esp0;
	uint32_t page_directory_addr;
	uint32_t status;
	pcb_t *parent;
	pcb_t *child;
	struct regs {
		uint32_t eax;
		uint32_t ebx;
		uint32_t ecx;
		uint32_t edx;
		uint32_t esi;
		uint32_t edi;
		uint32_t esp;
		uint32_t ebp;
		uint32_t eip;
		uint32_t eflags;
	}
};

typedef struct task_kernel_stack_t {
	pcb_t pcb;
	uint8_t stack[8192 - sizeof(pcb_t)];
} task_kernel_stack_t;

void set_kernel_stack(void *stack);
extern void switch_to_ring_3(void);

#endif
