#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

// http://wiki.osdev.org/%228042%22_PS/2_Controller#PS.2F2_Controller_IO_Ports
#define KEYBOARD_IRQ 1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYCODE_MASK 0x7F

void keyboard_handler();
extern void keyboard_handler_wrapper(void);

#endif
