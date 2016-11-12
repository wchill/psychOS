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

    # Page directory passed in first parameter
    mov 4(%esp), %eax                   # 4 is offset to 1st argument.
    mov %eax, %cr3

    # Set Page Size Extensions bit / 4MB pages
    mov %cr4, %eax
    or  $0x10, %eax                     # 0x10 is mask to turn on the 4th bit

    # Clear Physical Address Extension bit / Disable 36-bit addressing
    and $0xFFFFFFDF, %eax               # 0xFFFFFFDF is mask to turn off 5th bit. Rodney: I originally mentioned this is necessary, now unsure.
    mov %eax, %cr4

    # Set Paging Enable bit / enable paging
    mov %cr0, %eax
    or  $0x80000000, %eax               # 0x80000000 is mask to turn on 32nd bit. 
    mov %eax, %cr0

    ret                                 # return
