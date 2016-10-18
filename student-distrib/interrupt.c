#include "interrupt.h"
#include "lib.h"
#include "x86_desc.h"

void install_interrupt_handler(uint8_t interrupt_num, void *handler, uint8_t seg_selector, uint8_t dpl) {
	idt_desc_t exception_handle_desc;
	SET_IDT_ENTRY(exception_handle_desc, handler);
	exception_handle_desc.present = 1;
	exception_handle_desc.dpl = dpl;
	exception_handle_desc.reserved0 = 0;

	// 32-bit 80386 interrupt gate
	exception_handle_desc.size = 1;
	exception_handle_desc.reserved1 = 1;
	exception_handle_desc.reserved2 = 1;
	exception_handle_desc.reserved3 = 0;

	exception_handle_desc.reserved4 = 0;

	// Kernel code segment
	exception_handle_desc.seg_selector = seg_selector;

	idt[interrupt_num] = exception_handle_desc;
}

void exception_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
	uint32_t esi, uint32_t edi, uint32_t esp, uint32_t ebp,
	uint32_t int_num, uint32_t error,
	uint32_t eip, uint32_t cs, uint32_t eflags) {

	// TODO: check if the values of these registers are actually correct

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

	// loop forever
	for(;;) {
    	asm("hlt");
 	}
}
