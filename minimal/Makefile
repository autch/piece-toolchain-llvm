C33HOME = $(PWD)/../
SDK=$(C33HOME)/sdk
BIN=$(C33HOME)/build/bin
SYSROOT=$(C33HOME)/sysroot/s1c33-none-elf

CC=$(BIN)/clang --sysroot=$(SYSROOT)
CFLAGS=-O1 -Wall
LD=$(BIN)/clang --sysroot=$(SYSROOT)
LDFLAGS=-v -g -Wl,-Map,minimal.map
PPACK=$(C33HOME)/tools/ppack/ppack

all: mini_l.pex

mini_l.pex: minimal.elf
	$(PPACK) -e -n"mini@LLVM" $< -o$@
minimal.elf: minimal.o
	$(LD) $(LDFLAGS) -o $@ $^
minimal.o: minimal.c
	$(CC) $(CFLAGS) -c $< -o $@

minimal.s: minimal.c
	$(CC) $(CFLAGS) -S $< -o $@

clean:
	rm -f minimal.o minimal.elf mini_l.pex minimal.map minimal.i minimal.s minimal.bc
