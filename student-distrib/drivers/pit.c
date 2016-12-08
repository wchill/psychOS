#include <drivers/pit.h>
#include <arch/x86/task.h>
#include <lib/lib.h>
#include <arch/x86/i8259.h>
#include <arch/x86/io.h>
#include <tty/terminal.h>

static volatile uint32_t system_ticks = 0;

/**
 * context_switch
 * Saves the current process's state and loads another process for execution.
 *
 * @param last_pcb 	A pointer to the PCB of the process being swapped out
 * @param pcb 		A pointer to the PCB of the process being run
 */
static void context_switch(pcb_t *last_pcb, pcb_t *pcb) {
	cli();

	set_process_vmem_page(pcb->slot_num, get_terminal_output_buffer(pcb->terminal_num));
	enable_paging(pcb->process_pd_ptr);

	asm volatile(
		"mov %%esp, %0\r\n"
		"mov %%ebp, %1\r\n"
		: "=g"(last_pcb->regs.esp), "=g"(last_pcb->regs.ebp)
		: 
		: "memory"
	);

	// Prepare for context switch
	set_kernel_stack(get_kernel_stack_base_from_slot(pcb->slot_num));

	if(pcb->regs.esp == NULL) {
		send_eoi(PIT_IRQ);

	    // Start the program
	    switch_to_ring_3(PROCESS_LINK_START, pcb->entrypoint);
	}

	asm volatile(
		"mov %0, %%esp\r\n"
		"mov %1, %%ebp\r\n"
		:
		: "r"(pcb->regs.esp), "r"(pcb->regs.ebp)
		: "memory"
	);
}

/**
 * scheduler
 * Finds a process that should next be run and preempts the current process if one is available.
 */
void scheduler() {
	int i;

	pcb_t *former_pcb = get_current_pcb();
	int last_slot_checked = former_pcb->slot_num - 1;
	if(last_slot_checked < 0) last_slot_checked += MAX_PROCESSES;
	for(i = former_pcb->slot_num + 1; i != last_slot_checked; i = (i+1) % MAX_PROCESSES) {
		pcb_t *current_pcb = get_pcb_from_slot(i);
		if(current_pcb->in_use && current_pcb->status == PROCESS_RUNNING) {
			context_switch(former_pcb, current_pcb);
			break;
		}
	}

	send_eoi(PIT_IRQ);

	// Unable to find a suitable process to switch to, exit
}

/**
 * pit_init
 * Initializes the PIT so it can be used for preemptive multitasking.
 *
 * @param hertz 	The rate that the PIT should be run at (currently ignored)
 */
void pit_init(uint32_t hertz) {
	// 100 Hz = 10ms per tick/context switch
	// For now, we ignore the given parameter
	hertz = 100;

	uint16_t divisor = PIT_FREQUENCY / hertz;

	// Put the PIT into binary counting mode, square wave generator, 16-bit receiving mode for the divisor, and use channel 0
	outportb(PIT_CMD_REG_PORT, PIT_BINARY_VAL | PIT_CMD_MODE3 | PIT_CMD_RW_BOTH | PIT_CMD_COUNTER0);

	// Send the clock divisor
	outportb(PIT_CH0_DATA_PORT, (uint8_t) (divisor & LOW_EIGHT_BIT_BITMASK));
	outportb(PIT_CH0_DATA_PORT, (uint8_t) ((divisor >> 8) & LOW_EIGHT_BIT_BITMASK)); // 8 represents shifting to get bits 8-15 into bits 0-7, to apply bitmask.
}

/**
 * pit_handler
 * Interrupt handler for the PIT - just increments the number of ticks that have passed and runs the scheduler.
 */
void pit_handler() {
	system_ticks++;
	scheduler();
}
