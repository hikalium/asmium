SRCS=asmium.c

default: asmium

asmium: $(SRCS) Makefile
	$(CC) $(CFLAGS) -o asmium $(SRCS)

test :
	make -C Tests/

clean: 
	-rm asmium
	-rm testbin
	-rm *.o


run: asmium
	./asmium testbin.s -o testbin.o
	make testbin
	./testbin
