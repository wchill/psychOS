#ifndef _RTC_H
#define _RTC_H

#include "types.h" // Rodney: added for mp3.2 to recognize int32_t

#define RTC_IRQ 8
#define RTC_STATUS_PORT 0x70
#define RTC_DATA_PORT 0x71

#define RTC_DISABLE_NMI 0x80
#define RTC_ENABLE_INTERRUPTS 0x40

#define RTC_REG_A 0xA
#define RTC_REG_B 0xB
#define RTC_REG_C 0xC

void rtc_handler();
extern void rtc_handler_wrapper(void);

#define MIN_FREQ    2
#define MAX_FREQ 1024

int32_t rtc_open();
int32_t rtc_close();
int32_t rtc_read();         
int32_t rtc_write(uint32_t htz);

#endif