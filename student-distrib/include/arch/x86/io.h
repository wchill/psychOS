#ifndef _X86_PORT_IO_H
#define _X86_PORT_IO_H

#include <types.h>

/* Port read functions */
uint8_t inportb(uint16_t port);
uint16_t inportw(uint16_t port);
uint32_t inportl(uint16_t port);

/* Port write functions */
void outportb(uint16_t port, uint8_t data);
void outportw(uint16_t port, uint16_t data);
void outportl(uint16_t port, uint32_t data);

#endif

