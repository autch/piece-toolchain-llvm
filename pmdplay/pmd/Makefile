include ../../llvm-c33.makefile

SOURCES = $(wildcard *.pmd) 
TARGETS = $(SOURCES:.pmd=.s)
OBJECTS = $(TARGETS:.s=.o)

all: pmd_data.a pmd_data.h

pmd_data.a: $(OBJECTS)
	$(AR) rcs $@ $^

pmd_data.h: $(TARGETS)
	sh pmd2h.sh > $@


%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

%.s: %.pmd
	sh pmd2s.sh $< > $@

clean:
	rm -f $(TARGETS) $(OBJECTS) pmd_data.a pmd_data.h
