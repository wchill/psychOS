#ifndef _PIC_H
#define _PIC_H

#include <types.h>

void pit_handler();
extern void pit_handler_wrapper(void);

#endif