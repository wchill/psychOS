.align 4
common_isr_wrapper:
	# Push all general purpose registers (4x8 = 32 bytes)
	# Order of values goes like this (high -> low):
	# eflags, cs, eip, error, int num, ebp, esp, ..., eax
	push %ebp
	push %esp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	push %eax
	cld
	call interrupt_handler
	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %esp
	pop %ebp
	add $8, %esp
	iret

.macro no_error_code_interrupt_handler num
.globl interrupt_handler_\num
interrupt_handler_\num:
	push 	$0
	push 	$\num
	jmp common_isr_wrapper
.endm

.macro error_code_interrupt_handler num
.globl interrupt_handler_\num
interrupt_handler_\num:
	push 	$\num
	jmp common_isr_wrapper
.endm

no_error_code_interrupt_handler 0
no_error_code_interrupt_handler 1
no_error_code_interrupt_handler 2
no_error_code_interrupt_handler 3
no_error_code_interrupt_handler 4
no_error_code_interrupt_handler 5
no_error_code_interrupt_handler 6
no_error_code_interrupt_handler 7
error_code_interrupt_handler 	8
no_error_code_interrupt_handler 9
error_code_interrupt_handler 	10
error_code_interrupt_handler 	11
error_code_interrupt_handler 	12
error_code_interrupt_handler 	13
error_code_interrupt_handler 	14
no_error_code_interrupt_handler 15
no_error_code_interrupt_handler 16
error_code_interrupt_handler 	17
no_error_code_interrupt_handler 18
no_error_code_interrupt_handler 19
no_error_code_interrupt_handler 20
no_error_code_interrupt_handler 21
no_error_code_interrupt_handler 22
no_error_code_interrupt_handler 23
no_error_code_interrupt_handler 24
no_error_code_interrupt_handler 25
no_error_code_interrupt_handler 26
no_error_code_interrupt_handler 27
no_error_code_interrupt_handler 28
no_error_code_interrupt_handler 29
error_code_interrupt_handler 	30
no_error_code_interrupt_handler 31
