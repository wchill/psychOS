// Implements the bridge between the syscall interface and the underlying ECE391 filesystem driver.

#include <fs/fs.h>
#include <fs/ece391_fs.h>
#include <lib.h>

// File syscalls

/*
int32_t read_dentry_by_name (const uint8_t *fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t *dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);
*/

int32_t file_open(const uint8_t *filename) {
	// TODO: Return file FD
	dentry_t dentry;
	int32_t res = read_dentry_by_name(filename, &dentry);

	if(res < 0) return -1;

	return -1;
}

int32_t file_close(int32_t fd) {
	return -1;
}

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {
	// TODO: Handle file descriptor

	// TODO: Translate FD to inode
	uint32_t inode = 1;
	static int num_read = 0;

	int32_t res = read_data(inode, num_read, buf, nbytes);
	if(res < 0) return -1;
	if(res == 0) return 0;

	num_read += res;

	return res;
}

int32_t file_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}

// Test code for Checkpoint 3.2: to be removed later
int32_t read_file_by_name(char *filename, void *buf, uint32_t nbytes) {
    dentry_t dentry;
    int32_t res = read_dentry_by_name((uint8_t*) filename, &dentry);

    if(res < 0) return -1;

    res = read_data(dentry.inode_num, 0, buf, nbytes);
    return res;
}


// Directory syscalls

int32_t dir_open(const uint8_t *filename) {
	// TODO: Return directory FD
	return -1;
}

int32_t dir_close(int32_t fd) {
	return -1;
}

int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {
	// TODO: Tie the index to the file descriptor
	static int index_num = 0;

    dentry_t dentry;
    int32_t res = read_dentry_by_index(index_num, &dentry);
    if(res < 0) return -1;

    index_num++;

    int num_bytes_to_copy = 32;    // 32 is number of bytes to copy
    if(num_bytes_to_copy > nbytes) num_bytes_to_copy = nbytes;
    memcpy(buf, dentry.file_name, num_bytes_to_copy);
    return num_bytes_to_copy;
}

int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}
