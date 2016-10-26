#include <rtc.h>
#include <arch/x86/i8259.h>
#include <types.h>
#include <lib.h>
#include <arch/x86/x86_desc.h>
#include <arch/x86/interrupt.h>

/* References:
	http://wiki.osdev.org/RTC#Programming_the_RTC                            (this is the main reference we used)
	https://piazza.com/class/iqsg6pdvods1rw?cid=596                          
	https://courses.engr.illinois.edu/ece391/secure/references/mc146818.pdf  (we didn't really use this, but it is the most detailed reference)
*/

static volatile int rtc_tick_flag = 0;	/* Flag that waits for RTC tick. 1 = waiting for tick. 0 = tick occured */

/*
 * rtc_handler
 *   DESCRIPTION:  this code runs when RTC Interrupt happens. Makes screen flash
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: makes screen flash when it tests interrupts.
 */  
void rtc_handler() {
	outportb(RTC_STATUS_PORT, RTC_REG_C);
	inportb(RTC_DATA_PORT);

	//test_interrupts(); // This causes screen to flash. Can comment it out to stop flashing.
	test_rtc();     

	rtc_tick_flag = 0; // Rodney: newly added for 3.2
	
	send_eoi(RTC_IRQ);
}

/*
 * rtc_read
 *   DESCRIPTION:  Returns (only when) the next RTC tick occurs
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: 0
 *   SIDE EFFECTS: none
 */ 
int32_t rtc_read() {
	rtc_tick_flag = 1;			// set flag to wait for RTC tick
	while(rtc_tick_flag);	// wait for RTC tick
	return 0;					// acknowledge RTC tick
}

/*
 * rtc_write
 *   DESCRIPTION:  Changes the frequency of the RTC
 *   INPUTS:       htz - frequency in Hz. Must be between 1 and 1024 inclusive, and also be a power of 2
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *				    0 on success
 *   SIDE EFFECTS: changes the frequency of the RTC
 */ 
int32_t rtc_write(uint32_t htz) {
	int     temp = htz;						// temp variable to calculate rate
	uint8_t rate = 15;						// rate passed to the RTC. Rate of 15 corresponds to 2 Hz. We use formula: htz = 32768 >> (rate - 1)
	char    prev;							// just a temporary variable

	/* Check for valid input */
	if( htz < MIN_FREQ || htz > MAX_FREQ ) 
		return -1;
	if( (htz & (htz-1)) != 0)  				// fancy & efficient way to check if htz is a power of 2
		return -1;			

	/* Calculate appropriate 'rate' value for the htz we want */
	while (temp != MIN_FREQ){
		temp >>= 1;							// divides by 2
		rate--;
	}

	// set the interrupt rate
	outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_A);
	prev = inportb(RTC_DATA_PORT);								// get initial value of register A
	outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_A);		// reset index to A
	outportb(RTC_DATA_PORT, (prev & 0xF0) | rate);				// write only our rate to A. Note, rate is the bottom 4 bits, so keep the top 4 bits. 0xF0 is top 4 bit mask for a char.

	return 0;
}

/*
 * rtc_open
 *   DESCRIPTION:  Initializes the RTC with a frequency of 2 Hz (TA Rohan's suggestion)
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *				    0 on success (for now we are always succesful)
 *   SIDE EFFECTS: changes the frequency of the RTC to 2 Hz
 */ 
int32_t rtc_open() {
	// TODO checkpoint 3 File Descriptor Table (FDT)

	char prev;	// just a temporary variable.

	outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_B); 	// select register B, and disable NMI
	prev = inportb(RTC_DATA_PORT);								// read the current value of register B
	outportb(RTC_STATUS_PORT, RTC_DISABLE_NMI | RTC_REG_B);		// set the index again (a read will reset the index to register D)
	outportb(RTC_DATA_PORT, prev | RTC_ENABLE_INTERRUPTS);		// write the previous value ORed with 0x40. This turns on bit 6 of register B

	// Need to initialize RTC before enabling IRQ
	install_interrupt_handler(IRQ_INT_NUM(RTC_IRQ), rtc_handler_wrapper, KERNEL_CS, PRIVILEGE_KERNEL);
	enable_irq(RTC_IRQ);
	
	rtc_write(MIN_FREQ);

	return 0;
}

/*
 * rtc_close
 *   DESCRIPTION:  Sets RTC to frequency of 2 Hz. We keep RTC interrupts on as suggested on p.16 of MP3 spec.
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: -1 on failure
 *				    0 on success
 *   SIDE EFFECTS: changes the frequency of the RTC to 2 Hz
 */ 
int32_t rtc_close() {
	// TODO checkpoint 3 File Descriptor Table (FDT)

	return rtc_write(MIN_FREQ);
}
