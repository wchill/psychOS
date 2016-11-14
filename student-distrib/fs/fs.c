// Implements the bridge between the syscall interface and the underlying ECE391 filesystem driver.

#include <fs/fs.h>
#include <fs/ece391_fs.h>
#include <lib/lib.h>

// File syscalls

/*
 * file_open
 * No-op (already opened in syscall)
 * 
 * @param f 		the file struct for the file to open
 * 
 * @returns 0
 */
int32_t file_open(file_t *f, const int8_t *filename) {
	return 0;
}

/*
 * file_close
 * No-op (already closed in syscall)
 * 
 * @param f 		the file struct for the file to close
 * 
 * @returns 0
 */
int32_t file_close(file_t *f) {
	return 0;
}

/*
 * file_read
 * Reads nbytes from file represent by fd to provided buffer
 * 
 * @param f 		the file struct for the file to read from
 * @param buf     	the buffer to read nbytes into.
 * @param nbytes  	the number of bytes to read into the provided buffer.
 * 
 * @returns       number of bytes read (may be less than nbytes), or -1 for failure
 */
int32_t file_read(file_t *f, void *buf, int32_t nbytes) {
	int32_t res = read_data(f->inode, f->file_position, buf, nbytes);
	if(res < 0) return -1;

	f->file_position += res;

	return res;
}

/*
 * file_write
 * For now, does nothing and just returns -1.
 * 
 * @param f 		the file struct for the file to write to
 * @param buf     	the buffer to read nbytes from.
 * @param nbytes  	the number of bytes to write from buffer to file.
 * 
 * @returns       number of bytes written (may be less than nbytes), or -1 for failure
 */
int32_t file_write(file_t *f, const void *buf, int32_t nbytes) {
	// Not supported - read only
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
int32_t read_file_by_name(const char *filename, void *buf, uint32_t nbytes) {
    dentry_t dentry;
    int32_t res = read_dentry_by_name(filename, &dentry);

    if(res < 0) return -1;

    res = read_data(dentry.inode_num, 0, buf, nbytes);
    return res;
}


// Directory syscalls

/*
 * dir_open
 * No-op (already opened in syscall)
 * 
 * @param f 	the file struct for the directory to open
 * 
 * @returns 0
 */
int32_t dir_open(file_t *f, const int8_t *filename) {
	return 0;
}

/*
 * dir_close
 * No-op (already closed in syscall)
 * 
 * @param f 	the file struct for the directory to close
 * 
 * @returns 0
 */
int32_t dir_close(file_t *f) {
	return 0;
}

/*
 * dir_read
 * Reads the name of a file in the directory.
 * 
 * @param f 		the file struct for the directory to read from
 * @param buf     	the buffer to read nbytes into.
 * @param nbytes  	the number of bytes to read into the provided buffer.
 * 
 * @returns       number of bytes read (may be less than nbytes), or 0 if end is reached. -1 on failure
 */
int32_t dir_read(file_t *f, void *buf, int32_t nbytes) {
    dentry_t dentry;
    int32_t res = read_dentry_by_index(f->file_position, &dentry);
    if(res < 0) return 0;

    f->file_position++;

    int num_bytes_to_copy = MAX_FILE_NAME_LENGTH;
    if(num_bytes_to_copy > nbytes) num_bytes_to_copy = nbytes;
    memcpy(buf, dentry.file_name, num_bytes_to_copy);
    return num_bytes_to_copy;
}

/*
 * dir_write
 * For now, does nothing and just returns -1.
 * 
 * @param f 		the file struct for the directory to write to
 * @param buf     	the buffer to read nbytes from.
 * @param nbytes  	the number of bytes to write from buffer to file.
 * 
 * @returns       number of bytes written (may be less than nbytes), or -1 for failure
 */
int32_t dir_write(file_t *f, const void *buf, int32_t nbytes) {
	// Not supported - read only
	return -1;
}
