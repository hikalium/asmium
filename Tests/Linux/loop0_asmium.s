:main
	edi = 0
:loop
	++edi
	10 ? edi
	jne :loop
	rax = 0x2000001
	syscall
	retq

