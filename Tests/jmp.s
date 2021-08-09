
.global _main

_main:
jmp loop
movq     $0x2000001, %rax
loop:
movl	$0, %edi
syscall
retq

