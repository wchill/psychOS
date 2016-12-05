# Reference: https://littleosbook.github.io/#creating-a-generic-interrupt-handler

# The only difference between an interrupt handler and a trap handler is that the interrupt handler 
# disables interrupts, which means you cannot get an interrupt while at the same time handling an interrupt.
# We will use trap handlers and disable interrupts manually when we need to.

.align 4  # aligns on 4 byte entries

# common_exception_wrapper
# Does bookkeeping and calls exception_handler to handle one of 32 Exceptions
common_exception_wrapper:
	# Push all general purpose registers (4x8 = 32 bytes)
	# Order of values goes like this (high -> low):
	# eflags, cs, eip, int_num, error, ebp, esp, ..., eax

	# Rodney: eflags, cs, eip already on stack. int_num, error, were pushed by the handler (in macro functions below) that called this code.
	push %esp
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	push %eax
	cld
	call exception_handler
	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	pop %esp
	add $8, %esp  # pops 2 arguments off the stack (int_num and error)
	iret


# null_interrupt_handler
# Does nothing
.globl null_interrupt_handler
null_interrupt_handler:
	iret

# Macro: interrupt_handler_X
# Handler for exceptions that don't have an error code
.macro no_error_code_exception_handler num
.globl interrupt_handler_\num
interrupt_handler_\num:
	push 	$0
	push 	$\num
	jmp common_exception_wrapper
.endm

# Macro: interrupt_handler_X
# Handler for exceptions that have an error code
.macro error_code_exception_handler num
.globl interrupt_handler_\num
interrupt_handler_\num:
	push 	$\num
	jmp common_exception_wrapper
.endm

# Macro for device interrupt handlers.
.macro interrupt_handler func_name
.globl \func_name\()_wrapper
\func_name\()_wrapper:
	# Push all general purpose registers (4x8 = 32 bytes)
	# Order of values goes like this (high -> low):
	# eflags, cs, eip, ebp, esp, ..., eax
	push %esp
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	push %eax
	cld
	call \func_name
	pop %eax
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	pop %esp
	iret
.endm

no_error_code_exception_handler 0
no_error_code_exception_handler 1
no_error_code_exception_handler 2
no_error_code_exception_handler 3
no_error_code_exception_handler 4
no_error_code_exception_handler 5
no_error_code_exception_handler 6
no_error_code_exception_handler 7
error_code_exception_handler 	8
no_error_code_exception_handler 9
error_code_exception_handler 	10
error_code_exception_handler 	11
error_code_exception_handler 	12
error_code_exception_handler 	13
error_code_exception_handler 	14
no_error_code_exception_handler 15
no_error_code_exception_handler 16
error_code_exception_handler 	17
no_error_code_exception_handler 18
no_error_code_exception_handler 19
no_error_code_exception_handler 20
no_error_code_exception_handler 21
no_error_code_exception_handler 22
no_error_code_exception_handler 23
no_error_code_exception_handler 24
no_error_code_exception_handler 25
no_error_code_exception_handler 26
no_error_code_exception_handler 27
no_error_code_exception_handler 28
no_error_code_exception_handler 29
error_code_exception_handler 	30
no_error_code_exception_handler 31

interrupt_handler keyboard_handler
interrupt_handler rtc_handler
interrupt_handler pit_handler
