TESTS = exit0

TEST_TARGETS = $(addsuffix .diff, $(TESTS))
TEST_ASMIUM_OUT = $(addsuffix _asmium.o, $(TESTS))
TEST_S_OUT = $(addsuffix .o, $(TESTS))

default: $(TEST_ASMIUM_OUT) $(TEST_S_OUT) $(TEST_TARGETS)

%.diff : %_asmium.o.hex %.o.hex Makefile
	-diff -u $*_asmium.o.hex $*.o.hex

%.hex : % Makefile
	hexdump $* > $*.hex

%_asmium.o : %_asmium.s Makefile
	../asmium $*_asmium.s | xxd -r -p > $*_asmium.o

%.s.o : %.s Makefile
	gcc -o $*.o $*.s
