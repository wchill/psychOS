#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

// http://wiki.osdev.org/%228042%22_PS/2_Controller#PS.2F2_Controller_IO_Ports
#define KEYBOARD_IRQ 1
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYCODE_MASK 0x7F

#define KEYBOARD_SIZE 128
#define KEYBOARD_BUFFER_SIZE 128

#define TERMINAL_COLUMNS 80
#define TERMINAL_ROWS 25
#define TERMINAL_FOREGROUND_COLOR 0x0F00
#define TERMINAL_BACKGROUND_COLOR 0x0000

#define VIDEO_PTR ((uint8_t*) 0xB8000)

#define VGA_CRTC_PORT_COMMAND 0x3D4
#define VGA_CRTC_PORT_DATA 0x3D5

#define KEYBOARD_ESCAPE_CODE 0xE1
#define KEYBOARD_CTRL 0x1D
#define KEYBOARD_ALT 0x38
#define KEYBOARD_LEFT_SHIFT 0x2A
#define KEYBOARD_RIGHT_SHIFT 0x36
#define KEYBOARD_CAPS_LOCK 0x3A

// Useful macro to get index into a uint16_t array representing video memory
#define VIDEO_INDEX(x, y) ((y) * TERMINAL_COLUMNS + (x))

void keyboard_handler();
extern void keyboard_handler_wrapper(void);

void clear_terminal();
void putc(uint8_t c);

uint32_t terminal_open(const int8_t *filename);
uint32_t terminal_close(int32_t fd);
uint32_t terminal_read(int32_t fd, void *buf, int32_t nbytes);
uint32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes);

void get_keyboard_state(uint8_t *buf); // Rodney added for testing

#endif
