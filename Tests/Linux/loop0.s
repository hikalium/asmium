.global main
main:
	movl $0, %edi
loop:
	incl %edi
	cmpl $10, %edi
	jne loop
	movq $0x2000001, %rax
	syscall
	retq

