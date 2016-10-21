/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = EIGHT_BIT_MASK; /* IRQs 0-7  */
uint8_t slave_mask = EIGHT_BIT_MASK;  /* IRQs 8-15 */

/* PIC References:
		https://www.kernel.org/pub/linux/kernel/people/marcelo/linux-2.4/arch/mips64/kernel/i8259.c  (has actual Linux code for first 3 functions below)
		Lecture 9 (especially slide 21)
*/

/* Initialize the 8259 PIC */
void
i8259_init(void)
{
    outb(master_mask, MASTER_8259_PORT_DATA);    /* mask all of 8259A-1 */
    outb(slave_mask,  SLAVE_8259_PORT_DATA);     /* mask all of 8259A-2 */

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

    master_mask &= (~ICW3_MASTER);               // Rodney: this enables IRQ 2 so that slave works.
    outb(master_mask, MASTER_8259_PORT_DATA);
}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
    if (irq_num & SLAVE_BIT){
        slave_mask &= ~(1 << (irq_num - IRQ_MASTER_OFFSET));
        outb(slave_mask, SLAVE_8259_PORT_DATA);
    }
    else{
        master_mask &= ~(1 << irq_num);
        outb(master_mask, MASTER_8259_PORT_DATA);
    }
}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
    if (irq_num & SLAVE_BIT){
        slave_mask |= (1 << (irq_num - IRQ_MASTER_OFFSET));
        outb(slave_mask, SLAVE_8259_PORT_DATA);
    }
    else{
        master_mask |=  (1 << irq_num);
        outb(master_mask, MASTER_8259_PORT_DATA);
    }
}

/* Send end-of-interrupt signal for the specified IRQ */
void
send_eoi(uint32_t irq_num)
{
    if (irq_num & SLAVE_BIT){
        outb(EOI | (irq_num - IRQ_MASTER_OFFSET), SLAVE_8259_PORT_COMMAND); 
        outb(EOI | 2, MASTER_8259_PORT_COMMAND); // 2 represents Slave connected to IRQ 2
    }
    else
        outb(EOI | irq_num, MASTER_8259_PORT_COMMAND);
}
