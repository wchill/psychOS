#include "circular_buffer.h"
#include "terminal.h"
#include "keyboard_map.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"
#include "x86_desc.h"
#include "tests.h"

static volatile uint8_t keyboard_state[KEYBOARD_SIZE] = {0};
static volatile uint8_t caps_lock_status = 0;

static volatile uint8_t keyboard_buffer_internal[KEYBOARD_BUFFER_SIZE] = {0};
static volatile circular_buffer_t keyboard_buffer;
static volatile uint8_t keyboard_new_line_ready = 0;

static volatile uint16_t *video_buffer = (volatile uint16_t*) VIDEO_PTR;
static volatile uint8_t cursor_x = 0;
static volatile uint8_t cursor_y = 0;

// Sets VGA hardware cursor to be at (x, y) (0-indexed).
static void set_hardware_cursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;

    uint16_t index = VIDEO_INDEX(cursor_x, cursor_y);

    // Select index 14 in the CRTC register and write upper byte
    outb(14, VGA_CRTC_PORT_COMMAND);
    outb(index >> 8, VGA_CRTC_PORT_DATA);

    // Select index 15 in the CRTC register and write lower byte
    outb(15, VGA_CRTC_PORT_COMMAND);
    outb(index & 0xFF, VGA_CRTC_PORT_DATA);
}

// Places the given character at (x, y) in video memory.
static inline void video_buffer_putc(uint8_t x, uint8_t y, uint8_t ch) {
    uint16_t word = ch | (TERMINAL_FOREGROUND_COLOR | TERMINAL_BACKGROUND_COLOR);
    uint16_t index = VIDEO_INDEX(x, y);
    video_buffer[index] = word;
}

// Scrolls the screen vertically one row up.
static void scroll_screen() {
    memmove((uint16_t*) &video_buffer[VIDEO_INDEX(0, 0)], (uint16_t*) &video_buffer[VIDEO_INDEX(0, 1)], TERMINAL_COLUMNS * (TERMINAL_ROWS - 1) * 2);

    uint16_t blank_word = ' ' | (TERMINAL_FOREGROUND_COLOR | TERMINAL_BACKGROUND_COLOR);
    memset_word((uint16_t*) &video_buffer[VIDEO_INDEX(0, TERMINAL_ROWS - 1)], blank_word, TERMINAL_COLUMNS);
}

// Buffers one key from the keyboard and echoes it to the screen, updating the cursor position.
static uint8_t keyboard_putc(uint8_t ch) {
    if(ch == '\b') {
    // Backspace

        // Check if keyboard circular buffer is empty before erasing anything
        if (circular_buffer_len((circular_buffer_t*) &keyboard_buffer) == 0) return 0;

        // Check if last byte in buffer is a new line
        uint8_t last_ch;
        circular_buffer_peek_end_byte((circular_buffer_t*) &keyboard_buffer, &last_ch);
        if (last_ch == '\n') return 0;

        // Remove last byte in buffer
        circular_buffer_remove_end_byte((circular_buffer_t*) &keyboard_buffer);

        putc('\b');
    } else if(ch == '\n') {
    // New line

        // Check if buffer is full
        if (circular_buffer_len((circular_buffer_t*) &keyboard_buffer) >= KEYBOARD_BUFFER_SIZE) return 0;

        // Add new line to buffer
        circular_buffer_put_byte((circular_buffer_t*) &keyboard_buffer, ch);

        putc('\n');
        keyboard_new_line_ready++;

    } else if(ch == '\t') {
    // Tab, unimplemented
        return 0;

    } else if(ch > 0) {
    // All other characters considered printable

        // Check if there's space for at least 2 bytes (because we also need new line character)
        if (circular_buffer_len((circular_buffer_t*) &keyboard_buffer) < KEYBOARD_BUFFER_SIZE - 1) {

            // Add to buffer
            circular_buffer_put_byte((circular_buffer_t*) &keyboard_buffer, ch);

            putc(ch);
        } else {
            // No space in buffer, ignore key
            return 0;
        }
    }
    return 1;
}

void putc(uint8_t ch) {
    if(ch == '\b') {
        // Update cursor position
        if(cursor_x == 0) {
            cursor_y--;
            cursor_x = TERMINAL_COLUMNS;
        }
        cursor_x--;

        // Blank out the character at the cursor's new position
        video_buffer_putc(cursor_x, cursor_y, ' ');
    } else if (ch == '\n') {
        // Update cursor position
        cursor_y++;
        cursor_x = 0;
        if(cursor_y >= TERMINAL_ROWS) {
            // If cursor has reached the bottom, scroll the screen
            scroll_screen();
            cursor_y = TERMINAL_ROWS - 1;
        }
    } else if (ch == '\t') {
        // Unimplemented
    } else if (ch > 0) {
        // Echo to screen at cursor's current position
        video_buffer_putc(cursor_x, cursor_y, ch);

        // Update cursor
        if(++cursor_x >= TERMINAL_COLUMNS) {
            cursor_y++;
            cursor_x = 0;

            if(cursor_y >= TERMINAL_ROWS) {
                scroll_screen();
                cursor_y = TERMINAL_ROWS - 1;
            }
        }
    }
    set_hardware_cursor(cursor_x, cursor_y);
}

// Clears the terminal
void clear_terminal() {

    // 16-bit word representing space with black background and white foreground
    uint16_t blank_word = ' ' | (TERMINAL_FOREGROUND_COLOR | TERMINAL_BACKGROUND_COLOR);

    // Overwrite every byte in video memory with this and reset cursor to (0, 0)
    memset_word((uint16_t*) video_buffer, blank_word, TERMINAL_COLUMNS * TERMINAL_ROWS);
    set_hardware_cursor(0, 0);
}

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

            // If there's an escape code, we can just drop the keycode
            // Reference: http://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html#ss1.5
            if((uint8_t) keycode == KEYBOARD_ESCAPE_CODE) continue;

            if(keycode < 0) {
                // Key released
                keyboard_state[keycode & KEYCODE_MASK] = 0;
            } else {
                // Key pressed
                keyboard_state[keycode & KEYCODE_MASK] = 1;

                // Handle special keys, then print if printable
                uint8_t ctrl_pressed = keyboard_state[KEYBOARD_CTRL];
                uint8_t alt_pressed = keyboard_state[KEYBOARD_ALT];

                uint8_t shift_pressed = keyboard_state[KEYBOARD_LEFT_SHIFT] || keyboard_state[KEYBOARD_RIGHT_SHIFT];

                int map_index = shift_pressed | (caps_lock_status << 1);
                uint8_t pressed_char = keyboard_map[map_index][(int) keycode];

                // Toggle caps lock if necessary
                if(keycode == KEYBOARD_CAPS_LOCK) {
                    caps_lock_status = !caps_lock_status;
                }

                // Ctrl+L should clear the screen and place cursor at the top
                if(ctrl_pressed && pressed_char == 'l') {
                    int i;
                    uint8_t current_buf[KEYBOARD_BUFFER_SIZE];
                    uint32_t len = circular_buffer_peek((circular_buffer_t*) &keyboard_buffer, current_buf, KEYBOARD_BUFFER_SIZE);

                    clear_terminal();

                    for(i = 0; i < len; i++) {
                        putc(current_buf[i]);
                    }
                }

                if (ctrl_pressed && pressed_char >= '1' && pressed_char <= '5'){
                    test_suite(pressed_char - '0');
                }

                // Key combos aren't printable
                if(ctrl_pressed || alt_pressed) {
                    pressed_char = 0;
                }

                // Print to screen
                if(pressed_char > 0) {
                    keyboard_putc(pressed_char);
                }
            }
        }
    } while (status & 0x01);
    
    // Acknowledge interrupt
    send_eoi(KEYBOARD_IRQ);
}

// Keyboard syscalls
uint32_t terminal_open(const int8_t *filename) {
    // TODO: Assign proper file descriptor
    uint32_t flags;
    cli_and_save(flags);

    // Clear buffer and screen, then enable keyboard interrupts
    circular_buffer_init((circular_buffer_t*) &keyboard_buffer, (void*) keyboard_buffer_internal, KEYBOARD_BUFFER_SIZE);
    clear_terminal();
    enable_irq(KEYBOARD_IRQ);

    restore_flags(flags);
    return 0;
}

uint32_t terminal_read(int32_t fd, void *buf, int32_t nbytes) {
    // TODO: Do something with the fd

    uint32_t retval;
    uint32_t max_len;
    uint32_t flags;

    // Wait for Enter key
    while (!keyboard_new_line_ready);

    cli_and_save(flags);

    // Read up to min(nbytes, number of bytes available in buffered line)
    max_len = circular_buffer_find((circular_buffer_t*) &keyboard_buffer, '\n') + 1;
    if(max_len < nbytes) nbytes = max_len;
    retval = circular_buffer_get((circular_buffer_t*) &keyboard_buffer, buf, nbytes);

    keyboard_new_line_ready--;
    
    restore_flags(flags);
    return retval;
}

uint32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
    int i;
    uint32_t flags;
    cli_and_save(flags);

    // Write characters to screen
    for(i = 0; i < nbytes; i++) {
        putc(((uint8_t*)buf)[i]);
    }

    restore_flags(flags);
    return nbytes;
}

uint32_t terminal_close(int32_t fd) {
    // TODO: handle invalid file descriptor
    uint32_t flags;
    cli_and_save(flags);

    circular_buffer_init((circular_buffer_t*) &keyboard_buffer, (void*) keyboard_buffer_internal, KEYBOARD_BUFFER_SIZE);
    clear_terminal();

    restore_flags(flags);
    // disable_irq(KEYBOARD_IRQ);
    return 0;
}

// copy keyboard state to buf
void get_keyboard_state(uint8_t *buf) {
    uint32_t flags;
    cli_and_save(flags);

    memcpy(buf, (uint8_t*) keyboard_state, KEYBOARD_BUFFER_SIZE);

    restore_flags(flags);
}
