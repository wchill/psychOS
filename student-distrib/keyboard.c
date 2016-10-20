#include "keyboard.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"
#include "x86_desc.h"

/* The following array is taken from 
    http://www.osdever.net/bkerndev/Docs/keyboard.htm
   All credits where due
*/
static const uint8_t keyboard_map[KEYBOARD_SIZE] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

static volatile uint8_t keyboard_state[KEYBOARD_SIZE] = {0};

/*
 * keyboard_handler
 *   DESCRIPTION:  this code runs when keyboard Interrupt happens. Outputs keys pressed
 *   INPUTS:       none
 *   OUTPUTS:      Outputs characters to screen
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Screen is updated
 */  
void keyboard_handler() {
	uint8_t status;
	do {
		// Check keyboard status
		status = (uint8_t) inb(KEYBOARD_STATUS_PORT);

		// If this bit is set, data is available
		if(status & 0x01) {
			int8_t keycode = (int8_t) inb(KEYBOARD_DATA_PORT);
			if(keycode < 0) {
				// Key released
				keyboard_state[keycode & KEYCODE_MASK] = 0;
			} else {
				// Key pressed
				keyboard_state[keycode & KEYCODE_MASK] = 1;

				// Update pressed keys, output key if it's printable
				uint8_t ch = keyboard_map[(int) keycode];
				if(ch > 0) {
					putc(ch);
				}
			}
		}
	} while (status & 0x01);
  
  // Acknowledge interrupt
  send_eoi(KEYBOARD_IRQ);
}
