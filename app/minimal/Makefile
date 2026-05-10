include ../../llvm-c33.makefile

CFLAGS := -O1 -Wall $(CFLAGS)
LDFLAGS := -v -g -Wl,-Map,minimal.map $(LDFLAGS)

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
