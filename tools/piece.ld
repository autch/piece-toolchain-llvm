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
 *   IRAM: 0x001000 – 0x001FFF (  4 KB, user area; 0x2000 is stack bottom
 *                                     when _stacklen == 0)
 *
 * Stack & heap placement (runtime, controlled by pceAppHead.stack_size):
 *   _stacklen == 0 (default)
 *     Kernel sets SP = 0x2000; stack descends into IRAM [0x1000, 0x2000).
 *     Kernel sets heap base = bss_end (SRAM).
 *     IRAM placement users must keep .fastdata+.fastbss+.fastrun small
 *     enough that the stack does not descend into __firam_end.
 *   _stacklen > 0
 *     Kernel sets SP = bss_end + stack_size; stack & heap are in SRAM.
 *     IRAM [0x1000, 0x2000) is fully available for placed objects.
 *     Override via `-Wl,--defsym,_stacklen=0x1000` (or similar).
 *
 * Required external symbols (provided by cstart.o / this script):
 *   __START_DEFAULT_BSS  – start of .bss (address)
 *   __END_DEFAULT_BSS    – end of .bss (address, stored in pceAppHead.bss_end)
 *   __SIZEOF_DEFAULT_BSS – size of .bss in bytes (used by memset in cstart)
 *
 * Internal-RAM placement (optional; zero cost when unused):
 *   .fastrun   – hot code, VMA=IRAM, LMA=SRAM, copied by crt0
 *   .fastdata  – hot initialised data, VMA=IRAM, LMA=SRAM, copied by crt0
 *   .fastbss   – hot zero-initialised data, VMA=IRAM, NOLOAD, cleared by crt0
 *   __firam_end – first free IRAM address above placed objects; applications
 *                 that relocate the stack into SRAM (via pceAppHead) can use
 *                 this to decide whether placement still fits.
 *   ASSERT enforces that .fastbss_end stays within the 0x1000–0x2000 user area.
 *
 * Section ordering constraint (IMPORTANT):
 *   .bss must come AFTER .fastdata and .fastrun in SRAM so that
 *   __END_DEFAULT_BSS (reported to the kernel as bss_end, used as the
 *   initial heap base by ResetHeap) sits past the LMAs of .fastdata and
 *   .fastrun.  Otherwise the kernel's ResetHeap(bss_end) writes heap
 *   metadata INTO the .fastrun/.fastdata LMA bytes before crt0 has a
 *   chance to copy them to IRAM, corrupting the placed code at startup.
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
    IRAM (rwx) : ORIGIN = 0x001000, LENGTH = 0x001000
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

    /* C++ static constructors (and __attribute__((constructor))).
     * Clang emits function pointers into .init_array; crt0 iterates them
     * at startup before pceAppInit(). */
    .init_array : ALIGN(4) {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > SRAM

    /* Initialised data — loaded as part of the binary image. */
    .data : ALIGN(4) {
        *(.data .data.*)
        . = ALIGN(4);
    } > SRAM

    /* Internal-RAM placed code/data.  VMA is in IRAM (0x1000-0x2000); LMA
     * sits in SRAM and the bytes ride along in the .pex image.  crt0 copies
     * them into IRAM at startup.  When unused, all sections have size 0 and
     * add no image bytes nor runtime cost.
     *
     * These sections come BEFORE .bss so that their LMAs are covered by the
     * bss_end address reported to the kernel.  See header comment about the
     * ResetHeap(bss_end) LMA-corruption hazard.
     *
     * Order in IRAM is DATA FIRST, CODE LAST:
     *   0x1000 → .fastdata → .fastbss → .fastrun → __firam_end
     * Placing variables at lower addresses than code makes the .fastrun end
     * the one that bumps into the stack top when _stacklen == 0, which fails
     * loudly via stack corruption rather than silently overwriting .fastbss. */
    .fastdata : ALIGN(4) {
        __fastdata_start = .;
        *(.fastdata .fastdata.*)
        . = ALIGN(4);
        __fastdata_end = .;
    } > IRAM AT > SRAM
    __fastdata_load = LOADADDR(.fastdata);

    .fastbss (NOLOAD) : ALIGN(4) {
        __fastbss_start = .;
        *(.fastbss .fastbss.*)
        . = ALIGN(4);
        __fastbss_end = .;
    } > IRAM

    .fastrun : ALIGN(4) {
        __fastrun_start = .;
        *(.fastrun .fastrun.*)
        . = ALIGN(4);
        __fastrun_end = .;
    } > IRAM AT > SRAM
    __fastrun_load = LOADADDR(.fastrun);

    /* First free IRAM byte above placed objects. */
    PROVIDE(__firam_end = __fastrun_end);

    /* BSS — must come AFTER the IRAM LMAs in SRAM so that __END_DEFAULT_BSS
     * (= pceAppHead.bss_end = kernel heap base) is past the LMAs.  Otherwise
     * ResetHeap(bss_end) would write heap metadata into the .fastrun/.fastdata
     * LMA bytes before crt0 copies them to IRAM.  Zero-initialised by
     * pceAppInit00 at startup. */
    .bss (NOLOAD) : ALIGN(4) {
        __START_DEFAULT_BSS = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __END_DEFAULT_BSS = .;
    } > SRAM
    __SIZEOF_DEFAULT_BSS = __END_DEFAULT_BSS - __START_DEFAULT_BSS;
    /* Alias for legacy SDK code that uses pce_app_bss_end as the start of
     * the free heap area (above .bss, below the kernel heap). */
    pce_app_bss_end = __END_DEFAULT_BSS;

    /DISCARD/ : {
        *(.note .note.*)
        *(.comment)
        *(.eh_frame .eh_frame_hdr)
    }
}

/* Hard limit: internal-RAM user area ends at 0x2000 (kernel stack bottom).
 * Placed outside SECTIONS — ld.lld does not accept trailing ASSERT inside. */
ASSERT(__fastrun_end <= 0x2000,
       "Internal RAM overflow: .fastdata/.fastbss/.fastrun exceed user area (0x1000-0x2000)")
