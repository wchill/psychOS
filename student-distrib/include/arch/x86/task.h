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

#define PROCESS_LINK_OFFSET 0x48000

typedef struct pcb_t pcb_t;

typedef struct file_ops {
    int32_t (* open) (const uint8_t * filename);
    int32_t (* read) (int32_t fd, void * buf, int32_t nbytes);
    int32_t (* write) (int32_t fd, const void * buf, int32_t nbytes);
    int32_t (* close) (int32_t fd);
    // TODO other sys calls
} file_ops;

typedef struct file_t {
    file_ops  * fops;          // a pointer to methods that we can use to manipulate file data (open, close, read, write)
    uint32_t    file_position; // a pointer within file. Will tell us where to read/write within the file.
    uint32_t    flags;         // in our case it's not for synchronization. It will be used to indicate if file descriptor is busy or free
    uint32_t    inode;         // a number that indicates which file we are talking about.
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
	file_t fa[8];
	int8_t args[128];
	uint32_t slot_num;
	uint32_t pid;
	uint32_t esp0;
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
void *get_kernel_stack_base_from_slot(uint32_t pcb_slot);
int32_t parse_args(const int8_t* command, int8_t *buf);
uint32_t load_program_into_slot(const int8_t *filename, uint32_t pcb_slot);

extern void switch_to_ring_3(uint32_t esp, uint32_t eip);

#endif
