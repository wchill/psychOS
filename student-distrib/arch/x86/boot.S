# boot.S - start point for the kernel after GRUB gives us control
# vim:ts=4 noexpandtab

#define ASM     1

#include <arch/x86/multiboot.h>
#include <arch/x86/x86_desc.h>

# GDT Reference: 
# Intel Manual - https://courses.engr.illinois.edu/ece391/secure/references/IA32-ref-manual-vol-3.pdf (see page 336, steps 2, 3)

.text

	# Multiboot header (required for GRUB to boot us)
	.long MULTIBOOT_HEADER_MAGIC
	.long MULTIBOOT_HEADER_FLAGS
	.long -(MULTIBOOT_HEADER_MAGIC+MULTIBOOT_HEADER_FLAGS)

# GDTR (GDT register) is 48 bits. It has two parts: 16-bit size and 32-bit address to table
gdtr:
gdtr_size:      # size is needed since it may be smaller than max size of 8192
	.word 0     # word is 16 bits
gdtr_ptr:
	.long 0     # long is 32 bits

# Entrypoint to the kernel
.globl  start, _start

.align 4
start:
_start:
	# Make sure interrupts are off
	cli
	jmp     continue

continue:
	# Load the GDT
	# Load the size of the GDT
	movl 	$gdt_size, %ecx
	movw 	%cx, gdtr_size

	# Load the address of the GDT
	leal 	gdt, %ecx
	movl 	%ecx, gdtr_ptr

	# Set the GDT register (48-bit)
	lgdt	gdtr

	# Load CS with the new descriptor value
	ljmp    $KERNEL_CS, $keep_going

keep_going:
	# Set up ESP so we can have an initial stack
	movl    $0x800000, %esp

	# Set up the rest of the segment selector registers
	movw    $KERNEL_DS, %cx
	movw    %cx, %ss
	movw    %cx, %ds
	movw    %cx, %es
	movw    %cx, %fs
	movw    %cx, %gs

	# Push the parameters that entry() expects (see kernel.c):
	# eax = multiboot magic
	# ebx = address of multiboot info struct
	pushl   %ebx
	pushl   %eax

	# Jump to the C entrypoint to the kernel.
	call    entry

	# We'll never get back here, but we put in a hlt anyway.
halt:
	hlt
	jmp     halt
