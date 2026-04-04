include ../llvm-c33.makefile

all: cpptest.pex ctortest.pex virttest.pex virptest.pex cpptest2.pex heaptest.pex

cpptest.elf: cpptest.o
	$(LD) $(CFLAGS) -Wl,-Map,cpptest.map -o $@ $^
ctortest.elf: ctortest.o
	$(LD) $(CFLAGS) -Wl,-Map,ctortest.map -o $@ $^
virttest.elf: virttest.o
	$(LD) $(CFLAGS) -Wl,-Map,virttest.map -o $@ $^
virptest.elf: virptest.o
	$(LD) $(CFLAGS) -Wl,-Map,virptest.map -o $@ $^
cpptest2.elf: cpptest2.o
	$(LD) $(CFLAGS) -Wl,-Map,cpptest2.map -o $@ $^
heaptest.elf: heaptest.o itoa.o
	$(LD) $(CFLAGS) -Wl,-Map,heaptest.map -o $@ $^

cpptest.o: cpptest.cpp
ctortest.o: ctortest.cpp
virttest.o: virttest.cpp
virptest.o: virptest.cpp
cpptest2.o: cpptest2.cpp
heaptest.o: heaptest.cpp itoa.h

itoa.o: itoa.c itoa.h

pex: cpptest.pex ctortest.pex virttest.pex virptest.pex cpptest2.pex heaptest.pex

cpptest.pex: cpptest.elf
	$(PPACK) -e -v -n"C++ test" $< -o$@
ctortest.pex: ctortest.elf
	$(PPACK) -e -v -n"C++ ctor test" $< -o$@
virttest.pex: virttest.elf
	$(PPACK) -e -v -n"C++ virtual test" $< -o$@
virptest.pex: virptest.elf
	$(PPACK) -e -v -n"C++ vdispatch test" $< -o$@
cpptest2.pex: cpptest2.elf
	$(PPACK) -e -v -n"C++ test 2" $< -o$@
heaptest.pex: heaptest.elf
	$(PPACK) -e -v -n"C++ heap test" $< -o$@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.elf
