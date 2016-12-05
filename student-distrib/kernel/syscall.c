#include <kernel/syscall.h>
#include <arch/x86/task.h>
#include <fs/fs.h>
#include <fs/ece391_fs.h>
#include <lib/file.h>
#include <lib/lib.h>
#include <drivers/rtc.h>

#define RTC_FT 0
#define DIRECTORY_FT 1
#define FILE_FT 2

// Declare fops structs for use in process file array
static file_ops file_fops = {
    file_open,
    file_read,
    file_write,
    file_close
};

static file_ops dir_fops = {
    dir_open,
    dir_read,
    dir_write,
    dir_close
};

static file_ops rtc_fops = {
    rtc_open,
    rtc_read,
    rtc_write,
    rtc_close
};

// Store pointers to above fops structs
static file_ops *fops_table[3] = {&rtc_fops, &dir_fops, &file_fops};

/**
 * syscall_open
 * Open a file.
 * 
 * @param filename  The name of the file to open
 *
 * @return          The file descriptor of the new file, or -1 on failure
 */
int32_t syscall_open(const uint8_t *filename) {
    /* TODO: determine whether stuff might be better initialized inside specific open instead of this */

    // get the process control block
    pcb_t *PCB = get_current_pcb();

    // find the lowest index in the file array that is free
    int32_t fd = -1;
    int32_t idx;
    for(idx = 0 ; idx < MAX_FILE_DESCRIPTORS ; idx++) {
        if(!(PCB->fa[idx].flags & FILE_IN_USE)) {
            fd = idx;
            break;
        }
    }

    // Check if file descriptor array is full
    if(fd == -1) return -1;

    // find file in file system
    dentry_t dentry;
    int32_t res = read_dentry_by_name((const int8_t*) filename, &dentry);

    // File does not exist
    if(res < 0) return -1;

    // Not a valid file type
    if(dentry.file_type > FILE_FT) {
        return -1;
    }

    // fill in table pointer in file array at the index of the file descriptor
    PCB->fa[fd].fops = fops_table[dentry.file_type];

    // call the open function, checking for error code
    int32_t retval = PCB->fa[fd].fops->open(&PCB->fa[fd], (const int8_t*) filename);
    if(retval < 0) return retval;

    // mark this file descriptor as taken
    PCB->fa[fd].flags = FILE_IN_USE;

    // copy the inode over to the file array at the index of the file descriptor
    PCB->fa[fd].inode = dentry.inode_num;

    // set file position to 0
    PCB->fa[fd].file_position = 0;

    // return file descriptor
    return fd;
}

/**
 * syscall_close
 * Close a file.
 * 
 * @param fd        The file descriptor of the file to close.
 *
 * @return          0 on success, -1 on failure
 */
int32_t syscall_close(int32_t fd) {
    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS) return -1;

    // get the process control block
    pcb_t *PCB = get_current_pcb();

    // Check if this fd is valid and if close is defined for it.
    if(!(PCB->fa[fd].flags & FILE_IN_USE)) return -1;
    if(PCB->fa[fd].fops == NULL || PCB->fa[fd].fops->close == NULL) return -1;

    // Mark the file descriptor as unused
    PCB->fa[fd].flags &= ~(FILE_IN_USE);

    // call the close function
    return PCB->fa[fd].fops->close(&PCB->fa[fd]);

}

/**
 * syscall_read
 * Reads from a file (and writes it to a buffer)
 * 
 * @param fd     File descriptor of file to read from.
 * @param buf    The buffer to write to.
 * @param nbytes The number of bytes to (try to) read.
 *
 * @return       The number of bytes read
 */
int32_t syscall_read(int32_t fd, void *buf, int32_t nbytes) {
    /* Error handling */
    uint32_t target_addr = (uint32_t) buf;

    // Check if pointer is in target page bounds
    if(!(PROCESS_VIRT_PAGE_START <= target_addr && target_addr < PROCESS_VIRT_PAGE_START + PROCESS_PAGE_SIZE + nbytes)) {
        return -1;
    }

    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS) return -1;

    // get the process control block
    pcb_t *PCB = get_current_pcb();

    // Check if this fd is valid and if read is defined for it.
    if(!(PCB->fa[fd].flags & FILE_IN_USE)) return -1;
    if(PCB->fa[fd].fops == NULL || PCB->fa[fd].fops->read == NULL) return -1;

    /* Actually call the read function */
    // Individual file operations should update file positions
    return PCB->fa[fd].fops->read(&PCB->fa[fd], buf, nbytes);
}

/**
 * syscall_write
 * Writes to a file (by reading from a buffer)
 * 
 * @param fd     File descriptor of file to write to.
 * @param buf    The buffer to read from.
 * @param nbytes The number of bytes to (try to) write.
 *
 * @return       The number of bytes written
 */
int32_t syscall_write(int32_t fd, const void *buf, int32_t nbytes) {
    /* Error handling */
    uint32_t target_addr = (uint32_t) buf;

    // Check if pointer is in target page bounds
    if(!(PROCESS_VIRT_PAGE_START <= target_addr && target_addr < PROCESS_VIRT_PAGE_START + PROCESS_PAGE_SIZE + nbytes)) {
        return -1;
    }

    if(fd < 0 || fd >= MAX_FILE_DESCRIPTORS) return -1;

    // get the process control block
    pcb_t *PCB = get_current_pcb();

    // Check if this fd is valid and if write is defined for it.
    if(!(PCB->fa[fd].flags & FILE_IN_USE)) return -1;
    if(PCB->fa[fd].fops == NULL || PCB->fa[fd].fops->write == NULL) return -1;

    /* Actually call the write function */
    // Individual file operations should update file positions
    return PCB->fa[fd].fops->write(&PCB->fa[fd], buf, nbytes);

}

/**
 * syscall_execute
 * Executes a new program.
 * 
 * @param command   The name of the new executable to launch.
 *
 * @return          0-255 if the program executed successfully
 *                  256 if the program was killed due to an exception
 *                  -1 on failure
 */
int32_t syscall_execute(const int8_t *command) {
    pcb_t *parent_pcb = get_current_pcb();
    pcb_t *child_pcb = NULL;

    // Find a free PCB slot
    int i;
    for(i = 0; i < MAX_PROCESSES; i++) {
        pcb_t *some_pcb = get_pcb_from_slot(i);
        if(!some_pcb->in_use) {
            child_pcb = some_pcb;
            child_pcb->slot_num = i;
            break;
        }
    }

    // No free PCB slots available
    if(child_pcb == NULL) return -1;
    
    // Save program name and args in the child process's PCB
    parse_command(command, child_pcb->program_name, child_pcb->args);

    // Load executable and check validity
    uint32_t entrypoint = load_program_into_slot(child_pcb->program_name, child_pcb->slot_num);
    if(entrypoint == NULL) return -1;
    child_pcb->entrypoint = entrypoint;

    // Set up paging for the new child process
    // FIXME: multiple terminals
    void *vmem_ptr = 0xB8000;
    child_pcb->process_pd_ptr = setup_process_paging(get_process_page_from_slot(child_pcb->slot_num), child_pcb->slot_num, vmem_ptr);
    enable_paging(child_pcb->process_pd_ptr);

    // Set up the child process's PCB
    child_pcb->parent = parent_pcb;
    child_pcb->child = NULL;
    parent_pcb->child = child_pcb;
    child_pcb->in_use = 1;
    child_pcb->pid = get_next_pid();
    child_pcb->terminal_num = parent_pcb->terminal_num;
    open_stdin_and_stdout(child_pcb);

    // Prepare for context switch: set the new kernel stack in the TSS and save esp/ebp registers
    set_kernel_stack(get_kernel_stack_base_from_slot(child_pcb->slot_num));
    asm volatile(
        "mov %%esp, %0\r\n"
        "mov %%ebp, %1\r\n"
        : "=g"(parent_pcb->regs.esp), "=g"(parent_pcb->regs.ebp)
        :
        : "memory"
    );

    // Switch to user mode
    switch_to_ring_3(PROCESS_LINK_START, child_pcb->entrypoint);

    // We'll never come back here (read syscall_halt comments for complete reason)
    return -1;
}

/**
 * syscall_halt
 * Exits the current program and returns to the parent program.
 * 
 * @param status    The program's exit code
 *
 * @return          Does not return (jumps back to parent program)
 */
int32_t syscall_halt(uint32_t status) {
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

        // Set up a temporary page table to use while restarting process, otherwise machine page faults for some reason
        //static pd_entry temp_pd[NUM_PD_ENTRIES] __attribute__((aligned (FOUR_KB_ALIGNED)));
        //initialize_paging_structs(temp_pd);
        //enable_paging(temp_pd);

        // We need to call the function that kernel initially calls to load the first shell
        // This will have to be changed later for 3.5, since that function clears all PCBs
        // Have to be careful though: declaring the command string static means that it
        // shouldn't be touched by any code doing stack operations
        void *current_kernel_stack_base = get_current_kernel_stack_base();
        static const int8_t cmd[] = "shell\0";

        // We need to stack pivot (?) hence assembly
        // Push the parameter, push a return address that is never used, jump to function
        asm volatile(
            "mov %0, %%esp\r\n"
            "push %1\r\n"
            "push $0\r\n"
            "jmp *%2\r\n"
            :
            : "r"(current_kernel_stack_base), "r"(cmd), "r"(kernel_run_first_program)
            : "memory"
        );
    }

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
        : "r"(parent_pcb->regs.esp), "r"(parent_pcb->regs.ebp), "r"(status & 0x000000FF)
        : "memory", "cc"
    );

    // We never reach this point; this is just to shut gcc up
    return -1;
}

/**
 * syscall_getargs
 * PCB already has arguments saved when a new program was loaded. This copies the arguments into a user-level buffer.
 * 
 * @param buf     A user-level buffer to copy arguments into
 * @param nbytes  Size of the buffer (Rodney: This is my best guess as to what this is)
 *
 * @return        
 */
int32_t syscall_getargs(uint8_t *buf, int32_t nbytes) {
    uint32_t target_addr = (uint32_t) buf;

    // Check if pointer is in target page bounds
    if(!(PROCESS_VIRT_PAGE_START <= target_addr && target_addr < PROCESS_VIRT_PAGE_START + PROCESS_PAGE_SIZE + nbytes)) {
        return -1;
    }
    
    pcb_t *child_pcb = get_current_pcb();
    
    // Error check: Make sure buffer is big enough to fit all the arguments.
    uint32_t args_length = strlen((const int8_t *) child_pcb->args) + 1; // We add 1 to count the '\0' at end of string that strlen() does not account for.
    if (nbytes < args_length){
        return -1;
    }
    // Copy Data
    strcpy((int8_t *) buf, (const int8_t *) child_pcb->args);

    return 0;
}

int32_t syscall_vidmap(uint8_t **screen_start) {
    pcb_t *pcb = get_current_pcb();
    uint32_t target_addr = (uint32_t) screen_start;

    // Check if pointer is in target page bounds
    if(!(PROCESS_VIRT_PAGE_START <= target_addr && target_addr < PROCESS_VIRT_PAGE_START + PROCESS_PAGE_SIZE + sizeof(target_addr))) {
        return -1;
    }

    // Update the process's pointer to video memory
    *screen_start = (uint8_t*) get_process_vmem_page(pcb->slot_num);
    return 0;
}

int32_t syscall_set_handler(int32_t signum, void *handler_address) {
    return -1;
}

int32_t syscall_sigreturn() {
    return -1;
}
