#include <arch/x86/interrupt.h>
#include <lib/lib.h>
#include <tty/terminal.h>
#include <arch/x86/x86_desc.h>

// Human-readable errors for the 32 possible Exceptions (Entries 0-31 in IDT table)
static const char *human_readable_errors[] = {
    "Divide-by-zero",
    "Debug",
    "Non-maskable interrupt",
    "Breakpoint",
    "Overflow",
    "Bound range exceeded",
    "Invalid opcode",
    "Device not available",
    "Double fault",
    "Coprocessor segment overrun",
    "Invalid TSS",
    "Segment not present",
    "Stack-segment fault",
    "General protection fault",
    "Page fault",
    "Reserved",
    "x87 Floating point exception",
    "Alignment check",
    "Machine check",
    "SIMD Floating point exception",
    "Virtualization exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Security Exception",
    "Triple fault"
};

// 32-element array for the 32 Exceptions. Only a few of them have error codes, as indicated in array.
static const int has_error_code[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 1, 1, 1, 1, 1, 0,
    0, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 0
};

/**
 * install_interrupt_handler
 * Installs interrupt handler for a given interrupt number.
 * 
 * @param interrupt_num  The interrupt number on IDT (0 to 255)
 * @param handler        A pointer to function to handle the interrupt
 * @param seg_selector   The segment in GDT table. usually (or maybe even always) KERNEL_CS (See x86_desc.h)
 * @param dpl            Descriptor Privilige Level, the privilege level the handler can be called from (0, 1, 2, 3).
 */
void install_interrupt_handler(uint8_t interrupt_num, void *handler, uint8_t seg_selector, uint8_t dpl) {
    idt_desc_t exception_handle_desc;
    SET_IDT_ENTRY(exception_handle_desc, handler); // Takes 32-bit handler address and puts it in appropriate spots: top 16 and bottom 16 bits of 64-bit IDT entry
    exception_handle_desc.present = 1;
    exception_handle_desc.dpl = dpl;
    exception_handle_desc.reserved0 = 0;

    // 32-bit 80386 interrupt gate
    exception_handle_desc.size = 1;
    exception_handle_desc.reserved1 = 1;
    exception_handle_desc.reserved2 = 1;
    exception_handle_desc.reserved3 = 0;

    exception_handle_desc.reserved4 = 0; // We assume reserved bits are just for padding when we clear them here.

    // Kernel code segment
    exception_handle_desc.seg_selector = seg_selector;

    idt[interrupt_num] = exception_handle_desc;
}

/**
 * interpret_exception
 * Returns human-readable errors for interrupt numbers 0-31, and "Unknown" for numbers 32-255.
 * 
 * @param int_num   The interrupt number (From 0-255).
 *
 * @returns         a pointer to a const string containing a human-readable description of the error
 */
const char *interpret_exception(uint32_t int_num) {
    if(int_num >= NUM_RESERVED_VEC) return "Unknown";

    return human_readable_errors[int_num];
}

/**
 * exception_handler
 * Handles exceptions (0 to 31 in IDT table)
 *
 * @param eax      Register value (printed to screen)
 * @param ebx      Register value (printed to screen)
 * @param ecx      Register value (printed to screen)
 * @param edx      Register value (printed to screen)
 * @param esi      Register value (printed to screen)
 * @param edi      Register value (printed to screen)
 * @param ebp      Register value (printed to screen)
 * @param esp      Register value (printed to screen)
 * @param int_num  The interrupt number
 * @param error    The error number: 8, 10-14, 17, or 30. Other exceptions in range 0-31 just use error number 0.
 * @param eip      Register value (printed to screen)
 * @param cs       Not used here. It's still a parameter since it's on the stack.
 * @param eflags   Flags values (printed to screen)
 */
void exception_handler(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx,
    uint32_t esi, uint32_t edi, uint32_t ebp, uint32_t esp,
    uint32_t int_num, uint32_t error,
    uint32_t eip, uint32_t cs, uint32_t eflags) {

    //pcb_t *pcb = get_current_pcb()->parent;


    clear_terminal(0);
    switch_active_terminal(0);
    printf("---------------------------AN EXCEPTION HAS OCCURRED----------------------------\n\n");
    printf(" An exception has occurred: %s (0x%x)\n", interpret_exception(int_num), int_num);

    if(has_error_code[int_num]) {
        printf(" The associated error code is: 0x%#x\n", error);
    }
    printf("\n");

    switch(int_num) {
        case 0xE:
            {
                // Load cr2 register to determine the page fault address
                uint32_t virtual_addr;
                asm volatile(
                    "movl   %%cr2, %0"
                    : "=a"(virtual_addr)
                    : 
                    : "memory"
                );
                printf("An attempt was made to access the following virtual address: 0x%#x\n\n", virtual_addr);
            }
            break;
        default:
            break;
    }
    printf(" REGISTERS:\n");
    printf(" eax: 0x%#x    ebx:    0x%#x\n", eax, ebx);
    printf(" ecx: 0x%#x    edx:    0x%#x\n", ecx, edx);
    printf(" esi: 0x%#x    edi:    0x%#x\n", esi, edi);
    printf(" esp: 0x%#x    ebp:    0x%#x\n", esp, ebp);
    printf("\n");
    printf(" eip: 0x%#x    eflags: 0x%#x\n", eip, eflags);

    // loop forever
    for(;;) {
        asm volatile("hlt");
    }
}
