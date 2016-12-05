#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include <lib/file.h>

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

#define NUM_TERMINALS 3

#define VGA_CRTC_PORT_COMMAND 0x3D4
#define VGA_CRTC_PORT_DATA 0x3D5

#define KEYBOARD_ESCAPE_CODE 0xE1
#define KEYBOARD_CTRL 0x1D
#define KEYBOARD_ALT 0x38
#define KEYBOARD_LEFT_SHIFT 0x2A
#define KEYBOARD_RIGHT_SHIFT 0x36
#define KEYBOARD_CAPS_LOCK 0x3A
#define KEYBOARD_F1 0x3B

#define VIDEO_VIRT_ADDR 0x8400000
#define VIDEO_PHYS_ADDR 0xB8000

// Useful macro to get index into a uint16_t array representing video memory
#define VIDEO_INDEX(x, y) ((y) * TERMINAL_COLUMNS + (x))

void keyboard_handler();
extern void keyboard_handler_wrapper(void);

void reset_terminal(uint8_t terminal_num);
void multiple_terminal_init();

uint16_t *get_terminal_output_buffer(uint8_t terminal_num);

void clear_terminal(uint8_t terminal_num);
void putc(uint8_t c);

int32_t terminal_open(file_t *f, const int8_t *filename);
int32_t terminal_close(file_t *f);
int32_t terminal_read(file_t *f, void *buf, int32_t nbytes);
int32_t terminal_write(file_t *f, const void *buf, int32_t nbytes);

void get_keyboard_state(uint8_t *buf); // Rodney added for testing

#endif
