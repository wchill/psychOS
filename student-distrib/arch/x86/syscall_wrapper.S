.data
syscall_jump_table:
.long syscall_halt, syscall_execute, syscall_read, syscall_write, syscall_open, syscall_close, syscall_getargs, syscall_vidmap, syscall_set_handler, syscall_sigreturn

.text
.globl syscall_handler_wrapper
syscall_handler_wrapper:
	# On the stack when this is called:
	# ss, esp0, eflags, cs, eip

	# syscall num check
	cmpl $0, %eax
	jl invalid_syscall_num
	cmpl $9, %eax
	jg invalid_syscall_num

	# Save general purpose registers
	# Order of values goes like this on the stack (high -> low):
	# ss, esp (process user-mode), eflags, cs, eip, edi, esi, edx, ecx, ebx, eax
	push %edi
	push %esi
	push %edx
	push %ecx
	push %ebx
	mov 32(%esp), %ebx
	push %ebx
	cld
	call *syscall_jump_table(, %eax, 4)
	add $4, %esp
	pop %ebx
	pop %ecx
	pop %edx
	pop %esi
	pop %edi
	ret

	invalid_syscall_num:
	mov -1, %eax
	ret
