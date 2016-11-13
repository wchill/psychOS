#ifndef _PCB_H
#define _PCB_H

#include <types.h>
#include <arch/x86/paging.h>

#define PCB_BITMASK (~0x1FFF)
#define ELF_MAGIC_HEADER "\x7f\x45\x4c\x46"

#define KERNEL_PAGE_END 0x800000
#define PROCESS_PAGE_SIZE 0x400000
#define KERNEL_STACK_SIZE 0x2000
#define MAX_PROCESSES 2

typedef struct pcb_t pcb_t;

typedef struct file_t {
    uint32_t tp;    // table pointer
    uint32_t inode; // inode
    uint32_t fp;    // file position
    uint32_t flags; // flags
} file_t;

struct pcb_t {
	pd_entry process_pd[NUM_PD_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));
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
	file_t fd[8];
	uint8_t args[128];
	uint32_t pid;
	uint32_t esp0;
	uint32_t status;
	pcb_t *parent;
	pcb_t *child;
	uint8_t in_use;
};

typedef struct task_kernel_stack_t {
	pcb_t pcb;
	uint8_t stack[KERNEL_STACK_SIZE - sizeof(pcb_t)];
} task_kernel_stack_t;

void kernel_run_first_program(const int8_t* command);

void set_kernel_stack(const void *stack);
uint32_t get_executable_entrypoint(const void *executable);
pcb_t *get_pcb_from_esp(void *process_esp);
pcb_t *get_pcb_from_slot(uint32_t pcb_slot);
void *get_process_page_from_slot(uint32_t task_slot);
void *get_current_kernel_stack_base();
int32_t parse_args(const int8_t* command, int8_t *buf);

extern void switch_to_ring_3(void);

#endif
