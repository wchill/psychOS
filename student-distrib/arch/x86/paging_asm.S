# Paging References:
# http://wiki.osdev.org/Paging
# https://courses.engr.illinois.edu/ece391/secure/references/IA32-ref-manual-vol-3.pdf (page 86 is useful)


# void enable_paging(int * pd_address);
#
# Sets appropriate flags and enables paging
#
# Inputs   : pd_address  - 32-bit address to page directory
# Outputs  : none
# Registers: Standard C calling convention
.globl enable_paging
enable_paging:
    /* Create stack frame */
    pushl   %ebp                        # save old frame pointer
    movl    %esp, %ebp                  # point to new frame
    pushl   %ebx                        # protect callee-saved register
    pushl   %esi                        # protect callee-saved register
    pushl   %edi                        # protect callee-saved register


    # Page directory passed in first parameter
    mov 8(%ebp), %eax                   # 8 is offset to 1st argument.
    mov %eax, %cr3

    # Set Page Size Extensions bit / 4MB pages
    mov %cr4, %ebx
    or  $0x10, %ebx                     # 0x10 is mask to turn on the 4th bit

    # Clear Physical Address Extension bit / Disable 36-bit addressing
    and $0xFFFFFFDF, %ebx               # 0xFFFFFFDF is mask to turn off 5th bit. Rodney: I originally mentioned this is necessary, now unsure.
    mov %ebx, %cr4

    # Set Paging Enable bit / enable paging
    mov %cr0, %ebx
    or  $0x80000000, %ebx               # 0x80000000 is mask to turn on 32nd bit. 
    mov %ebx, %cr0


    /* Tear down stack frame */
    popl    %edi                        # restore callee-saved register
    popl    %esi                        # restore callee-saved register
    popl    %ebx                        # restore callee-saved register
    leave                               # restore frame pointer
    ret                                 # return
