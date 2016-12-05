#include <drivers/rtc.h>
#include <lib/lib.h>
#include <arch/x86/i8259.h>
#include <arch/x86/io.h>
#include <arch/x86/x86_desc.h>
#include <arch/x86/interrupt.h>
#include <arch/x86/task.h>
#include <types.h>

/* References:
    http://wiki.osdev.org/RTC#Programming_the_RTC                            (this is the main reference we used)
    https://piazza.com/class/iqsg6pdvods1rw?cid=596                          
    https://courses.engr.illinois.edu/ece391/secure/references/mc146818.pdf  (we didn't really use this, but it is the most detailed reference)
*/

static volatile int rtc_test_enabled = 0;

/**
* void test_rtc(void)
*   Inputs: void
*   Return Value: void
*   Function: increment video memory (just the top-right of the screen), to show RTC works.
*/
static void
test_rtc(void)
{
    putc('1');
}


/**
 * set_rtc_test_enabled
 *   DESCRIPTION:  set whether RTC test is running.
 *   INPUTS:       enabled - 1 if test is running, else 0
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: enables RTC test
 */  
void set_rtc_test_enabled(int enabled) {
    uint32_t flags;

    cli_and_save(flags);
    rtc_test_enabled = enabled;
    restore_flags(flags);
}

/**
 * rtc_handler
 *   DESCRIPTION:  this code runs when RTC Interrupt happens. Makes screen flash
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: makes screen flash when it tests interrupts.
 */  
void rtc_handler() {
    uint32_t flags;

    cli_and_save(flags);

    outportb(RTC_STATUS_PORT, RTC_REG_C);
    inportb(RTC_DATA_PORT);

    //test_interrupts(); // This causes screen to flash. Can comment it out to stop flashing.
    if(rtc_test_enabled) {
        test_rtc();     
    }

    //rtc_tick_flag = 0; // Rodney: newly added for 3.2

    /* Update number of ticks left before interrupt fires for each process */
    int i;
    for(i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *pcb = get_pcb_from_slot(i);
        if(pcb->in_use && pcb->rtc_enabled && pcb->remaining_rtc_ticks > 0) {
            pcb->remaining_rtc_ticks--;
        }
    }
    
    send_eoi(RTC_IRQ);
    restore_flags(flags);
}

/**
 * rtc_init
 * Initializs the RTC and sets interrupt rate.
 */
void rtc_init() {
    int     htz = MAX_FREQ;       // hertz
    uint8_t rate = 15;            // rate passed to the RTC. Rate of 15 corresponds to 2 Hz. We use formula: htz = 32768 >> (rate - 1)
    char    prev;                 // just a temporary variable

    // Calculate appropriate 'rate' value for the htz we want
    while (htz != MIN_FREQ){
        htz >>= 1;                          // divides by 2
        rate--;
    }

    // set the interrupt rate
    outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_A);
    prev = inportb(RTC_DATA_PORT);                              // get initial value of register A
    outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_A);     // reset index to A
    outportb(RTC_DATA_PORT, (prev & 0xF0) | rate);              // 0xF0 is bitmask for bits 4-7
}

/**
 * rtc_open
 *   DESCRIPTION:  Initializes the RTC with a frequency of 2 Hz
 *   INPUTS:       f - file struct representing an RTC
 *                 filename - name of RTC file
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *                  0 on success (for now we are always succesful)
 *   SIDE EFFECTS: changes the frequency of the RTC to 2 Hz, installs a handler
 */ 
int32_t rtc_open(file_t *f, const int8_t * filename) {
    outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_B);     // select register B, and disable NMI
    char prev = inportb(RTC_DATA_PORT);                         // read the current value of register B
    outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_B);     // set the index again (a read will reset the index to register D)
    outportb(RTC_DATA_PORT, prev | RTC_ENABLE_INTERRUPTS);      // write the previous value ORed with 0x40. This turns on bit 6 of register B

    // Need to initialize RTC before enabling IRQ
    install_interrupt_handler(IRQ_INT_NUM(RTC_IRQ), rtc_handler_wrapper, KERNEL_CS, PRIVILEGE_KERNEL);
    enable_irq(RTC_IRQ);
    
    rtc_init();

    // Set virtualized RTC rate to 2 Hz
    pcb_t *pcb = get_current_pcb();
    pcb->rtc_enabled = 1;
    pcb->rtc_interval = MAX_FREQ / 2;  // we divide by 2 cuz, when a program opens RTC, it should be set to 2 Hz by default.
    pcb->remaining_rtc_ticks = pcb->rtc_interval;
    return 0;
}

/**
 * rtc_close
 *   DESCRIPTION:  Disables the virtualized RTC for this process
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *                  0 on success
 *   SIDE EFFECTS: Disables RTC interrupts for this process only
 */ 
int32_t rtc_close(file_t *f) {
    pcb_t *pcb = get_current_pcb();
    pcb->rtc_enabled = 0;
    return 0;
}

/**
 * rtc_read
 *   DESCRIPTION:  Returns (only when) the next RTC tick occurs
 *   INPUTS:       f - file struct representing an RTC
 *                 buf - buffer to read to
 *                 nbytes - number of bytes to read
 *   OUTPUTS:      none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */ 
int32_t rtc_read(file_t *f, void *buf, int32_t nbytes) {
    pcb_t *pcb = get_current_pcb();
    if(pcb->rtc_enabled == 0) return -1;

    uint32_t flags;

    // We're in a system call, so interrupts have been disabled.
    // We need to temporarily enable interrupts so that we can
    // actually receive the RTC tick.
    cli_and_save(flags);
    sti();

    // wait for RTC tick
    while(pcb->remaining_rtc_ticks) {
        //asm volatile("hlt;");
    }

    cli();
    pcb->remaining_rtc_ticks = pcb->rtc_interval;

    restore_flags(flags);
    return 0;                   // acknowledge RTC tick
}

/**
 * rtc_write
 *   DESCRIPTION:  Changes the frequency of the RTC
 *   INPUTS:       f - file struct representing an RTC
 *                 buf - buffer to write
 *                 nbytes - number of bytes to write
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *                  0 on success
 *   SIDE EFFECTS: changes the frequency of the RTC
 */ 
int32_t rtc_write(file_t *f, const void *buf, int32_t nbytes) {

    uint32_t hertz = *((uint32_t*) buf);

    if(hertz < MIN_FREQ || hertz > MAX_FREQ) 
        return -1;

    // Check for power of 2
    if((hertz & (hertz-1)) != 0)
        return -1;   

    pcb_t *pcb = get_current_pcb();
    pcb->rtc_interval = MAX_FREQ / hertz;

    return 0;
}
