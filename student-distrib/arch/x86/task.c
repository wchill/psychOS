#include <arch/x86/x86_desc.h>
#include <lib/lib.h>
#include <arch/x86/task.h>
#include <fs/fs.h>
#include <lib/file.h>
#include <tty/terminal.h>

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
    // stdin
    pcb->fa[0].flags = FILE_IN_USE;
    pcb->fa[0].fops.open = NULL;
    pcb->fa[0].fops.close = NULL;
    pcb->fa[0].fops.read = terminal_read;
    pcb->fa[0].fops.write = NULL;
    pcb->fa[0].file_position = 0;

    // stdout
    pcb->fa[1].flags = FILE_IN_USE;
    pcb->fa[1].fops.open = NULL;
    pcb->fa[1].fops.close = NULL;
    pcb->fa[1].fops.read = NULL;
    pcb->fa[1].fops.write = terminal_write;
    pcb->fa[1].file_position = 0;
}

/**
 * kernel_run_first_program
 * Clears all PCBs, then sets up an execution environment and runs a program. Does not return.
 * 
 * @param command   Pointer to a string containing the name of a command (and arguments, space separated, if desired)
 */
void kernel_run_first_program(const int8_t* command) {
    // Clear kernel PCBs
    int i;
    for(i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *current_pcb = get_pcb_from_slot(i);
        current_pcb->in_use = 0;
        current_pcb->slot_num = i;
    }

    // TODO: change for 3.5
    pcb_t *child_pcb = get_pcb_from_slot(0);

    // Process paging
    setup_process_paging(child_pcb->process_pd, get_process_page_from_slot(child_pcb->slot_num), child_pcb->slot_num);
    enable_paging(child_pcb->process_pd);

    // Initialize terminal for programs to use
    terminal_open(&child_pcb->fa[0], "stdin");

    // Load program and determine entrypoint
    uint32_t entrypoint = load_program_into_slot(command, child_pcb->slot_num);
    if(entrypoint == NULL) return;

    // Set up this process's PCB
    child_pcb->parent = NULL;
    child_pcb->child = NULL;
    child_pcb->in_use = 1;
    child_pcb->pid = 0;
    open_stdin_and_stdout(child_pcb);
    parse_args(command, child_pcb->args);

    // Prepare for context switch
    set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));

    // Start the program
    switch_to_ring_3(PROCESS_LINK_START, entrypoint);
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
 * parse_args
 * Given a string containing a program and its arguments, copy the arguments into a buffer.
 * 
 * @param command   Pointer to a string containing a program and its arguments
 * @param buf       Pointer to a buffer to copy arguments to
 *
 * @return          0 if no arguments provided, else the number of bytes copied
 */
int32_t parse_args(const int8_t *command, int8_t *buf) {
    // Parse args
    int index = 0;
    int start = 0;
    int len = 0;
    int8_t ch;
    while((ch = command[index++]) != '\0' && index) {
        // Find the first character after space
        if (ch == ' ') {
            while(ch == ' ') {
                ch = command[index++];
            }
            start = index;
            break;
        }
    }

    // If there were any args, then get their length and copy
    if(start > 0) {
        len = strlen(&command[start]);
        if(len > MAX_ARGS_LENGTH) {
            len = MAX_ARGS_LENGTH;
        }
        memcpy(buf, &command[start], len);
    }

    // Null terminate the string
    buf[len] = '\0';
    return len;
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

    return get_executable_entrypoint(process_page + PROCESS_LINK_OFFSET);
}
