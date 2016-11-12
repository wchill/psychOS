#ifndef _PCB_H
#define _PCB_H

#include <types.h>

#define PCB_BITMASK (~0x1FFF)
#define ELF_MAGIC_HEADER "\x7f\x45\x4c\x46"

typedef struct pcb_t pcb_t;

struct pcb_t {
	struct {
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
	} regs;
	uint32_t pid;
	int32_t fd[8];
	uint32_t esp0;
	uint32_t page_directory_addr;
	uint32_t status;
	pcb_t *parent;
	pcb_t *child;
	uint8_t *args;
};

typedef struct task_kernel_stack_t {
	pcb_t pcb;
	uint8_t stack[8192 - sizeof(pcb_t)];
} task_kernel_stack_t;

void set_kernel_stack(const void *stack);
uint32_t get_executable_entrypoint(const void *executable);
pcb_t *get_task_pcb(void *process_esp);
void *get_current_kernel_stack_base();
extern void switch_to_ring_3(void);

#endif
