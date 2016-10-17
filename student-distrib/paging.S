.globl enable_paging
enable_paging:
	push %esp
	mov %esp, %ebp

	# Page directory passed in eax
	mov %eax, %cr3

	# Set Page Size Extensions bit / 4MB pages
	mov %cr4, %ebx
	or 	$0x10, %ebx
	mov %ebx, %cr4

	# Set Paging Enable bit / enable paging
	mov %cr0, %ebx
	or  $0x80000000, %ebx
	mov %ebx, %cr0

	leave
	ret