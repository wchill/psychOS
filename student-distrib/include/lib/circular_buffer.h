#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include "types.h"

typedef struct circular_buffer_t {
	uint32_t max_len;
	uint32_t current_len;
	uint8_t *data_start;
	uint8_t *data_end;
	uint8_t *head;
	uint8_t *tail;
} circular_buffer_t;

void circular_buffer_init(circular_buffer_t *buf, void *data_buf, uint32_t max_len);
void circular_buffer_clear(circular_buffer_t *buf);

uint32_t circular_buffer_put(circular_buffer_t *buf, void *input_buf, uint32_t len);
uint32_t circular_buffer_get(circular_buffer_t *buf, void *output_buf, uint32_t len);
uint32_t circular_buffer_peek(circular_buffer_t *buf, void *output_buf, uint32_t len);

uint32_t circular_buffer_put_byte(circular_buffer_t *buf, uint8_t b);
uint32_t circular_buffer_get_byte(circular_buffer_t *buf, uint8_t *b);

uint32_t circular_buffer_peek_end_byte(circular_buffer_t *buf, uint8_t *b);
uint32_t circular_buffer_remove_end_byte(circular_buffer_t *buf);

int32_t circular_buffer_find(circular_buffer_t *buf, uint8_t val);
uint32_t circular_buffer_len(circular_buffer_t *buf);

#endif
