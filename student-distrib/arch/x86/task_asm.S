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

	# Need to create artificial iret context
	# push in this order: ss, esp, eflags, cs, eip
	push 	$USER_DS
	push 	%ecx

	# push flags
	# todo: evaluate whether this is actually correct?
	pushf
	# Enable interrupts
	pop 	%eax
	or 		$0x200, %eax  # the 0x200 is just 1 bit set in binary. We use it to enable interrupts.
	push 	%eax

	push 	$USER_CS
	push 	%ebx
	iret
