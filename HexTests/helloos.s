.bits 16
	jmp 0x4e
	nop
.asciinz	"HELLOIPL"
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
.asciinz	"HELLO-OS"
.asciinz	"FAT12"
.data8		0 0 0
.data32		0 0 0 0
.data16		0

.offset		0x50
	ax = 0
	ss = ax
	sp = 0x7c00
	ds = ax
	es = ax
	si = 0x7c74
	al = [ si ]
#.data8x		8a 04
.data8		0x83 0xc6 0x01
.data8		0x3c 0x00
.data8		0x74 0x09
.data8		0xb4 0x0e
	bx = 15
	int 0x10
.data8		0xeb 0xee 
	hlt
	jmp -3

.data8		0x0a 0x0a
.asciinz	"HELLO"
.data8		0x20
.asciinz	"ASMIUM"
.data8		0x0a 0x00

.offset		0x1fe
.data8		0x55 0xaa

# Cluster0: 0xff0 (3.5inch FD)
// Cluster1: 0xfff (Reserved)
.offset		0x200
.data8		0xf0 0xff 0xff

/*
/*Cluster0: 0xff0 (3.5inch FD)
Cluster1: 0xfff (Reserved)*/
*/
.offset		0x1400
.data8		0xf0 0xff 0xff
