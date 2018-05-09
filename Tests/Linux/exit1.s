
.global main
.text

main:
movq     $60, %rax	# exit
movq     $1, %rdi	# exit code = 1
syscall
retq

