_main:
	edi = 0
loop:
	inc edi
	cmp 10 edi
	jnz loop
	rax = 0x2000001
	syscall
	retq

