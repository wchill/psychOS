#define ASM     1
#include <arch/x86/x86_desc.h>

.globl switch_to_ring_3
switch_to_ring_3:
	cli

	# eip
	mov 	8(%esp), %ebx   # 8 is offset to get EIP

	# esp
	mov 	4(%esp), %ecx   # 4 is offset to get ESP

	# Set segment registers
	xor 	%eax, %eax
	mov 	$USER_DS, %ax
	mov 	%eax, %ds
	mov 	%eax, %es
	mov 	%eax, %fs
	mov 	%eax, %gs

	# Create artificial iret context
	# Push in this order: ss, esp, eflags, cs, eip

	# User stack = user data segment
	push 	$USER_DS

	# esp parameter
	push 	%ecx

	# push eflags, pop it off so we can modify it, then set IF in the new eflags
	# setting eflags is equivalent to ORing the register with 0x200
	pushf
	pop 	%eax
	or 		$EFLAGS_INTERRUPT, %eax
	push 	%eax

	# User code = user code segment
	push 	$USER_CS

	# eip parameter
	push 	%ebx

	# Return from ring 3 to new program
	iret
