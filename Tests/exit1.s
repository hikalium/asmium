
.global _main
.text

_main:
movq     $0x2000001, %rax
movq     $0, %rdx
syscall
retq

