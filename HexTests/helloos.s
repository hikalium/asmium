.bits 16
	jmp 0x4e
	nop
.asciinz	HELLOIPL
.data16		512
.data8		1
.data16		1
.data8		2
.data16		224
.data16		2880
.data8		0xf0
.data16		9
.data16		18
.data16		2
.data32		0
.data32		2880
.data8		0 0 0x29
.data32		0xffffffff
.asciinz	HELLO-OS
.asciinz	FAT12
.data8		0 0 0
.data32		0 0 0 0
.data16		0

.offset		0x50
.data8x		b8 00 00
.data8x		8e d0
.data8x		bc 00 7c
.data8x		8e d8
.data8x		8e c0
.data8x		be 74 7c
.data8x		8a 04
.data8x		83 c6 01
.data8x		3c 00
.data8x		74 09
.data8x		b4 0e
.data8x		bb 0f 00
	int 0x10
.data8x		eb ee 
	hlt
	jmp -3

.data8x		0a 0a
.asciinz	HELLO
.data8x		20
.asciinz	ASMIUM
.data8x		0a 00

.offset		0x1fe
.data8x		55 aa

.offset		0x200
.data8x		f0 ff ff 00 00 00 00 00

.offset		0x1400
.data8x		f0 ff ff 00 00 00 00 00
