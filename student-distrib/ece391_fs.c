// Filesystem driver for the ECE391 filesystem.

#include "ece391_fs.h"
#include "lib.h"

static boot_block_t *fs_boot_ptr;
static inode_block_t *fs_inode_ptr;
static data_block_t *fs_data_ptr;

void ece391_fs_init(void *ptr) {
	fs_boot_ptr = (boot_block_t*) ptr;
	fs_inode_ptr = (inode_block_t*) fs_boot_ptr + 1;
	fs_data_ptr = (data_block_t*) fs_inode_ptr + fs_boot_ptr->num_inodes;
}

int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
	uint32_t i;
	for(i = 0; i < fs_boot_ptr->num_directory_entries; i++) {
		dentry_t *dir_entry = &(fs_boot_ptr->directory_entries[i]);
		if(!strncmp((const char*) fname, (const char*) dir_entry->file_name, 32)) {
			strncpy(dentry->file_name, dir_entry->file_name, 32);
			dentry->file_type = dir_entry->file_type;
			dentry->inode_num = dir_entry->inode_num;
			memcpy(dentry->reserved, dir_entry->reserved, 24);
			return 0;
		}
	}
	return -1;
}

int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {
	if (index >= fs_boot_ptr->num_directory_entries) return -1;

	dentry_t *dir_entry = &(fs_boot_ptr->directory_entries[index]);
	strncpy(dentry->file_name, dir_entry->file_name, 32);
	dentry->file_type = dir_entry->file_type;
	dentry->inode_num = dir_entry->inode_num;
	memcpy(dentry->reserved, dir_entry->reserved, 24);
	return 0;
}

static int32_t read_block(uint32_t block_index, uint32_t offset, uint8_t *buf, uint32_t length) {
	if(block_index >= fs_boot_ptr->num_data_blocks) return -1;

	data_block_t *data_block = &fs_data_ptr[block_index];
	memcpy(buf, &(data_block->data[offset]), length);
	return length;
}

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length) {
	if (inode >= fs_boot_ptr->num_inodes) return -1;

	inode_block_t *inode_block = &fs_inode_ptr[inode];
	if (offset >= inode_block->file_length) return 0;

	if (length + offset > inode_block->file_length) {
		length = inode_block->file_length - offset;
	}

	// Find which block to start reading from
	// Right shifting 12 bits == dividing by 4096 which is the block size
	int block_num = offset >> 12;
	uint32_t file_pos = offset;
	uint32_t block_pos = offset & 4095;

	uint32_t bytes_written = 0;

	while(bytes_written < length && file_pos < inode_block->file_length) {
		int block_index = inode_block->data_blocks[block_num];

		// Two edge cases to handle:
		// 1. The beginning offset
		// 2. The ending portion

		uint32_t num_bytes_to_copy = 4096 - block_pos;
		if(num_bytes_to_copy > length - bytes_written) {
			num_bytes_to_copy = length - bytes_written;
		}

		int32_t res = read_block(block_index, block_pos, &buf[bytes_written], num_bytes_to_copy);
		if(res < 0) return -1;

		bytes_written += res;
		file_pos += res;
		block_pos += res;

		if(block_pos >= 4096) {
			block_pos -= 4096;
			block_num++;
		}
	}

	return bytes_written;
}
