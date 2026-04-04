include ../llvm-c33.makefile

all: fpkplay_.pex fpkplay.s

fpkplay.elf: fpkplay.o decode.o fpk.o music/muslib.a
	$(LD) $(CFLAGS) -Wl,-Map,fpkplay.map -o $@ $^

music/muslib.a:
	make -C music muslib.a

fpkplay.o: fpkplay.c fpack.h lzss.h
decode.o: decode.c lzss.h
fpk.o: fpk.c fpk.h

pex: fpkplay.pex
fpkplay_.pex: fpkplay.elf
	$(PPACK) -e -v -n"FPACK player@llvm" $< -o$@

fpkplay.s: fpkplay.c
	$(CC) $(CFLAGS) -S -o $@ $<

copy: fpkplay_.pex
	isd =w"$<"
#	isd =w"fpkplay.fpk"


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o fpkplay.elf
	make -C music clean
