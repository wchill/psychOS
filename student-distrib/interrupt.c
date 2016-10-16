#include "interrupt.h"
#include "lib.h"

void interrupt_handler(uint32_t interrupt_num, uint32_t error_code) {
	clear();
	printf("An exception has occurred: %x\n", interrupt_num);
	printf("The associated error code is: %x\n", error_code);
	asm volatile(".1: hlt; jmp .1;");
}
