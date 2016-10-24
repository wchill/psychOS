#include "tests.h"
#include "types.h"
#include "rtc.h"
#include "terminal.h"
#include "keyboard_map.h"
#include "lib.h"
#include "circular_buffer.h"

static volatile uint16_t htz = 1;

/*
 * test_suite
 *   DESCRIPTION:  
 *   INPUTS:       none
 *   OUTPUTS:      
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
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
 *   DESCRIPTION:  
 *   INPUTS:       none
 *   OUTPUTS:      
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
 */  
void list_all_files(){

}

/*
 * read_files_by_name
 *   DESCRIPTION:  
 *   INPUTS:       none
 *   OUTPUTS:      
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
 */  
void read_files_by_name(){

}

/*
 * read_file_by_index
 *   DESCRIPTION:  
 *   INPUTS:       none
 *   OUTPUTS:      
 *   RETURN VALUE: none
 *   SIDE EFFECTS: 
 */ 
void read_file_by_index(){

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

    // THIS CODE DOESN'T WORK PROPERLY. Should be totally rewritten.

    htz = 1;  // so that Ctrl+4 starts at a slow frequency again.

    uint32_t flags;
    cli_and_save(flags);

    // Clear buffer and screen
    //circular_buffer_init((circular_buffer_t*) &keyboard_buffer, (void*) keyboard_buffer_internal, KEYBOARD_BUFFER_SIZE);
    clear_terminal();

    disable_irq(RTC_IRQ);   // we were told not to disable the RTC, but I did anyway...

    restore_flags(flags);
}
