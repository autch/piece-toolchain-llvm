include ../llvm-c33.makefile

all: pcecircle.pex

pcecircle.elf: pcecircle.o pceLCDCircle.o pceLCDEllipse.o
	$(LD) $(CFLAGS) -Wl,-Map,pcecircle.map -o $@ $^

pcericle.o: pcericle.c pcecircle.h

pex: pcecircle.pex
pcecircle.pex: pcecircle.elf
	$(PPACK) -e -v -n"pcecircle@llvm" $< -o$@

copy: pcecircle.pex
	isd =w"$<"
#	isd =w"pmdplay.fpk"


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o pcecircle.elf
