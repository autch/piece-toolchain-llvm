include ../../llvm-c33.makefile

CFLAGS := -s -O2 -flto=full -Wall -DFROM="\"LLVM\"" $(CFLAGS)
LDFLAGS := -g -O2 -flto=full -Wl,-Map,hello.map $(LDFLAGS)

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
