include ../../llvm-c33.makefile

# -ffreestanding so the compiler does not assume a hosted libc; this also
# prevents it from rewriting our rand/srand calls into hosted-libc builtins.
CFLAGS := -O2 -ffreestanding -Wall $(CFLAGS)

LIBOBJS = rand.o assert.o

all: lib install
lib: libpceshim.a

libpceshim.a: $(LIBOBJS)
	$(AR) -rcu $@ $^

clean:
	rm -f *.o libpceshim.a

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(SYSROOT)/lib/libpceshim.a

$(SYSROOT)/lib/libpceshim.a: libpceshim.a
	install -D -m 644 $< $@
