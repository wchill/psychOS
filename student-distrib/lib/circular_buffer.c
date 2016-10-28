#include <lib/circular_buffer.h>
#include <lib/lib.h>

// Initialize the circular buffer and use the given backing memory.
void circular_buffer_init(circular_buffer_t *buf, void *data_buf, uint32_t max_len) {
	buf->max_len = max_len;
	buf->current_len = 0;
	buf->data = data_buf;
	buf->data_end = data_buf + max_len;
	buf->head = data_buf;
	buf->tail = data_buf;
}

// Clear the circular buffer.
void circular_buffer_clear(circular_buffer_t *buf) {
	buf->current_len = 0;
	buf->head = buf->data;
	buf->tail = buf->data;
}

// Copy data from input buffer into circular buffer, up to len bytes (may be less).
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
		if(buf->tail >= buf->data_end) buf->tail = buf->data;
	}

	buf->current_len += len;
	return len;
}

// Copy data out of circular buffer into output buffer, up to len bytes (may be less).
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
		if(buf->head >= buf->data_end) buf->head = buf->data;
	}

	buf->current_len -= len;
	return len;
}

// Copy data out of circular buffer without modifying buffer, up to len bytes (may be less)
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
		if(addr >= buf->data_end) addr = buf->data;
	}

	return len;
}

// Copy 1 byte into circular buffer (may not copy if full).
uint32_t circular_buffer_put_byte(circular_buffer_t *buf, uint8_t b) {
	if(buf->current_len >= buf->max_len) return 0;

	// Copy byte, increment pointer, wraparound
	*(buf->tail) = b;
	buf->tail++;
	if(buf->tail >= buf->data_end) buf->tail = buf->data;

	buf->current_len++;
	return 1;
}

// Copy 1 byte out of circular buffer (may not copy if empty).
uint32_t circular_buffer_get_byte(circular_buffer_t *buf, uint8_t *b) {
	if(buf->current_len == 0) return 0;
	
	// Copy byte, increment pointer, wraparound
	*b = *(buf->head);
	buf->head++;
	if(buf->head >= buf->data_end) buf->head = buf->data;

	buf->current_len--;
	return 1;
}

// Examine last byte of circular buffer without affecting data (may fail if empty).
uint32_t circular_buffer_peek_end_byte(circular_buffer_t *buf, uint8_t *b) {
	if(buf->current_len == 0) return 0;

	uint8_t *addr = buf->tail - 1;

	if(addr < buf->data) addr = buf->data_end - 1;

	*b = *addr;
	return 1;
}

// Remove last byte of circular buffer (may fail if empty).
uint32_t circular_buffer_remove_end_byte(circular_buffer_t *buf) {
	if(buf->current_len == 0) return 0;

	// Remove byte, increment pointer, wraparound
	buf->tail--;
	if(buf->tail < buf->data) buf->tail = buf->data;

	buf->current_len--;
	return 1;
}

// Find and return the number of characters before the given value is encountered, or return -1 if not found.
int32_t circular_buffer_find(circular_buffer_t *buf, uint8_t val) {
	int32_t len = 0;

	// Basically a strlen but with pointer wraparound
	for(; len < buf->current_len; len++) {
		uint8_t *addr = &buf->head[len];
		if(addr >= buf->data_end) {
			addr = buf->data + (buf->head + len - buf->data_end);
		}
		if(*addr == val) {
			return len;
		}
	}
	return -1;
}

// Return current length of circular buffer.
inline uint32_t circular_buffer_len(circular_buffer_t *buf) {
	return buf->current_len;
}
