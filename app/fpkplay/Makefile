include ../../llvm-c33.makefile

CFLAGS := -O1 $(CFLAGS)

all: fpkplay_.pex fpkplay.s

fpkplay.elf: fpkplay.o decode.o fpk.o 
	$(LD) $(CFLAGS) -Wl,-Map,fpkplay.map -o $@ $^ -lmuslib

fpkplay.o: fpkplay.c fpack.h lzss.h
decode.o: decode.c lzss.h
fpk.o: fpk.c fpk.h

pex: fpkplay.pex
fpkplay_.pex: fpkplay.elf
	$(PPACK) -e -v -n"FPACK player@llvm" $< -o$@

fpkplay.s: fpkplay.c
	$(CC) $(CFLAGS) -S -o $@ $<

decode_all.s: decodeO0.s decodeO1.s decodeO2.s

decodeO0.s: decode.c
	$(CC) -fno-inline -O0 -S -o $@ $<
decodeO1.s: decode.c
	$(CC) -fno-inline -O1 -S -o $@ $<
decodeO2.s: decode.c
	$(CC) -fno-inline -O2 -S -o $@ $<

copy: fpkplay_.pex
	isd =w"$<"
#	isd =w"fpkplay.fpk"


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o fpkplay.elf decodeO*.s
