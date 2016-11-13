#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H

#include "ece391_fs.h"

int32_t file_open(const uint8_t *filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void *buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes);

int32_t read_file_by_name(const char *filename, void *buf, uint32_t nbytes);

int32_t dir_open(const uint8_t *filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes);

#endif
