.data
.align 4
syscall_jump_table:
	.long 0
	.long syscall_halt
	.long syscall_execute
	.long syscall_read
	.long syscall_write
	.long syscall_open
	.long syscall_close
	.long syscall_getargs
	.long syscall_vidmap
	.long syscall_set_handler
	.long syscall_sigreturn

.text
.globl syscall_handler_wrapper
syscall_handler_wrapper:
	# On the stack when this is called:
	# ss, esp0, eflags, cs, eip

	# syscall num check
	cmpl $1, %eax
	jl invalid_syscall_num
	cmpl $10, %eax
	jg invalid_syscall_num

	# Save general purpose registers
	# Order of values goes like this on the stack (high -> low):
	# ss, esp0, eflags, cs, eip, ebp, edi, esi, edx, ecx, ebx
	push %ebp
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	cld
	call *syscall_jump_table(, %eax, 4)
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	pop %ebp
	iret

	invalid_syscall_num:
	mov -1, %eax
	iret
