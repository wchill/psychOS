#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include <lib/file.h>

int32_t file_open(file_t *f, const int8_t *filename);
int32_t file_close(file_t *f);
int32_t file_read(file_t *f, void *buf, int32_t nbytes);
int32_t file_write(file_t *f, const void *buf, int32_t nbytes);

int32_t read_file_by_name(const char *filename, void *buf, uint32_t nbytes);

int32_t dir_open(file_t *f, const int8_t *filename);
int32_t dir_close(file_t *f);
int32_t dir_read(file_t *f, void *buf, int32_t nbytes);
int32_t dir_write(file_t *f, const void *buf, int32_t nbytes);

#endif
