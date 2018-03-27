ASMIUM = ../asmium
TESTS = general64

TEST_TARGETS = $(addsuffix .test, $(TESTS))

test:
	make $(TEST_TARGETS)

clean:
	-rm *.bin *.o *_hex.txt

%.test : %_hex.txt %_hex_expected.txt Makefile
	@diff -u $*_hex_expected.txt $*_hex.txt && echo "PASS $*"

%_hex.txt : %.s Makefile
	$(ASMIUM) --hex -o $*_hex.txt $*.s

