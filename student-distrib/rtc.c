#include "rtc.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"
#include "x86_desc.h"

/*
 * rtc_handler
 *   DESCRIPTION:  this code runs when RTC Interrupt happens. Makes screen flash
 *   INPUTS:       none
 *   OUTPUTS:      none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: makes screen flash when it tests interrupts.
 */  
void rtc_handler() {
	outb(RTC_REG_C, RTC_STATUS_PORT);
	inb(RTC_DATA_PORT);

	test_interrupts(); // This is what causes screen to flash. Can comment it out to stop flashing.

	send_eoi(RTC_IRQ);
}
