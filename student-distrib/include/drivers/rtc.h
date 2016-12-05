#ifndef _RTC_H
#define _RTC_H

#include "types.h" // Rodney: added for mp3.2 to recognize int32_t
#include <lib/file.h>

#define RTC_IRQ 8
#define RTC_STATUS_PORT 0x70
#define RTC_DATA_PORT 0x71

#define RTC_DISABLE_NMI 0x80
#define RTC_ENABLE_INTERRUPTS 0x40

#define RTC_REG_A 0xA
#define RTC_REG_B 0xB
#define RTC_REG_C 0xC

#define MIN_FREQ    2
#define MAX_FREQ 1024

void set_rtc_test_enabled(int enabled);
void rtc_handler();
extern void rtc_handler_wrapper(void);

int32_t rtc_open(file_t *f, const int8_t * filename);
int32_t rtc_close(file_t *f);
int32_t rtc_read(file_t *f, void *buf, int32_t nbytes);         
int32_t rtc_write(file_t *f, const void *buf, int32_t nbytes);

#endif
