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

void ece391_fs_init(void *ptr);

int32_t read_dentry_by_name (const int8_t *fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t *dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);

int32_t get_file_size (uint32_t inode);

#endif
