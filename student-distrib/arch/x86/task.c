#include <arch/x86/task.h>

void set_kernel_stack(void *stack) {
    tss.esp0 = (uint32_t) stack;
    ltr(KERNEL_TSS);
}
