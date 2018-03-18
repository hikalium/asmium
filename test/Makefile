ASMIUM = ../asmium
TESTS = exit0 exit1

TEST_TARGETS = $(addsuffix .diff, $(TESTS))
TEST_ASMIUM_OUT = $(addsuffix _asmium.o, $(TESTS))
TEST_S_OUT = $(addsuffix .o, $(TESTS))
TEST_ASMIUM_BIN = $(addsuffix _asmium, $(TESTS))
TEST_S_BIN = $(TESTS)

default: $(TEST_ASMIUM_OUT) $(TEST_S_OUT) $(TEST_TARGETS) $(TEST_ASMIUM_BIN) $(TEST_S_BIN)

clean:
	-rm $(TEST_ASMIUM_BIN) $(TEST_S_BIN) *.o

%.diff : %_asmium.o.hex %.o.hex Makefile
	-diff -u $*_asmium.o.hex $*.o.hex

%.hex : % Makefile
	hexdump $* > $*.hex

% : %.o Makefile
	gcc -o $* $*.o

%_asmium.o : %_asmium.s $(ASMIUM) Makefile
	$(ASMIUM) $*_asmium.s | xxd -r -p > $*_asmium.o

%.s.o : %.s Makefile
	gcc -o $*.o $*.s

