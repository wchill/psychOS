#include "interrupt.h"
#include "lib.h"

void interrupt_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
	uint32_t esi, uint32_t edi, uint32_t esp, uint32_t ebp,
	uint32_t error, uint32_t int_num,
	uint32_t eip, uint32_t cs, uint32_t eflags) {

	clear();
	printf("---------------------------AN EXCEPTION HAS OCCURRED-----------------------------\n\n");
	printf(" An exception has occurred: 0x%x\n", int_num);
	printf(" The associated error code is: 0x%#x\n", error);
	printf("\n\n REGISTERS:\n");
	printf(" eax: 0x%#x    ebx:    0x%#x\n", eax, ebx);
	printf(" ecx: 0x%#x    edx:    0x%#x\n", ecx, edx);
	printf(" esi: 0x%#x    edi:    0x%#x\n", esi, edi);
	printf("\n");
	printf(" eip: 0x%#x    eflags: 0x%#x\n", eip, eflags);
	asm volatile(".1: hlt; jmp .1;");
}
