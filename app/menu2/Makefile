#NO_LTO=1
include ../../llvm-c33.makefile
CFLAGS := -s -O1 $(CFLAGS)

all: startup.pex

startup.elf: menu.o os.o launch.o
	$(LD) $(CFLAGS) -Wl,-Map,startup.map -o $@ $^

menu.o: menu.c launch.h
os.o: os.c launch.h
launch.o: launch.c launch.h


pex: startup.pex
startup.pex: startup.elf
	$(PPACK) -e -v -n"SystemMenu@llvm" -ip.pid $< -o$@

copy: startup.pex
	isd =w"$<"


%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o startup.elf
