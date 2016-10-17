#include "rtc.h"
#include "i8259.h"
#include "types.h"
#include "lib.h"
#include "x86_desc.h"

void rtc_handler() {
	outb(RTC_REG_C, RTC_STATUS_PORT);
	inb(RTC_DATA_PORT);

	test_interrupts();

	send_eoi(RTC_IRQ);
}
