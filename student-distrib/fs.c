// Implements the bridge between the syscall interface and the underlying ECE391 filesystem driver.

#include "fs.h"

// File syscalls

/*
int32_t read_dentry_by_name (const uint8_t *fname, dentry_t* dentry);
int32_t read_dentry_by_index (uint32_t index, dentry_t *dentry);
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);
*/

int32_t file_open(const uint8_t *filename) {
	dentry_t dentry;
	int32_t res = read_dentry_by_name(filename, &dentry);

	if(res < 0) return -1;

	return -1;
}

int32_t file_close(int32_t fd) {
	return -1;
}

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {
	return -1;
}

int32_t file_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}


// Directory syscalls

int32_t dir_open(const uint8_t *filename) {
	return -1;
}

int32_t dir_close(int32_t fd) {
	return -1;
}

int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {
	return -1;
}

int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}