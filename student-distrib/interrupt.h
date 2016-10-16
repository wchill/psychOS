#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "types.h"

void install_interrupt_handler(uint8_t interrupt_num, void *handler, uint8_t seg_selector, uint8_t dpl);

void keyboard_handler();

void rtc_handler();

void exception_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
	uint32_t esi, uint32_t edi, uint32_t esp, uint32_t ebp,
	uint32_t int_num, uint32_t error,
	uint32_t eip, uint32_t cs, uint32_t eflags);

extern void null_interrupt_handler(void);

extern void interrupt_handler_0(void);
extern void interrupt_handler_1(void);
extern void interrupt_handler_2(void);
extern void interrupt_handler_3(void);
extern void interrupt_handler_4(void);
extern void interrupt_handler_5(void);
extern void interrupt_handler_6(void);
extern void interrupt_handler_7(void);
extern void interrupt_handler_8(void);
extern void interrupt_handler_9(void);
extern void interrupt_handler_10(void);
extern void interrupt_handler_11(void);
extern void interrupt_handler_12(void);
extern void interrupt_handler_13(void);
extern void interrupt_handler_14(void);
extern void interrupt_handler_15(void);
extern void interrupt_handler_16(void);
extern void interrupt_handler_17(void);
extern void interrupt_handler_18(void);
extern void interrupt_handler_19(void);
extern void interrupt_handler_20(void);
extern void interrupt_handler_21(void);
extern void interrupt_handler_22(void);
extern void interrupt_handler_23(void);
extern void interrupt_handler_24(void);
extern void interrupt_handler_25(void);
extern void interrupt_handler_26(void);
extern void interrupt_handler_27(void);
extern void interrupt_handler_28(void);
extern void interrupt_handler_29(void);
extern void interrupt_handler_30(void);
extern void interrupt_handler_31(void);

extern void keyboard_handler_wrapper(void);
extern void rtc_handler_wrapper(void);
extern void syscall_handler_wrapper(void);

#endif
