SRCS=asmium.c tokenizer.c gen_macho.c gen_elf64.c
HEADERS=asmium.h
CFLAGS=-Wall -Wpedantic

default: asmium

asmium: $(SRCS) $(HEADERS) Makefile
	$(CC) $(CFLAGS) -o asmium $(SRCS)

test : asmium
	make -C Tests/
	make -C HexTests/

clean: 
	-rm asmium
	-rm testbin
	-rm *.o

run: asmium
	./asmium testbin.s -o testbin.o
	make testbin
	./testbin

format:
	clang-format -i $(SRCS)
