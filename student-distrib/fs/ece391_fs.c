// Filesystem driver for the ECE391 filesystem.

#include <fs/ece391_fs.h>
#include <lib/lib.h>

static boot_block_t *fs_boot_ptr;
static inode_block_t *fs_inode_ptr;
static data_block_t *fs_data_ptr;

/*
 * ece391_fs_init
 *   DESCRIPTION:  Initialize the file system (just keeping track of various pointers)
 *   INPUTS:       ptr - A pointer to our file system (where 1st entry is boot block)
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Initializes static variables for File System
 */ 
void ece391_fs_init(void *ptr) {
	fs_boot_ptr = (boot_block_t*) ptr;
	fs_inode_ptr = (inode_block_t*) fs_boot_ptr + 1;
	fs_data_ptr = (data_block_t*) fs_inode_ptr + fs_boot_ptr->num_inodes;
}

/*
 * read_dentry_by_name
 *   DESCRIPTION:  Read the contents of a directory entry based on file name.
 *   INPUTS:       fname  - The filename that we are searching for.
                   dentry - The directory entry that we will save data to.
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *				    0 on success
 *   SIDE EFFECTS: Fills dentry with data
 */ 
int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
	uint32_t i;

	// Iterate through every directory entry
	for(i = 0; i < fs_boot_ptr->num_directory_entries; i++) {
		dentry_t *dir_entry = &(fs_boot_ptr->directory_entries[i]);

		// Does the file name match?
		if(!strncmp((const char*) fname, (const char*) dir_entry->file_name, MAX_FILE_NAME_LENGTH)) {
			strncpy(dentry->file_name, dir_entry->file_name, MAX_FILE_NAME_LENGTH);
			dentry->file_type = dir_entry->file_type;
			dentry->inode_num = dir_entry->inode_num;
			memcpy(dentry->reserved, dir_entry->reserved, sizeof(dir_entry->reserved));
			return 0;
		}
	}

	// Not found
	return -1;
}

/*
 * read_dentry_by_index
 *   DESCRIPTION:  Read the contents of a directory entry based on entry index.
 *   INPUTS:       index  - The index (in the directory entries in boot block).
                   dentry - The directory entry that we will save data to.
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *				    0 on success
 *   SIDE EFFECTS: Fills dentry with data
 */ 
int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {
	// Check valid directory entry index
	if (index >= fs_boot_ptr->num_directory_entries) return -1;

	// Copy fields into the given dentry_t
	dentry_t *dir_entry = &(fs_boot_ptr->directory_entries[index]);
	strncpy(dentry->file_name, dir_entry->file_name, MAX_FILE_NAME_LENGTH);
	dentry->file_type = dir_entry->file_type;
	dentry->inode_num = dir_entry->inode_num;
	memcpy(dentry->reserved, dir_entry->reserved, sizeof(dir_entry->reserved));

	return 0;
}

/*
 * read_block
 * Read the contents of a FS block into a buffer.
 * 
 * @param block_index The index of the block in the filesystem to read from
 * @param offset      number of bytes into the block to start reading from.
 * @param buf         pointer to buffer to copy data to.
 * @param length      max number of bytes to copy into buffer.
 * 
 * @returns The number of bytes written, or -1 if an error occurred.
 */
static int32_t read_block(uint32_t block_index, uint32_t offset, uint8_t *buf, uint32_t length) {
	// Check valid block index
	if(block_index >= fs_boot_ptr->num_data_blocks) return -1;

	// Check for valid offset
	if(offset >= FS_BLOCK_SIZE) return -1;

	// Cap the length to not go beyond the end of the block
	if(length > FS_BLOCK_SIZE - offset) {
		length = FS_BLOCK_SIZE - offset;
	}

	// Copy from data block into buffer
	data_block_t *data_block = &fs_data_ptr[block_index];
	memcpy(buf, &(data_block->data[offset]), length);
	return length;
}

/*
 * read_data
 * Given an inode, read the contents of a file into a buffer.
 * 
 * @param inode   Represents which file we want to read from
 * @param offset  number of bytes into the file to start reading from.
 * @param buf     pointer to buffer to copy data to.
 * @param length  max number of bytes to copy into buffer.
 * 
 * @returns The number of bytes written, or -1 if an error occurred.
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length) {
	// Check valid inode
	if (inode >= fs_boot_ptr->num_inodes) return -1;

	// Check for EOF
	inode_block_t *inode_block = &fs_inode_ptr[inode];
	if (offset >= inode_block->file_length) return 0;

	// Cap # of bytes to copy so it doesn't go past EOF
	if (length + offset > inode_block->file_length) {
		length = inode_block->file_length - offset;
	}

	// Find which block to start reading from
	// Right shifting 12 bits == dividing by 4096 which is the block size
	int block_num = offset >> 12;
	uint32_t file_pos = offset;
	uint32_t block_pos = offset & (FS_BLOCK_SIZE - 1);

	uint32_t bytes_written = 0;

	while(bytes_written < length && file_pos < inode_block->file_length) {
		int block_index = inode_block->data_blocks[block_num];

		// Copy min(# of remaining bytes in block, # of remaining bytes in buffer)
		uint32_t num_bytes_to_copy = FS_BLOCK_SIZE - block_pos;
		if(num_bytes_to_copy > length - bytes_written) {
			num_bytes_to_copy = length - bytes_written;
		}

		int32_t res = read_block(block_index, block_pos, &buf[bytes_written], num_bytes_to_copy);
		if(res < 0) return -1;

		// Update our file position indices
		bytes_written += res;
		file_pos += res;
		block_pos += res;

		// Update block num
		if(block_pos >= FS_BLOCK_SIZE) {
			block_pos -= FS_BLOCK_SIZE;
			block_num++;
		}
	}

	return bytes_written;
}

/*
 * get_file_size
 * Given an inode, return the file size of a file.
 * 
 * @param inode   Represents the file we want to get the size (in bytes of)
 * 
 * @returns The file size in bytes, or -1 if an error occurred.
 */
int32_t get_file_size(uint32_t inode) {
	if (inode >= fs_boot_ptr->num_inodes) return -1;

	inode_block_t *inode_block = &fs_inode_ptr[inode];
	return inode_block->file_length;
}
