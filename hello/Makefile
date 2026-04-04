C33HOME = $(PWD)/../
SDK=$(C33HOME)/sdk
BIN=$(C33HOME)/build/bin
SYSROOT=$(C33HOME)/sysroot/s1c33-none-elf

CC=$(BIN)/clang --sysroot=$(SYSROOT)
CFLAGS=-s -O2 -flto=full -Wall -DFROM="\"LLVM\""
LD=$(BIN)/clang --sysroot=$(SYSROOT)
LDFLAGS=-g -O2 -flto=full -Wl,-Map,hello.map
PPACK=$(C33HOME)/tools/ppack/ppack

all: hello_l.pex

hello_l.pex: hello.elf
	$(PPACK) -e -n"hello@LLVM" $< -o$@
hello.elf: hello.o
	$(LD) $(LDFLAGS) -o $@ $^
hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@

hello.s: hello.c
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -f hello.o hello.elf hello_l.pex hello.map hello.i hello.s hello.bc
