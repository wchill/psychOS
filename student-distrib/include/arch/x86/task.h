#ifndef _PCB_H
#define _PCB_H

#include <types.h>
#include <arch/x86/paging.h>
#include <fs/fs.h>

#define PCB_BITMASK (~0x1FFF)
#define ELF_MAGIC_HEADER "\x7f\x45\x4c\x46"
#define ELF_MAGIC_HEADER_LEN 4
#define ELF_ENTRYPOINT_OFFSET 24

#define KERNEL_PAGE_END 0x800000
#define PROCESS_PAGE_SIZE 0x400000
#define KERNEL_STACK_SIZE 0x2000

#define MAX_PROCESSES 2
#define MAX_FILE_DESCRIPTORS 8
#define MAX_PROGRAM_NAME_LENGTH 128  // Rodney: I picked this length arbitrarily
#define MAX_ARGS_LENGTH 128

#define PROCESS_LINK_OFFSET 0x48000
#define PROCESS_VIRT_PAGE_START 0x8000000
#define PROCESS_LINK_START (PROCESS_VIRT_PAGE_START + PROCESS_LINK_OFFSET)

typedef struct pcb_t pcb_t;

struct pcb_t {
	pd_entry process_pd[NUM_PD_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));
	struct {
		uint32_t esp;
		uint32_t ebp;
	} regs;
	file_t fa[MAX_FILE_DESCRIPTORS];
	int8_t program_name[MAX_PROGRAM_NAME_LENGTH];
	int8_t args[MAX_ARGS_LENGTH];
	uint32_t slot_num;
	uint32_t pid;
	pcb_t *parent;
	pcb_t *child;
	uint8_t in_use;
};

typedef struct task_kernel_stack_t {
	pcb_t pcb;
	uint8_t stack[KERNEL_STACK_SIZE - sizeof(pcb_t)];
} task_kernel_stack_t;

void open_stdin_and_stdout();
void kernel_run_first_program(const int8_t* command);

void set_kernel_stack(const void *stack);
uint32_t get_executable_entrypoint(const void *executable);
pcb_t *get_pcb_from_esp(void *process_esp);
pcb_t *get_pcb_from_slot(uint32_t pcb_slot);
pcb_t *get_current_pcb();
void *get_process_page_from_slot(uint32_t task_slot);
void *get_current_kernel_stack_base();
void *get_kernel_stack_base_from_slot(uint32_t pcb_slot);
int32_t parse_command(const int8_t* command, int8_t *buf_name, int8_t *buf_args);
uint32_t load_program_into_slot(const int8_t *filename, uint32_t pcb_slot);

extern void switch_to_ring_3(uint32_t esp, uint32_t eip);

#endif
