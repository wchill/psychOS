#ifndef _ECE391_FS_H
#define _ECE391_FS_H

#include "types.h"

#define MAX_FILE_NAME_LENGTH 32
#define FS_BLOCK_SIZE (1 << 12)

/* Directory Entry */
typedef struct dentry_t {
	char file_name[MAX_FILE_NAME_LENGTH];
	uint32_t file_type;
	uint32_t inode_num;
	uint8_t reserved[24]; 			// Rodney: 24 represents # of reserved bytes
} __attribute__((packed)) dentry_t;

/* Boot Block */
typedef struct boot_block_t {
	uint32_t num_directory_entries;
	uint32_t num_inodes;
	uint32_t num_data_blocks;
	uint8_t reserved[52]; 			// Rodney: 52 represents # of reserved bytes
	dentry_t directory_entries[63]; // Rodney: 63 represents number of directory entries
} __attribute__((packed)) boot_block_t;

/* Inode Block */
typedef struct inode_block_t {
	uint32_t file_length;
	uint32_t data_blocks[1023];	 	// Rodney: 1023 represents number of data blocks.
} __attribute__((packed)) inode_block_t;

/* Data Block */
typedef struct data_block_t {
	uint8_t data[FS_BLOCK_SIZE];
} data_block_t;

/*
typedef struct file {
    file_ops * fops;          // a pointer to methods that we can use to manipulate file data (open, close, read, write)
    int32_t    file_position; // a pointer within file. Will tell us where to read/write within the file.
    int32_t    flags;         // in our case it's not for synchronization. It will be used to indicate if file descriptor is busy or free
    int32_t    inode;         // a number that indicates which file we are talking about.
} file;

typedef struct file_operations {
    int32_t (* open) (const uint8_t * filename);
    int32_t (* read) (int32_t fd, void * buf, int32_t nbytes);
    int32_t (* write) (int32_t fd, const void * buf, int32_t nbytes);
    int32_t (* close) (int32_t fd);
} file_operations;
*/

void ece391_fs_init(void *ptr);

int32_t read_dentry_by_name (const uint8_t *fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t *dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);

int32_t get_file_size (uint32_t inode);

#endif
