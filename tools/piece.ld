/* P/ECE Application Linker Script
 *
 * Target: S1C33209 (Seiko Epson S1C33 Family)
 *
 * The P/ECE kernel loads the application binary into SRAM at 0x100000.
 * The pceAPPHEAD struct must reside at exactly 0x100000; crt0.o
 * provides it in its .rodata section (pceAppHead is static const).
 *
 * Memory map:
 *   SRAM: 0x100000 – 0x13CFFF (244 KB, shared with kernel heap)
 *   Stack: managed by the kernel below 0x002000 (not mapped here)
 *
 * Required external symbols (provided by cstart.o / this script):
 *   __START_DEFAULT_BSS  – start of .bss (address)
 *   __END_DEFAULT_BSS    – end of .bss (address, stored in pceAppHead.bss_end)
 *   __SIZEOF_DEFAULT_BSS – size of .bss in bytes (used by memset in cstart)
 */

/* The P/ECE kernel accesses the application header at 0x100000 directly by
 * address; there is no conventional entry point.  PROVIDE _start so that lld
 * does not emit a "cannot find entry symbol" warning. */
_start = 0x100000;

/* Stack size reported to the kernel via pceAppHead.stack_size.
 * The value 0 means "use kernel default".  Applications may override this
 * by defining _stacklen as an absolute symbol before linking. */
PROVIDE(_stacklen = 0);

MEMORY {
    SRAM (rwx) : ORIGIN = 0x100000, LENGTH = 0x3D000
}

SECTIONS {
    . = 0x100000;

    /* Text and read-only data.
     * cstart.o's .rodata comes first so that pceAppHead (static const) lands
     * at exactly 0x100000.  cstart.o's .text (wrapper functions) follows,
     * then all other text and rodata. */
    .text : {
        KEEP(*crt0.o(.rodata .rodata.*))
        KEEP(*crt0.o(.text .text.*))
        *(.text .text.*)
        . = ALIGN(4);
        *(.rodata .rodata.*)
        . = ALIGN(4);
    } > SRAM

    /* Initialised data — loaded as part of the binary image. */
    .data : ALIGN(4) {
        *(.data .data.*)
        . = ALIGN(4);
    } > SRAM

    /* BSS — zero-initialised by pceAppInit00 at startup via memset. */
    __START_DEFAULT_BSS = .;
    .bss (NOLOAD) : ALIGN(4) {
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
    } > SRAM
    __END_DEFAULT_BSS = .;
    __SIZEOF_DEFAULT_BSS = __END_DEFAULT_BSS - __START_DEFAULT_BSS;

    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame .eh_frame_hdr)
    }
}
