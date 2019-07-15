SRCS=muimsa.cc
HEADERS=
CFLAGS=-Wall -Wpedantic -std=c++17
CXX=clang++

default: muimsa

muimsa: $(SRCS) $(HEADERS) Makefile
	$(CXX) $(CFLAGS) -o $@ $(SRCS)

test : muimsa
	./muimsa

clean: 
	-rm muimsa
	-rm *.o

format:
	clang-format -i $(SRCS) $(HEADERS)
