LTO=1
include ../../llvm-c33.makefile

CFLAGS := -O2 -Wall -DREG $(CFLAGS)
CFLAGS_FASTRUN := $(CFLAGS) -include fastrun_prefix.h
CFLAGS_TURBO := $(CFLAGS) -DTURBO
LDFLAGS := -O2 -Wall $(LDFLAGS)

all: dhry1.pex dhry2.pex dhry2t.pex

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


dhry2t.pex: dhry2t.elf
	$(PPACK) -e -n"Dhry 2 (turbo)" $< -o$@	
dhry2t.elf: main_t.o pass1_f.o pass2_f.o turboctl.o c33209.o
	$(LD) $(LDFLAGS) -Wl,-Map,dhry2t.map -o $@ $^

main_t.o: main.c dry.h turboctl.h
	$(CC) -c $(CFLAGS_TURBO) -o $@ $<
turboctl.o: turboctl.c turboctl.h
c33209.o: c33209.c

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f *.o
