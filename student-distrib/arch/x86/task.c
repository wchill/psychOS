#include <arch/x86/x86_desc.h>
#include <lib/lib.h>
#include <arch/x86/task.h>
#include <fs/fs.h>
#include <lib/file.h>
#include <tty/terminal.h>
#include <drivers/pit.h>
#include <arch/x86/i8259.h>
#include <kernel/syscall.h>

inline uint32_t get_next_pid() {
    static uint32_t next_pid = 0;
    return next_pid++;
}

/**
 * get_pcb_from_esp
 * Get a process's Process Control Block given its current stack pointer.
 * 
 * @param process_esp   Pointer to the process's current stack
 *
 * @return              A pointer to the process's PCB
 */
inline pcb_t *get_pcb_from_esp(void *process_esp) {
    // The PCB is at the end of the process's kernel stack
    // Subtract 1 first in case stack hasn't been used yet (otherwise
    // the bitmask won't do anything), then AND out the bits representing
    // 8kB worth of memory space
    return (pcb_t *) ((uint32_t) (process_esp - 1) & PCB_BITMASK);
}

/**
 * get_pcb_from_slot
 * Get a process's Process Control Block given its current process slot.
 * 
 * @param pcb_slot  The process's current process slot
 *
 * @return          A pointer to the process's PCB
 */
inline pcb_t *get_pcb_from_slot(uint32_t pcb_slot) {
    // Take the end of the kernel page, subtract (8kB * (process # + 1)) to get
    // to the end of that process's kernel stack, which is also where its PCB is
    return (pcb_t*) (KERNEL_PAGE_END - (KERNEL_STACK_SIZE * (pcb_slot + 1)));
}

/**
 * get_current_pcb
 * Get the current process's Process Control Block.
 *
 * @return      A pointer to the process's PCB
 */
inline pcb_t *get_current_pcb() {
    // To get the current kernel stack pointer so we can get the PCB,
    // we just allocate a stack variable and take its address
    int stack_var = 0;
    return get_pcb_from_esp(&stack_var);
}

/**
 * get_current_kernel_stack_base
 * Get a pointer to the beginning of the current process's kernel stack.
 *
 * @return      A pointer to the start of the current process's kernel stack
 */
inline void *get_current_kernel_stack_base() {
    // Determine current stack pointer, determine the end of the stack, then add
    // the size of the stack to get to the beginning
    int stack_var = 0;
    return (void*) (((uint32_t) &stack_var & PCB_BITMASK) + KERNEL_STACK_SIZE);
}

/**
 * get_kernel_stack_base_from_slot
 * Get a pointer to the beginning of the current process's kernel stack.
 *
 * @param pcb_slot  The process's current process slot
 *
 * @return          A pointer to the start of the process's kernel stack
 */
inline void *get_kernel_stack_base_from_slot(uint32_t pcb_slot) {
    // Starting from the end of the kernel page, subtract (8kB * process #)
    // to get to the beginning of a process stack
    return (void*) (KERNEL_PAGE_END - (KERNEL_STACK_SIZE * pcb_slot));
}

/**
 * get_process_page_from_slot
 * Get a pointer to the beginning of a process's 4MB memory page.
 * 
 * @param task_slot     The process's current process slot
 *
 * @return              A pointer to the start of the process's memory page
 */
inline void *get_process_page_from_slot(uint32_t task_slot) {
    // Starting from the end of the kernel page, add (4MB * process #)
    // to get to the beginning of a process page
    return (void*) (KERNEL_PAGE_END + (PROCESS_PAGE_SIZE * task_slot));
}

/**
 * open_stdin_and_stdout
 * "Opens" stdin and stdout for the given process.
 * 
 * @param pcb   Pointer to a Process Control Block for some process.
 */
void open_stdin_and_stdout(pcb_t *pcb) {
    // Open, read, write, close
    static file_ops stdin_fops = {
        NULL,
        terminal_read,
        NULL,
        NULL
    };

    static file_ops stdout_fops = {
        NULL,
        NULL,
        terminal_write,
        NULL
    };

    // stdin
    pcb->fa[0].flags = FILE_IN_USE;
    pcb->fa[0].fops = &stdin_fops;
    pcb->fa[0].file_position = 0;

    // stdout
    pcb->fa[1].flags = FILE_IN_USE;
    pcb->fa[1].fops = &stdout_fops;
    pcb->fa[1].file_position = 0;
}

/**
 * kernel_run_first_program
 * Clears all PCBs, then sets up an execution environment and runs a program. Does not return.
 * 
 * @param command   Pointer to a string containing the name of a command (and arguments, space separated, if desired)
 */
void kernel_run_first_program(const int8_t* command) {
    cli();

    // Clear kernel PCBs
    int i;
    for(i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *current_pcb = get_pcb_from_slot(i);
        current_pcb->in_use = 0;
        current_pcb->slot_num = i;
        current_pcb->status = PROCESS_NONE;
    }
    multiple_terminal_init();

    for(i = 0; i < NUM_TERMINALS; i++) {
        pcb_t *child_pcb = get_pcb_from_slot(i);

        // Save program name and args in the child process's PCB
        parse_command(command, child_pcb->program_name, child_pcb->args);

        // Load program and determine entrypoint
        uint32_t entrypoint = load_program_into_slot(child_pcb->program_name, child_pcb->slot_num);
        if(entrypoint == NULL) return;
        child_pcb->entrypoint = entrypoint;

        // Process paging
        void *vmem_ptr = get_terminal_output_buffer(i);
        child_pcb->process_pd_ptr = setup_process_paging(get_process_page_from_slot(child_pcb->slot_num), child_pcb->slot_num, vmem_ptr);

        // Set up this process's PCB
        child_pcb->parent = NULL;
        child_pcb->child = NULL;
        child_pcb->in_use = 1;
        child_pcb->pid = get_next_pid();
        child_pcb->status = PROCESS_RUNNING;
        child_pcb->terminal_num = i;
        open_stdin_and_stdout(child_pcb);
    }
    pcb_t *child_pcb = get_pcb_from_slot(0);
    enable_paging(child_pcb->process_pd_ptr);

    // Prepare for context switch
    set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));

    // Enable scheduling/sleep
    enable_irq(PIT_IRQ);

    // Start the program
    switch_to_ring_3(PROCESS_LINK_START, child_pcb->entrypoint);
}

/**
 * halt_program
 * Exits the current program and returns to the parent program.
 * 
 * @param status    The program's exit code
 *
 * @return          Does not return (jumps back to parent program)
 */
int32_t halt_program(int32_t status) {
    pcb_t *child_pcb = get_current_pcb();

    // Mark the process's PCB as unused
    child_pcb->in_use = 0;

    // Close all file descriptors
    int i;
    for(i = 0; i < MAX_FILE_DESCRIPTORS; i++) {
        syscall_close(i);
    }

    pcb_t *parent_pcb = child_pcb->parent;

    // If this process does not have a parent, then it should be restarted
    if(parent_pcb == NULL) { 
        reset_terminal(child_pcb->terminal_num);

        // Set up this process's PCB
        child_pcb->parent = NULL;
        child_pcb->child = NULL;
        child_pcb->in_use = 1;
        child_pcb->pid = get_next_pid();
        child_pcb->status = PROCESS_RUNNING;
        open_stdin_and_stdout(child_pcb);

        // Prepare for context switch
        set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));

        // Start the program
        switch_to_ring_3(PROCESS_LINK_START, child_pcb->entrypoint);
    }

    child_pcb->status = PROCESS_NONE;
    parent_pcb->status = PROCESS_RUNNING;

    // Change the page table and TSS esp0/kernel stack to that of the parent process
    enable_paging(parent_pcb->process_pd_ptr);
    set_kernel_stack(get_kernel_stack_base_from_slot(parent_pcb->slot_num));

    /*
    A little bit of ASM hackery here.
    We have to do several things:
    - Restore the parent process's esp and ebp
    - Somehow jump to where the parent process left off
    - Turn the status exit code into a 32 bit number and return it

    Because we're using inline assembly, it's not a one-to-one translation to what
    actually gets output. Loading something as a parameter into inline assembly is
    complicated, and what makes things more annoying is parameters are based off of
    their offset from ebp. So we have to make sure we take care of the exit code before
    we restore ebp, otherwise we'll have garbage (hence why we do "mov status, %eax" first).
    Also things may get loaded into intermediate registers; hence we push/pop the exit code
    into eax.

    When we restore esp and ebp for the parent process's kernel stack, the leave instruction
    sets esp back to ebp, then pops the previous ebp from the stack. What's now on top of the
    stack is the parent process's return address from the execute syscall. So in effect,
    setting eax, restoring the registers and doing a leave/ret is essentially just returning
    from the execute function without actually going back there.

    Then we go back into the system call interrupt handler which restores the rest of the
    registers for us (save eax because that holds the return value from the system call).
    */
    asm volatile(
        "mov %0, %%esp\r\n"
        "push %2\r\n"
        "mov %1, %%ebp\r\n"
        "pop %%eax\r\n"
        "leave\r\n"
        "ret\r\n"
        :
        : "r"(parent_pcb->regs.esp), "r"(parent_pcb->regs.ebp), "r"(status)
        : "memory", "cc"
    );

    // We never reach this point; this is just to shut gcc up
    return -1;
}

/**
 * set_kernel_stack
 * Updates the TSS to use the given kernel stack, then flushes the TSS.
 * 
 * @param stack     Pointer to the stack to use when handling a syscall.
 */
void set_kernel_stack(const void *stack) {
    // Set TSS values
    tss.ss0 = KERNEL_DS;
    tss.esp0 = (uint32_t) stack;

    // Reset the TSS entry in GDT (might not be necessary, but I had problems otherwise)
    seg_desc_t the_tss_desc;
    the_tss_desc.granularity    = 0;
    the_tss_desc.opsize         = 0;
    the_tss_desc.reserved       = 0;
    the_tss_desc.avail          = 0;
    the_tss_desc.seg_lim_19_16  = TSS_SIZE & 0x000F0000;
    the_tss_desc.present        = 1;
    the_tss_desc.sys            = 0;
    the_tss_desc.type           = 0x9;

    // Make accessible from ring 3
    the_tss_desc.dpl            = 0x3;

    SET_TSS_PARAMS(the_tss_desc, &tss, tss_size);
    tss_desc_ptr = the_tss_desc;

    // Flush the TSS
    ltr(KERNEL_TSS);
}

/**
 * get_executable_entrypoint
 * Verifies that a binary is in ELF format and determines its starting address.
 * 
 * @param executable    Pointer to an executable in memory
 *
 * @return              NULL if not a valid binary, else a 32-bit integer representing the start address
 */
uint32_t get_executable_entrypoint(const void *executable) {
    const int8_t *ptr = (const int8_t*) executable;

    // Check for ELF header
    if(strncmp(ptr, ELF_MAGIC_HEADER, ELF_MAGIC_HEADER_LEN)) {
        return NULL;
    }

    // Read entry point address
    uint32_t addr = *((uint32_t*) &ptr[ELF_ENTRYPOINT_OFFSET]);

    return addr;
}

/**
 * parse_command
 * Given a string containing a program and its arguments, copy the arguments into a buffer.
 * 
 * @param command   Pointer to a string containing a program and its arguments
 * @param buf_name  Pointer to a buffer to copy program name to (like hello, pingpong, etc)
 * @param buf_args  Pointer to a buffer to copy arguments to
 *
 * @return          The number of bytes copied to buf_args
 */
int32_t parse_command(const int8_t *command, int8_t *buf_name, int8_t *buf_args) {
    // Parse args
    int index      = 0;
    int len_name   = 0;
    int len_args   = 0;

    int8_t ch;
    for (index = 0;   ; index++){
        ch = command[index];
        if (ch == ' ' || ch == '\0'){
            break;
        }
    }
    len_name = index;

    // Copy the program name
    strncpy(buf_name, command, len_name);
    buf_name[len_name] = '\0'; // null terminate the string
    
    // Find arguments (if any). Copy them into provided buffer.
    if (ch == ' '){ // then there is more to parse, so grab arguments
        index++;
        len_args = strlen(&command[index]);
        strncpy(buf_args, &command[index], len_args);
    }
    buf_args[len_args] = '\0'; // null terminate the string

    return len_args;
}
/**
 * load_program_into_slot
 * Load a program into a given process slot.
 * 
 * @param filename  Pointer to a string containing a filename to load
 * @param pcb_slot  An integer representing which process slot to use
 *
 * @return          NULL if not a valid program or nonexistent; start address otherwise
 */
uint32_t load_program_into_slot(const int8_t *filename, uint32_t pcb_slot) {
    // Get memory page and copy file into the correct location in that page
    void *process_page = get_process_page_from_slot(pcb_slot);
    int32_t retval = read_file_by_name(filename, process_page + PROCESS_LINK_OFFSET, PROCESS_PAGE_SIZE - PROCESS_LINK_OFFSET);

    // File doesn't exist
    if(retval < 0) return NULL;

    // Can't possibly be a valid program (<= size of ELF header)
    if(retval <= ELF_MAGIC_HEADER_LEN) return NULL;

    return get_executable_entrypoint(process_page + PROCESS_LINK_OFFSET);
}
