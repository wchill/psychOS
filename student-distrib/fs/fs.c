// Implements the bridge between the syscall interface and the underlying ECE391 filesystem driver.

#include <fs/fs.h>
#include <fs/ece391_fs.h>
#include <lib/lib.h>

// File syscalls

/*
 * file_open
 * For now, just finds the file represents by filename but does nothing with it.
 * 
 * @param filename   the name of the file to open
 * 
 * @returns For now, returns -1. Will eventually return file descriptor (FD)
 */
int32_t file_open(const uint8_t *filename) {
	// TODO: Return file FD
	dentry_t dentry;
	int32_t res = read_dentry_by_name(filename, &dentry);

	if(res < 0) return -1;

	return -1;
}

/*
 * file_close
 * For now, does nothing and just returns -1.
 * 
 * @param fd   the file descriptor of the file to close
 * 
 * @returns For now, returns -1.
 */
int32_t file_close(int32_t fd) {
	return -1;
}

/*
 * file_read
 * Reads nbytes from file represent by fd to provided buffer
 * 
 * @param fd      the file descriptor of the file to read.
 * @param buf     the buffer to read nbytes into.
 * @param nbytes  the number of bytes to read into the provided buffer.
 * 
 * @returns       number of bytes read (may be less than nbytes), or -1 for failure
 */
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

/*
 * file_write
 * For now, does nothing and just returns -1.
 * 
 * @param fd      the file descriptor of the file to write to.
 * @param buf     the buffer to read nbytes from.
 * @param nbytes  the number of bytes to write from buffer to file.
 * 
 * @returns       number of bytes written (may be less than nbytes), or -1 for failure
 */
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}


/*
 * read_file_by_name
 * Reads nbytes from a file into a buffer.
 * 
 * @param filename  Used to find the file in our file system
 * @param buf       the buffer to read nbytes into.
 * @param nbytes    the number of bytes to read into the provided buffer.
 * 
 * @returns         number of bytes read (may be less than nbytes), or -1 for failure
 */
int32_t read_file_by_name(char *filename, void *buf, uint32_t nbytes) {
    dentry_t dentry;
    int32_t res = read_dentry_by_name((uint8_t*) filename, &dentry);

    if(res < 0) return -1;

    res = read_data(dentry.inode_num, 0, buf, nbytes);
    return res;
}


// Directory syscalls

/*
 * dir_open
 * For now, just returns -1.
 * 
 * @param filename   the name of the directory entry to open
 * 
 * @returns For now, returns -1. Will eventually return file descriptor (FD)
 */
int32_t dir_open(const uint8_t *filename) {
	// TODO: Return directory FD
	return -1;
}

/*
 * dir_close
 * For now, does nothing and just returns -1.
 * 
 * @param fd   the file descriptor of the directory to close
 * 
 * @returns For now, returns -1.
 */
int32_t dir_close(int32_t fd) {
	return -1;
}

/*
 * dir_read
 * For now, reads the data at index 0. Will eventually use file descriptor.
 * 
 * @param fd      the file descriptor of the directory to read.
 * @param buf     the buffer to read nbytes into.
 * @param nbytes  the number of bytes to read into the provided buffer.
 * 
 * @returns       number of bytes read (may be less than nbytes), or -1 for failure
 */
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {
	// TODO: Tie the index to the file descriptor
	static int index_num = 0;

    dentry_t dentry;
    int32_t res = read_dentry_by_index(index_num, &dentry);
    if(res < 0) return -1;

    index_num++;

    int num_bytes_to_copy = MAX_FILE_NAME_LENGTH;
    if(num_bytes_to_copy > nbytes) num_bytes_to_copy = nbytes;
    memcpy(buf, dentry.file_name, num_bytes_to_copy);
    return num_bytes_to_copy;
}

/*
 * dir_write
 * For now, does nothing and just returns -1.
 * 
 * @param fd      the file descriptor of the file to write to.
 * @param buf     the buffer to read nbytes from.
 * @param nbytes  the number of bytes to write from buffer to file.
 * 
 * @returns       number of bytes written (may be less than nbytes), or -1 for failure
 */
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes) {
	return -1;
}
