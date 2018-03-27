ASMIUM = ../asmium
TESTS = exit0 exit1 loop0

TEST_TARGETS = $(addsuffix .test, $(TESTS))
TEST_ASMIUM_OUT = $(addsuffix _asmium.o, $(TESTS))
TEST_S_OUT = $(addsuffix .o, $(TESTS))
TEST_ASMIUM_BIN = $(addsuffix _asmium.bin, $(TESTS))
TEST_S_BIN = $(addsuffix .bin, $(TESTS))

test: $(TEST_ASMIUM_OUT) $(TEST_S_OUT) $(TEST_ASMIUM_BIN) $(TEST_S_BIN)
	make $(TEST_TARGETS)

clean:
	-rm *.bin *.o

%.test : %_asmium.bin %.bin Makefile
	@- ./$*.bin; ./test.sh ./$*_asmium.bin $$?

%.bin : %.o Makefile
	gcc -o $@ $*.o

%_asmium.o : %_asmium.s $(ASMIUM) Makefile
	$(ASMIUM) -o $*_asmium.o $*_asmium.s

