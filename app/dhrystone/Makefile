LTO=1
include ../../llvm-c33.makefile

CFLAGS := -O2 -Wall -DREG $(CFLAGS)
CFLAGS_FASTRUN := $(CFLAGS) -include fastrun_prefix.h
LDFLAGS := -O2 $(LDFLAGS)

all: dhry1.pex dhry2.pex

dhry1.pex: dhry1.elf
	$(PPACK) -e -n"Dhrystone 1" $< -o$@
dhry1.elf: main.o pass1.o pass2.o
	$(LD) $(LDFLAGS) -Wl,-Map,dhry1.map -o $@ $^

dhry2.pex: dhry2.elf
	$(PPACK) -e -n"Dhrystone 2" $< -o$@
dhry2.elf: main.o pass1_f.o pass2_f.o
	$(LD) $(LDFLAGS) -Wl,-Map,dhry2.map -o $@ $^

main.o: main.c dry.h
pass1.o: pass1.c dry.h
pass2.o: pass2.c dry.h

pass1_f.o: pass1.c dry.h
	$(CC) -c $(CFLAGS_FASTRUN) -o $@ $<
pass2_f.o: pass2.c dry.h
	$(CC) -c $(CFLAGS_FASTRUN) -o $@ $<


%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o dhry1.elf dhry1.pex dhry2.elf dhry2.pex

