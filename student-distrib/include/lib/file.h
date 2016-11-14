#ifndef _FILE_T_H
#define _FILE_T_H

#include <types.h>

#define FILE_IN_USE 1

typedef struct file_t file_t;

typedef struct file_ops {
    int32_t (* open) (file_t *f, const int8_t * filename);
    int32_t (* read) (file_t *f, void * buf, int32_t nbytes);
    int32_t (* write) (file_t *f, const void * buf, int32_t nbytes);
    int32_t (* close) (file_t *f);
} file_ops;

typedef struct file_t {
    file_ops    fops;          // a pointer to methods that we can use to manipulate file data (open, close, read, write)
    uint32_t    file_position; // a pointer within file. Will tell us where to read/write within the file.
    uint32_t    flags;         // in our case it's not for synchronization. It will be used to indicate if file descriptor is busy or free
    uint32_t    inode;         // a number that indicates which file we are talking about.
} file_t;

#endif
