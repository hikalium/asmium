SRCS=asmium.c

default: asmium

asmium: $(SRCS) Makefile
	$(CC) $(CFLAGS) -o asmium $(SRCS)

%.o : %.asm asmium
	./asmium $*.asm | xxd -r -p > $*.o

test : exit0
	./exit0

clean: 
	-rm asmium
