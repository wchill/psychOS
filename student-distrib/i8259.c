/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
    // Rodney: I replaced Linux's outb_p instructions with outb instructions. Hopefully that's okay
    unsigned long flags;

    // Rodney: may eventually need a spinlock here
    cli_and_save(flags);

    outb(EIGHT_BIT_MASK, MASTER_8259_PORT_DATA);      /* mask all of 8259A-1 */
    outb(EIGHT_BIT_MASK, SLAVE_8259_PORT_DATA);       /* mask all of 8259A-2 */

    /* Instructions for Master */
    outb(ICW1,        MASTER_8259_PORT_COMMAND); /* ICW1: select 8259A-1 init */
    outb(ICW2_MASTER, MASTER_8259_PORT_DATA);    /* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
    outb(ICW3_MASTER, MASTER_8259_PORT_DATA);    /* 8259A-1 (the master) has a slave on IR2 */
    outb(ICW4,        MASTER_8259_PORT_DATA);    /* master expects normal EOI */

    /* Instructions for Slave */
    outb(ICW1,       SLAVE_8259_PORT_COMMAND);   /* ICW1: select 8259A-2 init */      
    outb(ICW2_SLAVE, SLAVE_8259_PORT_DATA);      /* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
    outb(ICW3_SLAVE, SLAVE_8259_PORT_DATA);      /* 8259A-2 is a slave on master's IR2 */
    outb(ICW4,       SLAVE_8259_PORT_DATA);      /* (slave's support for AEOI in flat mode is to be investigated) */

    restore_flags(flags);
}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
    unsigned int mask = ~(1 << irq_num);
    unsigned long flags;

    // Rodney: may eventually need a spinlock here
    cli_and_save(flags);

    cached_irq_mask &= mask;
    if (irq_num & SLAVE_BIT)
        outb(CACHED_SLAVE, SLAVE_8259_PORT_DATA);   // Rodney: CACHED_SLAVE  is a "dynamic" changing value. See header file.
    else
        outb(CACHED_MASTER, MASTER_8259_PORT_DATA); // Rodney: CACHED_MASTER is a "dynamic" changing value. See header file.

    restore_flags(flags);
}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
    unsigned int mask = 1 << irq_num;
    unsigned long flags;

    // Rodney: may eventually need a spinlock here
    cli_and_save(flags);

    cached_irq_mask |= mask;
    if (irq_num & SLAVE_BIT)
        outb(CACHED_SLAVE, SLAVE_8259_PORT_DATA);   // Rodney: CACHED_SLAVE  is a "dynamic" changing value. See header file.
    else
        outb(CACHED_MASTER, MASTER_8259_PORT_DATA); // Rodney: CACHED_MASTER is a "dynamic" changing value. See header file.

    restore_flags(flags);
}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
	// http://wiki.osdev.org/PIC#End_of_Interrupt
    if (irq_num & SLAVE_BIT)
    	outb(0x20, SLAVE_8259_PORT_COMMAND);
    outb(0x20, MASTER_8259_PORT_COMMAND);
}

