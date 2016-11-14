#include <lib/circular_buffer.h>
#include <lib/lib.h>

/*
 * circular_buffer_init
 * Initialize the circular buffer and use the given backing memory.
 * 
 * @param buf       The circular buffer to initialize
 * @param data_buf  A contiguous block of memory that we will use circularly.
 * @param max_len   Maximum length of circular buffer
 */
void circular_buffer_init(circular_buffer_t *buf, void *data_buf, uint32_t max_len) {
    buf->max_len     = max_len;
    buf->current_len = 0;
    buf->data_start  = data_buf;
    buf->data_end    = data_buf + max_len;
    buf->head        = data_buf;
    buf->tail        = data_buf;
}

/*
 * circular_buffer_clear
 * Clear the circular buffer.
 * 
 * @param buf       The circular buffer to clear
 */
void circular_buffer_clear(circular_buffer_t *buf) {
    buf->current_len = 0;           // no need to actually delete the data, just set length to 0
    buf->head        = buf->data_start;
    buf->tail        = buf->data_start;
}

/*
 * circular_buffer_put
 * Copy data from input buffer into circular buffer, up to len bytes (may be less). Circular buffer grows.
 * 
 * @param buf       The circular buffer to copy data to
 * @param input_buf The linear buffer to copy data from
 * @param len       The number of bytes to (try to) copy
 *
 * @returns         number of bytes copied (may be less than len)
 */
uint32_t circular_buffer_put(circular_buffer_t *buf, void *input_buf, uint32_t len) {
    int i;
    uint8_t *in = (uint8_t*) input_buf;

    // Cap the # of bytes to add
    if(buf->current_len + len > buf->max_len) {
        len = buf->max_len - buf->current_len;
    }

    // Add byte and then increment tail pointer len times, wrapping around when necessary
    for(i = 0; i < len; i++) {
        *(buf->tail++) = *(in++);
        if(buf->tail >= buf->data_end) {
            buf->tail = buf->data_start;
        }
    }

    buf->current_len += len;
    return len;
}

/*
 * circular_buffer_get
 * Copy data out of circular buffer into output buffer, up to len bytes (may be less). Circular buffer shrinks.
 * 
 * @param buf       The circular buffer to copy data from
 * @param input_buf The linear buffer to copy data to
 * @param len       The number of bytes to (try to) copy
 *
 * @returns         number of bytes copied (may be less than len)
 */
uint32_t circular_buffer_get(circular_buffer_t *buf, void *output_buf, uint32_t len) {
    int i;
    uint8_t *out = (uint8_t*) output_buf;

    // Cap the # of bytes to remove
    if (len > buf->current_len) {
        len = buf->current_len;
    }

    // Remove byte and then increment head pointer len times, wrapping around when necessary
    for(i = 0; i < len; i++) {
        *(out++) = *(buf->head++);
        if(buf->head >= buf->data_end) {
            buf->head = buf->data_start;
        }
    }

    buf->current_len -= len;
    return len;
}

/*
 * circular_buffer_peek
 * Copy data out of circular buffer without modifying buffer, up to len bytes (may be less)
 * 
 * @param buf       The circular buffer to copy data from
 * @param input_buf The linear buffer to copy data to
 * @param len       The number of bytes to (try to) copy
 *
 * @returns         number of bytes copied (may be less than len)
 */
uint32_t circular_buffer_peek(circular_buffer_t *buf, void *output_buf, uint32_t len) {
    int i;
    uint8_t *out = (uint8_t*) output_buf;
    uint8_t *addr = buf->head;

    // Cap the # of bytes to copy
    if (len > buf->current_len) {
        len = buf->current_len;
    }

    // Copy byte and then increment temporary head pointer len times, wrapping around when necessary
    for(i = 0; i < len; i++) {
        *(out++) = *(addr++);
        if(addr >= buf->data_end) {
            addr = buf->data_start;
        }
    }

    return len;
}

/*
 * circular_buffer_put_byte
 * Copy 1 byte into circular buffer (may not copy if full).
 * 
 * @param buf       The circular buffer to copy data to
 * @param b         The byte to copy into circular buffer
 *
 * @returns         Number of bytes we successfully "put" (either 0 or 1)
 */
uint32_t circular_buffer_put_byte(circular_buffer_t *buf, uint8_t b) {
    if(buf->current_len >= buf->max_len) return 0;

    // Copy byte, increment pointer, wraparound
    *(buf->tail) = b;
    buf->tail++;
    if(buf->tail >= buf->data_end) {
        buf->tail = buf->data_start;
    }

    buf->current_len++;
    return 1;
}

/*
 * circular_buffer_get_byte
 * Copy 1 byte out of circular buffer (may not copy if empty). Circular buffer shrinks by 1 byte (if successful)
 * 
 * @param buf       The circular buffer to get data from
 * @param b         A pointer to copy the byte to.
 *
 * @returns         Number of bytes we successfully got (either 0 or 1)
 */
uint32_t circular_buffer_get_byte(circular_buffer_t *buf, uint8_t *b) {
    if(buf->current_len == 0) return 0;
    
    // Copy byte, increment pointer, wraparound
    *b = *(buf->head);
    buf->head++;
    if(buf->head >= buf->data_end) {
        buf->head = buf->data_start;
    }

    buf->current_len--;
    return 1;
}

/*
 * circular_buffer_peek_end_byte
 * Examine last byte of circular buffer without affecting data (may fail if empty).
 * 
 * @param buf       The circular buffer to peek at 1 byte from
 * @param b         A pointer to copy the byte to.
 *
 * @returns         Number of bytes we successfully got (either 0 or 1)
 */
uint32_t circular_buffer_peek_end_byte(circular_buffer_t *buf, uint8_t *b) {
    if(buf->current_len == 0) return 0;

    uint8_t *addr = buf->tail - 1;

    if(addr < buf->data_start) {
        addr = buf->data_end - 1;
    }

    *b = *addr;
    return 1;
}

/*
 * circular_buffer_remove_end_byte
 * Remove last byte of circular buffer (may fail if empty)
 * 
 * @param buf       The circular buffer to remove 1 byte from.
 *
 * @returns         Number of bytes we successfully removed (either 0 or 1)
 */
uint32_t circular_buffer_remove_end_byte(circular_buffer_t *buf) {
    if(buf->current_len == 0) return 0;

    // Remove byte, increment pointer, wraparound
    buf->tail--;
    if(buf->tail < buf->data_start) {
        buf->tail = buf->data_end - 1;
    }

    buf->current_len--;
    return 1;
}

/*
 * circular_buffer_find
 * Find and return the number of characters before the given value is encountered, or return -1 if not found.
 * 
 * @param buf       The circular buffer to search for the value.
 * @param val       The value to search circular buffer for.
 *
 * @returns         The number of characters before the given value is encountered, or -1 if not found.
 */
int32_t circular_buffer_find(circular_buffer_t *buf, uint8_t val) {
    int32_t len = 0;
    uint8_t *addr = buf->head;

    // Basically a strlen but with pointer wraparound
    for(; len < buf->current_len; len++) {
        if(*addr == val) {
            return len;
        }
        addr++;
        if(addr >= buf->data_end) {
            addr = buf->data_start;
        }
    }
    return -1;
}

/*
 * circular_buffer_len
 * Return current length of circular buffer.
 * 
 * @param buf       The circular buffer to get the size of
 *
 * @returns         Size (in bytes) of circular buffer.
 */
inline uint32_t circular_buffer_len(circular_buffer_t *buf) {
    return buf->current_len;
}
