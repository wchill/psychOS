#include "tests.h"
#include "types.h"
#include "ece391_fs.h"
#include "rtc.h"
#include "i8259.h"
#include "terminal.h"
#include "keyboard_map.h"
#include "lib.h"
#include "circular_buffer.h"

static volatile uint16_t htz = 1;
static uint16_t index_num = 0;

/*
 * test_suite
 *   DESCRIPTION:  Dispatcher for test suite, called from interrupt handler.
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Dependent on test that is run
 */  
void test_suite(int test_num){
     if (test_num == 1) {
         list_all_files();
     }
     else if (test_num == 2) {
         read_files_by_name();
     }
     else if (test_num == 3) {
         read_file_by_index();
     }
     else if (test_num == 4) {
         start_rtc_test();
     }
     else if (test_num == 5) {
         stop_rtc_test();
     }
}

/*
 * list_all_files
 *   DESCRIPTION:  Reads in the name of each file on the filesystem, including the directory.
 *   INPUTS:       none
 *   OUTPUTS:      Prints a list of filenames, file types and file sizes.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */  
void list_all_files(){
    int i;
    int j;

    clear_terminal();

    // Iterate through the list of files listed in the boot block
    int32_t res = 0;
    while(res >= 0) {

        // Read in file entry
        dentry_t dentry;
        res = read_dentry_by_index(i, &dentry);
        if(res >= 0) {

            // Copy file name into C-style string
            char file_name[33];
            memset(file_name, 0, sizeof(file_name));
            memcpy(file_name, dentry.file_name, sizeof(dentry.file_name));

            printf(" name \"%s\"", file_name);

            // Padding
            for(j = 0; j < 50 - strlen(file_name); j++) {
                putc(' ');
            }

            int32_t file_size = get_file_size(dentry.inode_num);
            printf(" type %u - size %d\n", dentry.file_type, file_size);
            i++;
        }
    }
}

/*
 * read_files_by_name
 *   DESCRIPTION:  Reads in a file by filename and prints its contents to the screen.
 *   INPUTS:       none
 *   OUTPUTS:      Prints the contents of frame0.txt to the display.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */  
void read_files_by_name(){

    clear_terminal();

    // Read in file entry
    dentry_t dentry;
    int32_t res = read_dentry_by_name((uint8_t*) "frame0.txt", &dentry);
    int32_t num_read = 0;
    if(res >= 0) {
        res = 0;

        // Read and print 4k bytes from the file at a time
        uint8_t buf[4096];
        do {
            res = read_data(dentry.inode_num, num_read, buf, 4096);
            terminal_write(0, buf, res);
            num_read += res;
        } while(res > 0);
    }

    // Copy file name into C-style string
    char file_name[33];
    memset(file_name, 0, sizeof(file_name));
    memcpy(file_name, dentry.file_name, sizeof(dentry.file_name));

    int32_t file_size = get_file_size(dentry.inode_num);

    printf("\n--------------------------------------------------------------------------------"
        "file name: \"%s\"\nfile size: %d\n", file_name, file_size);
}

/*
 * read_file_by_index
 *   DESCRIPTION:  Reads in a file by directory entry index and prints its contents to the screen.
 *   INPUTS:       none
 *   OUTPUTS:      Prints the contents of the file at the current index to the display.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Increments the current file index.
 */ 
void read_file_by_index(){

    clear_terminal();

    dentry_t dentry;
    int32_t res;
    do {  

        // Read in file entry
        res = read_dentry_by_index(index_num, &dentry);
        int32_t num_read = 0;
        if(res >= 0) {
            res = 0;

            // Read and print 4k bytes from the file at a time
            uint8_t buf[4096];
            do {
                res = read_data(dentry.inode_num, num_read, buf, 4096);
                terminal_write(0, buf, res);
                num_read += res;
            } while(res > 0);
        } else {

            // Loop back to directory entry 1
            index_num = 0;
        }
    } while(res == -1);

    // Copy file name into C-style string
    char file_name[33];
    memset(file_name, 0, sizeof(file_name));
    memcpy(file_name, dentry.file_name, sizeof(dentry.file_name));
    
    int32_t file_size = get_file_size(dentry.inode_num);

    printf("\n--------------------------------------------------------------------------------"
        "file name: \"%s\"\nfile size: %d\n", file_name, file_size);
    printf("file index: %u\n", index_num++);
}

/*
 * start_rtc_test
 *   DESCRIPTION:  Tests RTC by printing 1's to the screen at various frequencies. 
 *                 "Enter" changes frequency.
 *   INPUTS:       none
 *   OUTPUTS:      Clear screen, print 1's, repeat.
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */ 
void start_rtc_test() {
    rtc_open();             // tests "rtc_open". If our RTC interrupts occur that means 'rtc_open' worked correctly
    terminal_open(0);       // Need keyboard for test, so we open it. PARAMETER MAY CAUSE ERRORS IN 3.3, 3.4, 3.5 

    /* Update htz */
    htz <<= 1;              // doubles value. htz must be power of 2
    if (htz > MAX_FREQ)
        htz = MIN_FREQ;

    rtc_write(htz);         // tests "rtc_write".
}

/*
 * stop_rtc_test
 *   DESCRIPTION:  Stops the RTC test
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: Erases the screen
 */ 
void stop_rtc_test(){
    htz = 1;  // so that Ctrl+4 starts at a slow frequency again.

    uint32_t flags;
    cli_and_save(flags);

    // Clear buffer and screen
    terminal_close(0);

    disable_irq(RTC_IRQ);   // we were told not to disable the RTC, but I did anyway...

    restore_flags(flags);
}
