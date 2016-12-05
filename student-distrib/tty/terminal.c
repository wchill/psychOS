#include <lib/circular_buffer.h>
#include <tty/terminal.h>
#include <tty/keyboard_map.h>
#include <arch/x86/i8259.h>
#include <kernel/tests.h>
#include <types.h>
#include <lib/lib.h>
#include <arch/x86/x86_desc.h>
#include <arch/x86/io.h>
#include <arch/x86/paging.h>
#include <arch/x86/task.h>

static volatile uint8_t keyboard_state[KEYBOARD_SIZE] = {0};
static volatile uint8_t caps_lock_status = 0;

static volatile uint8_t input_buffer_internal[NUM_TERMINALS][KEYBOARD_BUFFER_SIZE];
static volatile circular_buffer_t input_buffer[NUM_TERMINALS];
static volatile uint8_t new_line_ready[NUM_TERMINALS];

static volatile uint16_t output_buffer[NUM_TERMINALS][FOUR_KB_ALIGNED];
static volatile uint8_t cursor_location[NUM_TERMINALS][2];

static volatile uint8_t active_terminal = 0;
static volatile uint16_t *video_memory = (volatile uint16_t*) VIDEO_VIRT_ADDR;

static volatile uint8_t single_terminal = 1;
 
static inline uint8_t get_process_terminal() {
    if(single_terminal) return 0;
    else {
        pcb_t *pcb = get_current_pcb();
        return pcb->terminal_num;
    }
}

/*
 * set_hardware_cursor
 * Sets VGA hardware cursor to be at (x, y) (0-indexed).
 * 
 * @param x   (0-indexed) x coordinate for cursor
 * @param y   (0-indexed) y coordinate for cursor
 */
static void set_hardware_cursor(uint8_t terminal_num, uint8_t x, uint8_t y) {
    cursor_location[terminal_num][0] = x;
    cursor_location[terminal_num][1] = y;

    if(terminal_num == active_terminal) {
        uint16_t index = VIDEO_INDEX(x, y);

        // Select index 14 in the CRTC register and write upper byte
        outportb(VGA_CRTC_PORT_COMMAND, 14);
        outportb(VGA_CRTC_PORT_DATA, index >> 8); // 8 is a bitshift to get rid of lowest 8 bits

        // Select index 15 in the CRTC register and write lower byte
        outportb(VGA_CRTC_PORT_COMMAND, 15);
        outportb(VGA_CRTC_PORT_DATA, index & 0xFF);  // 0xFF is a mash to grab lowest 8 bits
    }
}

/*
 * video_buffer_putc
 * Places the given character at (x, y) in video memory.
 * 
 * @param x   (0-indexed) x coordinate for cursor
 * @param y   (0-indexed) y coordinate for cursor
 * @param ch  the character to place in video memory
 */
static inline void video_buffer_putc(uint8_t x, uint8_t y, uint8_t ch) {
    uint16_t word = ch | (TERMINAL_FOREGROUND_COLOR | TERMINAL_BACKGROUND_COLOR); // Bits 0-7 are charcter. Bits 8-15 are color
    uint16_t index = VIDEO_INDEX(x, y);
    video_memory[index] = word;
}

/*
 * scroll_screen
 * Scrolls the screen vertically one row up.
 */
static void scroll_screen() {
    // Move the last 3840 bytes (79 rows) forward 160 bytes (1 row), overwriting the first row
    memmove((uint16_t*) &video_memory[VIDEO_INDEX(0, 0)], (uint16_t*) &video_memory[VIDEO_INDEX(0, 1)], TERMINAL_COLUMNS * (TERMINAL_ROWS - 1) * 2); // 2 is to double the value

    // Set the last row to be blank
    uint16_t blank_word = ' ' | (TERMINAL_FOREGROUND_COLOR | TERMINAL_BACKGROUND_COLOR);
    memset_word((uint16_t*) &video_memory[VIDEO_INDEX(0, TERMINAL_ROWS - 1)], blank_word, TERMINAL_COLUMNS);
}

/*
 * keyboard_putc
 * Buffers one key from the keyboard and echoes it to the screen, updating the cursor position.
 * 
 * @param ch  the character to buffer and echo to screen.
 *
 * @returns   number of characters echoed to screen (0 or 1)
 */
static uint8_t keyboard_putc(uint8_t terminal_num, uint8_t ch) {
    if(ch == '\b') {
    // Backspace

        // Check if keyboard circular buffer is empty before erasing anything
        if (circular_buffer_len((circular_buffer_t*) &input_buffer[terminal_num]) == 0) return 0;

        // Check if last byte in buffer is a new line
        uint8_t last_ch;
        circular_buffer_peek_end_byte((circular_buffer_t*) &input_buffer[terminal_num], &last_ch);
        if (last_ch == '\n') return 0;

        // Remove last byte in buffer
        circular_buffer_remove_end_byte((circular_buffer_t*) &input_buffer[terminal_num]);

        if(last_ch == '\t') {
            uint8_t pos = cursor_location[terminal_num][0] - (cursor_location[terminal_num][0] & 4);
            switch(pos) {
                case 0:
                    putc('\b');
                case 3:
                    putc('\b');
                case 2:
                    putc('\b');
                case 1:
                    putc('\b');
            }
        } else {
            putc('\b');
        }
    } else if(ch == '\n') {
    // New line

        // Check if buffer is full
        if (circular_buffer_len((circular_buffer_t*) &input_buffer[terminal_num]) >= KEYBOARD_BUFFER_SIZE) return 0;

        // Add new line to buffer
        circular_buffer_put_byte((circular_buffer_t*) &input_buffer[terminal_num], ch);

        putc('\n');

        // We read a new line
        new_line_ready[terminal_num]++;

    } else if(ch == '\t') {
        // Check if there's space for at least 2 bytes (because we also need new line character)
        if (circular_buffer_len((circular_buffer_t*) &input_buffer[terminal_num]) < KEYBOARD_BUFFER_SIZE - 1) {

            // Add to buffer
            circular_buffer_put_byte((circular_buffer_t*) &input_buffer[terminal_num], ch);

            putc(ch);
        } else {
            // No space in buffer, ignore key
            return 0;
        }

        uint8_t pos = cursor_location[terminal_num][0] & 4;
        switch(pos) {
            case 0:
                putc(' ');
            case 1:
                putc(' ');
            case 2:
                putc(' ');
            case 3:
                putc(' ');
        }
    } else if(ch > 0) {
    // All other characters considered printable

        // Check if there's space for at least 2 bytes (because we also need new line character)
        if (circular_buffer_len((circular_buffer_t*) &input_buffer[terminal_num]) < KEYBOARD_BUFFER_SIZE - 1) {

            // Add to buffer
            circular_buffer_put_byte((circular_buffer_t*) &input_buffer[terminal_num], ch);

            putc(ch);
        } else {
            // No space in buffer, ignore key
            return 0;
        }
    }
    return 1;
}

/*
 * putc
 * Outputs a character to the screen. Scrolls screen vertically if necessary.
 * 
 * @param ch  The character to output to the screen
 */
void putc(uint8_t ch) {
    uint8_t terminal_num = get_process_terminal();

    uint8_t cursor_x = cursor_location[terminal_num][0];
    uint8_t cursor_y = cursor_location[terminal_num][1];

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
    } else {
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
    set_hardware_cursor(terminal_num, cursor_x, cursor_y);
}

/*
 * clear_terminal
 * Clears the terminal
 */
void clear_terminal(uint8_t terminal_num) {

    // 16-bit word representing space with black background and white foreground
    uint16_t blank_word = ' ' | (TERMINAL_FOREGROUND_COLOR | TERMINAL_BACKGROUND_COLOR);

    // Overwrite every byte in video memory with this and reset cursor to (0, 0)
    memset_word((uint16_t*) video_memory, blank_word, TERMINAL_COLUMNS * TERMINAL_ROWS);
    set_hardware_cursor(terminal_num, 0, 0);
}

/*
 * keyboard_handler
 * runs when keyboard Interrupt happens. Outputs keys pressed. Can also run tests (Ctrl+1 to Ctrl+5)
 */
void keyboard_handler() {
    uint8_t terminal_num = get_process_terminal();
    uint8_t status;
    do {
        // Check keyboard status
        status = inportb(KEYBOARD_STATUS_PORT);

        // If this bit is set, data is available
        if(status & 0x01) {
            int8_t keycode = (int8_t) inportb(KEYBOARD_DATA_PORT);

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

                // Run test suite for Ctrl+1 to Ctrl+5
                if (ctrl_pressed && pressed_char >= '1' && pressed_char <= '5'){
                    test_suite(pressed_char - '0');
                }

                // Ctrl+L should clear the screen and place cursor at the top, but not clear the buffer
                if(ctrl_pressed && pressed_char == 'l') {
                    int i;
                    uint8_t current_buf[KEYBOARD_BUFFER_SIZE];
                    uint32_t len = circular_buffer_peek((circular_buffer_t*) &input_buffer[terminal_num], current_buf, KEYBOARD_BUFFER_SIZE);

                    clear_terminal(terminal_num);

                    for(i = 0; i < len; i++) {
                        putc(current_buf[i]);
                    }
                }

                // Ctrl/Alt key combos aren't printable
                if(ctrl_pressed || alt_pressed) {
                    pressed_char = 0;
                }

                // Print to screen
                if(pressed_char > 0) {
                    keyboard_putc(terminal_num, pressed_char);
                }
            }
        }
    } while (status & 0x01);
    
    // Acknowledge interrupt
    send_eoi(KEYBOARD_IRQ);
}

void multiple_terminal_init() {
    uint32_t flags;
    cli_and_save(flags);

    int i;
    for(i = 0; i < NUM_TERMINALS; i++) {
        circular_buffer_clear((circular_buffer_t*) &input_buffer[i]);
        clear_terminal(i);
        new_line_ready[i] = 0;
        set_hardware_cursor(i, 0, 0);
    }

    single_terminal = 0;

    restore_flags(flags);
}

// Keyboard syscalls

/*
 * terminal_open
 * Clears keyboard buffer. Clears terminal. Enables keyboard IRQ.
 * 
 * @param filename  For now, ignored.
 *
 * @returns         For now, just returns 0.
 */
int32_t terminal_open(file_t *f, const int8_t *filename) {
    uint8_t terminal_num = get_process_terminal();

    uint32_t flags;
    cli_and_save(flags);

    // Clear buffer and screen, then enable keyboard interrupts
    circular_buffer_init((circular_buffer_t*) &input_buffer[terminal_num], (void*) input_buffer_internal[terminal_num], KEYBOARD_BUFFER_SIZE);
    clear_terminal(terminal_num); // Rodney: a situation may arise where we don't want to clear the terminal.
    enable_irq(KEYBOARD_IRQ);

    restore_flags(flags);
    return 0;
}

/*
 * terminal_close
 * Clears keyboard buffer. Clears terminal. We don't disable IRQ since we will have 3 terminals in mp3.5
 * 
 * @param fd  For now, ignored.
 *
 * @returns   For now, just returns 0.
 */
int32_t terminal_close(file_t *f) {
    uint8_t terminal_num = get_process_terminal();

    uint32_t flags;
    cli_and_save(flags);

    circular_buffer_clear((circular_buffer_t*) &input_buffer[terminal_num]);
    //clear_terminal();

    restore_flags(flags);
    // disable_irq(KEYBOARD_IRQ);
    return 0;
}

/*
 * terminal_read
 * Reads (up to) nbytes into provided buffer
 * 
 * @param fd     For now, ignored.
 * @param buf    The buffer to write to.
 * @param nbytes The (maximum) number of bytes to write to provided buffer
 *
 * @returns      The number of bytes actually written.
 */
int32_t terminal_read(file_t *f, void *buf, int32_t nbytes) {
    uint8_t terminal_num = get_process_terminal();

    uint32_t retval;
    uint32_t max_len;
    uint32_t flags;

    // We're in a system call, so interrupts have been disabled.
    // We need to temporarily enable interrupts so that we can
    // actually fill the keyboard buffer.
    cli_and_save(flags);
    sti();

    // Wait for Enter key
    while (!new_line_ready[terminal_num]) {
        asm volatile("hlt");
    }

    restore_flags(flags);
    cli_and_save(flags);

    // Read up to min(nbytes, number of bytes available in buffered line)
    max_len = circular_buffer_find((circular_buffer_t*) &input_buffer[terminal_num], '\n') + 1;
    if(max_len < nbytes){
        nbytes = max_len;
    }
    retval = circular_buffer_get((circular_buffer_t*) &input_buffer[terminal_num], buf, nbytes);

    // We read one new line
    new_line_ready[terminal_num]--;
    
    restore_flags(flags);
    return retval;
}

/*
 * terminal_write
 * Writes nbytes from provided buffer to screen
 * 
 * @param fd     For now, ignored.
 * @param buf    This is where we get the bytes to write to the screen
 * @param nbytes The number of bytes to output to screen
 *
 * @returns      Always returns nbytes
 */
int32_t terminal_write(file_t *f, const void *buf, int32_t nbytes) {
    // TODO: something with the fd
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
