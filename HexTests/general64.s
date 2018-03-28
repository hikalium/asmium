.bits 64
	push rbp
	rbp = rsp
	ecx ^= ebx
	xor eax eax
	pop rbp
	retq
	syscall
