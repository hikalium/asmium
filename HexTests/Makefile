ASMIUM = ../asmium
TESTS = general64 helloos

TEST_TARGETS = $(addsuffix .test, $(TESTS))

test:
	make $(TEST_TARGETS)

.FORCE:

$(ASMIUM): .FORCE
	make -C ..


clean:
	-rm *.bin *.o *_hex.txt

%.test : %_hex.txt %_hex_expected.txt Makefile
	@diff -u $*_hex_expected.txt $*_hex.txt && echo "PASS $*"

%_hex.txt : %.s Makefile $(ASMIUM)
	$(ASMIUM) --hex -o $*_hex.txt $*.s

run: helloos_hex.txt
	cat helloos_hex.txt | xxd -r -p > helloos.img
	qemu-system-x86_64 -monitor stdio helloos.img
