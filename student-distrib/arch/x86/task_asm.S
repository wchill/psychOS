#define ASM     1
#include <arch/x86/x86_desc.h>

.globl switch_to_ring_3
switch_to_ring_3:
	cli

	# Backup pointer to function
	pop 	%ebx

	# Set segment registers
	xor 	%eax, %eax
	mov 	USER_DS, %ax
	mov 	%eax, %ds
	mov 	%eax, %es
	mov 	%eax, %fs
	mov 	%eax, %gs

	# Need to create artificial iret context
	# push in this order: ss, esp, eflags, cs, eip
	movl 	%esp, %eax
	pushl 	USER_DS
	pushl 	%eax
	# push flags (16-bit)
	# todo: evaluate whether this is actually correct?
	pushfd
	popfd 	%eax
	or 		$0x200, %eax
	push 	%eax
	pushl 	USER_CS
	pushl 	%ebx
	iretd
