.data
.align 4
syscall_jump_table:
.long syscall_halt, syscall_execute, syscall_read, syscall_write, syscall_open, syscall_close, syscall_getargs, syscall_vidmap, syscall_set_handler, syscall_sigreturn

.text
.globl syscall_handler_wrapper
syscall_handler_wrapper:
	# On the stack when this is called:
	# ss, esp0, eflags, cs, eip

	dec %eax

	# syscall num check
	cmpl $0, %eax
	jl invalid_syscall_num
	cmpl $9, %eax
	jg invalid_syscall_num

	# Save general purpose registers
	# Order of values goes like this on the stack (high -> low):
	# ss, esp0, eflags, cs, eip, ebp, edi, esi, edx, ecx, ebx, esp0
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
