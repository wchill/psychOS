/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include <arch/x86/i8259.h>
#include <lib/lib.h>

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = EIGHT_BIT_MASK; /* IRQs 0-7  */
uint8_t slave_mask = EIGHT_BIT_MASK;  /* IRQs 8-15 */

/* PIC References:
		https://www.kernel.org/pub/linux/kernel/people/marcelo/linux-2.4/arch/mips64/kernel/i8259.c  (has actual Linux code for first 3 functions below)
		Lecture 9 (especially slide 21)
*/

/*
 * i8259_init
 * Initializes the 8259 PIC
 */
void
i8259_init(void)
{
    outportb(MASTER_8259_PORT_DATA, master_mask);   /* mask all of 8259A-1 */
    outportb(SLAVE_8259_PORT_DATA, slave_mask);     /* mask all of 8259A-2 */

    /* Instructions for Master */
    outportb(MASTER_8259_PORT_COMMAND, ICW1);       /* ICW1: select 8259A-1 init */
    outportb(MASTER_8259_PORT_DATA, ICW2_MASTER);   /* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
    outportb(MASTER_8259_PORT_DATA);                /* 8259A-1 (the master, ICW3_MASTER) has a slave on IR2 */
    outportb(MASTER_8259_PORT_DATA, ICW4);          /* master expects normal EOI */

    /* Instructions for Slave */
    outportb(SLAVE_8259_PORT_COMMAND, ICW1);        /* ICW1: select 8259A-2 init */      
    outportb(SLAVE_8259_PORT_DATA, ICW2_SLAVE);     /* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
    outportb(SLAVE_8259_PORT_DATA, ICW3_SLAVE);     /* 8259A-2 is a slave on master's IR2 */
    outportb(SLAVE_8259_PORT_DATA);                 /* (slave's support for AEOI in flat mode is to be investigated, ICW4) */

    master_mask &= (~ICW3_MASTER);               // Rodney: this enables IRQ 2 so that slave works.
    outportb(MASTER_8259_PORT_DATA, master_mask);
}

/*
 * enable_irq
 * Enables (unmasks) the specified IRQ
 * 
 * @param irq_num    the IRQ number to unmask
 */
void
enable_irq(uint32_t irq_num)
{
    if (irq_num & SLAVE_BIT){
        slave_mask &= ~(1 << (irq_num - IRQ_MASTER_OFFSET));
        outportb(SLAVE_8259_PORT_DATA, slave_mask);
    }
    else{
        master_mask &= ~(1 << irq_num);
        outportb(MASTER_8259_PORT_DATA, master_mask);
    }
}

/*
 * disable_irq
 * Disables (masks) the specified IRQ
 * 
 * @param irq_num    the IRQ number to unmask
 */
void
disable_irq(uint32_t irq_num)
{
    if (irq_num & SLAVE_BIT){
        slave_mask |= (1 << (irq_num - IRQ_MASTER_OFFSET));
        outportb(SLAVE_8259_PORT_DATA, slave_mask);
    }
    else{
        master_mask |=  (1 << irq_num);
        outportb(MASTER_8259_PORT_DATA, master_mask);
    }
}

/*
 * send_eoi
 * Send end-of-interrupt signal for the specified IRQ
 * 
 * @param irq_num    The number IRQ to send EOI for.
 */
void
send_eoi(uint32_t irq_num)
{
    if (irq_num & SLAVE_BIT){
        outportb(SLAVE_8259_PORT_COMMAND, EOI | (irq_num - IRQ_MASTER_OFFSET)); 
        outportb(MASTER_8259_PORT_COMMAND, EOI | 2); // 2 represents Slave connected to IRQ 2
    }
    else
        outportb(MASTER_8259_PORT_COMMAND, EOI | irq_num);
}
