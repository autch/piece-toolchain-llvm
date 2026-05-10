C33HOME = $(PWD)/../../
SDK=$(C33HOME)/sdk
BIN=$(C33HOME)/build/bin
SYSROOT=$(C33HOME)/sysroot/s1c33-none-elf

CC=wine pcc33
CFLAGS=-v -O0 -Wall
LD=wine pcc33
LDFLAGS=-v -ls -lm
PPACK=wine ppack

all: hello33.pex

hello33.pex: hello.srf
	$(PPACK) -e -n"hello@gcc33" $< -o$@
hello.srf: hello.o
	$(LD) $(LDFLAGS) $^
hello.o: hello.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f hello.o hello.srf hello33.pex hello.map hello.sym hello.ms hello.ps hello.\$ __tmp__.cm
