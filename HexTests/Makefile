ASMIUM = ../asmium
TESTS = general64

TEST_TARGETS = $(addsuffix .test, $(TESTS))

test:
	make $(TEST_TARGETS)

clean:
	-rm *.bin *.o *.hex

%.test : %.hex %_expected.hex Makefile
	@diff -u $*_expected.hex $*.hex && echo "PASS $*"

%.hex : %.s Makefile
	$(ASMIUM) --hex -o $*.hex $*.s

