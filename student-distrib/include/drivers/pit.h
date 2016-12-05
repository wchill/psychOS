#ifndef _PIC_H
#define _PIC_H

#include <types.h>

#define PIT_IRQ 0
#define PIT_CH0_DATA_PORT 0x40
#define PIT_CMD_REG_PORT 0x43

#define PIT_BINARY_VAL                      0x00    // Binary counter values
#define PIT_BCD_VAL                         0x01    // BCD counter values

#define PIT_CMD_MODE0                       0x00    // Interrupt on Terminal Count
#define PIT_CMD_MODE1                       0x02    // Hardware Retriggerable One-Shot
#define PIT_CMD_MODE2                       0x04    // Rate Generator
#define PIT_CMD_MODE3                       0x06    // Square Wave
#define PIT_CMD_MODE4                       0x08    // Software Trigerred Strobe
#define PIT_CMD_MODE5                       0x0a    // Hardware Trigerred Strobe

#define PIT_CMD_LATCH                       0x00
#define PIT_CMD_RW_LOW                      0x10    // Least Significant Byte
#define PIT_CMD_RW_HI                       0x20    // Most Significant Byte
#define PIT_CMD_RW_BOTH                     0x30    // Least followed by Most Significant Byte

#define PIT_CMD_COUNTER0                    0x00
#define PIT_CMD_COUNTER1                    0x40
#define PIT_CMD_COUNTER2                    0x80
#define PIT_CMD_READBACK                    0xc0

#define PIT_FREQUENCY 1193182

void scheduler();

void pit_init(uint32_t hertz);
void pit_handler();
extern void pit_handler_wrapper(void);

#endif
