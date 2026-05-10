/*
 * ppack.c — PIECE File Packager (ELF input variant)
 *
 * Based on ppack.c by MIO.H (OeRSTED), Copyright (C)2001 AUQAPLUS Co., Ltd.
 * ELF32 input support added for the LLVM S1C33 toolchain.
 *
 * Usage:
 *   ppack -e hello.elf -ohello.pex -n"Hello World" -iicon.pid
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#ifdef PPACK_HAVE_ICONV
#  include <iconv.h>
#endif

/* -------------------------------------------------------------------------
 * ELF32 structures (little-endian, S1C33 = EM_SE_C33 = 107)
 * ------------------------------------------------------------------------- */

#define ELF_MAGIC0  0x7f
#define ELF_MAGIC1  'E'
#define ELF_MAGIC2  'L'
#define ELF_MAGIC3  'F'

#define ET_EXEC     2
#define ET_DYN      3
#define EM_SE_C33   107

#define PT_LOAD         1

#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

/* -------------------------------------------------------------------------
 * pffsFileHEADER — P/ECE filesystem executable header
 * (from sdk/include/piece.h)
 * ------------------------------------------------------------------------- */

#pragma pack(push, 1)
typedef struct {
    uint8_t  mark;        /* +0  always 'X' */
    uint8_t  type;        /* +1  PFFS_FT_EXE2 = 0x02 */
    uint16_t ofs_data;    /* +2  offset to compressed data from start of header */
    uint16_t ofs_name;    /* +4  offset to name string */
    uint16_t ofs_icon;    /* +6  offset to icon (0 if none) */
    uint32_t resv2;       /* +8  reserved */
    uint32_t top_adrs;    /* +12 load address */
    uint32_t length;      /* +16 compressed data length */
    uint32_t crc32;       /* +20 CRC32 of compressed data */
} pffsFileHEADER;
#pragma pack(pop)

#define PFFS_FT_EXE2  0x02

/* -------------------------------------------------------------------------
 * Globals
 * ------------------------------------------------------------------------- */

static char *infname;
static char *outfname;

static unsigned long bin_adrs = 0;

/* CRC from crc32.c */
unsigned long calcrc(unsigned char *c, unsigned n);
void calcrc_init(void);

/* zlib wrapper from iz.c */
unsigned zlbencode(unsigned char *inptr, unsigned size, unsigned char *code);
unsigned zlbdecode(unsigned char *inptr, unsigned size, unsigned char *data);

static char cmd_mode;
int fverbose;
int fwait;
int fmethod = 0;

char *store_name;
char DataPath[4096];
char FileName[25];
char IconName[256];

unsigned char databuff[1024 * 1024];
unsigned long topadrs;
unsigned long endadrs;

#define OUTBUFSIZ sizeof(databuff)

static char usage[] =
    "PPACK ... PIECE File Packager (ELF edition) ver1.00\n"
    "usage:\n"
    "  for encode: ppack -e [options] input.elf -ooutput.pex\n"
    "  for decode: ppack -d [options] input.pex -ooutput\n"
    "  for test:   ppack -t [options] input.pex\n"
    "options:\n"
    "   -v      : verbose\n"
    "   -bhhhh  : binary mode (raw binary at address hhhh)\n"
    "   -n<name>: caption, raw bytes (up to 24 bytes; pass SJIS directly)\n"
#ifdef PPACK_HAVE_ICONV
    "   -N<name>: caption, UTF-8 input transcoded to CP932 (up to 24 bytes)\n"
#endif
    "   -i<file>: icon image file (.pid, 256 bytes)\n"
    "   -k      : key wait\n"
    ;

#ifdef PPACK_HAVE_ICONV
/* -------------------------------------------------------------------------
 * utf8_to_cp932 — transcode caption from UTF-8 to CP932 (Shift_JIS).
 *
 * The pex file stores the caption as raw CP932 bytes (the BIOS font renderer
 * reads them directly without transcoding).  Writing those bytes literally in
 * a Makefile forces the Makefile itself to be saved as SJIS, which mangles
 * the caption when the file is viewed in a UTF-8 terminal.  This helper lets
 * callers pass UTF-8 on the command line and have ppack convert it.
 *
 * Only compiled when iconv is available (POSIX systems, MinGW/MSYS2 with
 * libiconv).  On platforms without iconv (e.g. plain MSVC builds), -N is
 * disabled and users should pass raw CP932 bytes via -n instead.
 *
 * Returns 0 on success.  On failure, prints a diagnostic and returns -1.
 * dst is always NUL-terminated on success.
 * ------------------------------------------------------------------------- */

static int utf8_to_cp932(const char *src, char *dst, size_t dst_size)
{
    iconv_t cd = iconv_open("CP932", "UTF-8");
    if (cd == (iconv_t)-1) {
        fprintf(stderr, "ppack: iconv_open(CP932<-UTF-8) failed: %s\n",
                strerror(errno));
        return -1;
    }

    char *inbuf = (char *)src;
    size_t inbytesleft = strlen(src);
    char *outbuf = dst;
    size_t outbytesleft = dst_size - 1;  /* leave room for NUL */

    size_t r = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    int saved_errno = errno;
    iconv_close(cd);

    if (r == (size_t)-1) {
        if (saved_errno == E2BIG) {
            fprintf(stderr,
                "ppack: caption too long after CP932 conversion (limit %zu bytes)\n",
                dst_size - 1);
        } else if (saved_errno == EILSEQ) {
            fprintf(stderr,
                "ppack: caption contains a character that cannot be represented in CP932\n");
        } else if (saved_errno == EINVAL) {
            fprintf(stderr, "ppack: caption ends with an incomplete UTF-8 sequence\n");
        } else {
            fprintf(stderr, "ppack: iconv failed: %s\n", strerror(saved_errno));
        }
        return -1;
    }

    *outbuf = '\0';
    return 0;
}
#endif /* PPACK_HAVE_ICONV */

/* -------------------------------------------------------------------------
 * readfile_elf — load PT_LOAD segments into databuff[] by LMA
 *
 * Uses program headers (not section headers) so that sections with LMA != VMA
 * (e.g. .fastrun / .fastdata placed in internal RAM via `AT > SRAM`) land at
 * their ROM/SRAM storage address, not their runtime IRAM address.  This is
 * the same scheme llvm-objcopy -O binary uses.
 *
 *   - databuff[] is pre-filled with 0xff
 *   - for each PT_LOAD segment, p_filesz bytes from p_offset are copied to
 *     p_paddr (LMA)
 *   - segments with p_filesz == 0 (pure BSS) are skipped; crt0 clears BSS
 *     at runtime, so no need to bloat the compressed payload
 *   - topadrs = lowest p_paddr among loaded segments
 *   - endadrs = highest p_paddr + p_filesz
 * ------------------------------------------------------------------------- */

int readfile_elf(FILE *fp)
{
    Elf32_Ehdr ehdr;
    Elf32_Phdr phdr;
    unsigned int i;

    fseek(fp, 0, SEEK_SET);
    if (fread(&ehdr, 1, sizeof(ehdr), fp) != sizeof(ehdr)) {
        fprintf(stderr, "ppack: cannot read ELF header\n");
        return 1;
    }

    /* Magic check */
    if (ehdr.e_ident[0] != ELF_MAGIC0 || ehdr.e_ident[1] != ELF_MAGIC1 ||
        ehdr.e_ident[2] != ELF_MAGIC2 || ehdr.e_ident[3] != ELF_MAGIC3) {
        fprintf(stderr, "ppack: not an ELF file\n");
        return 1;
    }

    /* Machine check */
    if (ehdr.e_machine != EM_SE_C33) {
        fprintf(stderr, "ppack: e_machine=%u, expected %u (EM_SE_C33)\n",
                ehdr.e_machine, EM_SE_C33);
        return 1;
    }

    if (ehdr.e_phoff == 0 || ehdr.e_phnum == 0) {
        fprintf(stderr, "ppack: ELF has no program headers\n");
        return 1;
    }

    /* First pass: find the lowest LMA to anchor databuff[] */
    for (i = 0; i < ehdr.e_phnum; i++) {
        uint32_t off = ehdr.e_phoff + i * ehdr.e_phentsize;
        fseek(fp, off, SEEK_SET);
        if (fread(&phdr, 1, sizeof(phdr), fp) != sizeof(phdr))
            break;
        if (phdr.p_type != PT_LOAD) continue;
        if (phdr.p_filesz == 0) continue;
        if (topadrs == 0 || phdr.p_paddr < topadrs)
            topadrs = phdr.p_paddr;
    }

    if (topadrs == 0) {
        fprintf(stderr, "ppack: no loadable segments found\n");
        return 1;
    }

    printf("\n");

    /* Second pass: copy each PT_LOAD segment's file bytes to databuff. */
    for (i = 0; i < ehdr.e_phnum; i++) {
        uint32_t off = ehdr.e_phoff + i * ehdr.e_phentsize;
        fseek(fp, off, SEEK_SET);
        if (fread(&phdr, 1, sizeof(phdr), fp) != sizeof(phdr))
            break;

        if (phdr.p_type != PT_LOAD) continue;
        if (phdr.p_filesz == 0) continue;

        uint32_t adr  = phdr.p_paddr;
        uint32_t size = phdr.p_filesz;

        if (adr < topadrs) {
            fprintf(stderr, "ppack: segment LMA 0x%08x below topadrs 0x%08x\n",
                    adr, (unsigned)topadrs);
            return 1;
        }
        if (adr + size - topadrs >= sizeof(databuff)) {
            fprintf(stderr, "ppack: segment [%u] at 0x%08x+0x%x overflows buffer\n",
                    i, adr, size);
            return 1;
        }

        printf("  %08x-%08x: %s (vaddr=%08x)\n",
               adr, adr + size - 1,
               (phdr.p_flags & PF_X) ? "CODE" : "DATA",
               phdr.p_vaddr);

        fseek(fp, phdr.p_offset, SEEK_SET);
        fread(databuff + (adr - topadrs), 1, size, fp);

        uint32_t end = adr + size;
        if (end > endadrs)
            endadrs = end;
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * arcs — compress databuff[0..endadrs-topadrs] in-place
 *
 * Layout after arcs():
 *   databuff[0..3]   = original length (LE32)
 *   databuff[4..7]   = CRC32 of original data
 *   databuff[8..]    = zlib-deflated data
 * ------------------------------------------------------------------------- */

void arcs(void)
{
    unsigned len = endadrs - topadrs;
    unsigned char *orgbuff = malloc(len);

    memcpy(orgbuff, databuff, len);

    calcrc_init();
    ((uint32_t *)databuff)[0] = len;
    ((uint32_t *)databuff)[1] = calcrc(orgbuff, len);

    len = zlbencode(orgbuff, len, databuff + 8);

    free(orgbuff);

    endadrs = topadrs + 8 + len;
}

/* -------------------------------------------------------------------------
 * readfile — dispatcher: ELF or raw binary
 * ------------------------------------------------------------------------- */

int readfile(char *infile)
{
    int err = 1;
    FILE *fp;

    memset(databuff, 0xff, sizeof(databuff));
    topadrs = 0;
    endadrs = 0;

    fp = fopen(infile, "rb");
    if (fp != NULL) {
        if (!bin_adrs) {
            err = readfile_elf(fp);
        } else {
            topadrs = bin_adrs;
            endadrs = topadrs + fread(databuff, 1, sizeof(databuff), fp);
            err = 0;
        }
        fclose(fp);
    } else {
        perror(infile);
    }

    return err;
}

/* -------------------------------------------------------------------------
 * copyicon — append 256-byte .pid icon to output file
 * ------------------------------------------------------------------------- */

void copyicon(FILE *fp)
{
    FILE *ifp = fopen(IconName, "rb");
    if (ifp) {
        unsigned char icon[256];
        fread(icon, 1, sizeof(icon), ifp);
        fclose(ifp);
        fwrite(icon, 1, sizeof(icon), fp);
    }
}

/* -------------------------------------------------------------------------
 * encode_pack — read ELF, compress, write .pex
 * ------------------------------------------------------------------------- */

void encode_pack(char *infile, char *outfile)
{
    FILE *fp = fopen(outfile, "wb");

    if (fp && !readfile(infile)) {
        pffsFileHEADER fh;
        int fnc = strlen(FileName);
        int icc = (*IconName) ? 256 : 0;

        fnc = (fnc + 1 + 3) & ~3;   /* round up to 4-byte boundary */
        arcs();

        calcrc_init();
        printf("%x - %x\n", (unsigned)topadrs, (unsigned)endadrs);

        fh.mark     = 'X';
        fh.type     = PFFS_FT_EXE2;
        fh.ofs_data = sizeof(fh) + fnc + icc;
        fh.ofs_name = sizeof(fh);
        fh.ofs_icon = icc ? (uint16_t)(sizeof(fh) + fnc) : 0;
        fh.resv2    = 0;
        fh.top_adrs = topadrs;
        fh.length   = endadrs - topadrs;
        fh.crc32    = calcrc(databuff, endadrs - topadrs);

        fwrite(&fh, 1, sizeof(fh), fp);
        fwrite(FileName, 1, fnc, fp);
        if (icc)
            copyicon(fp);
        fwrite(databuff, 1, fh.length, fp);
    }

    if (fp)
        fclose(fp);
}

void decode_pack(char *infile, char *outfile)
{
    (void)infile; (void)outfile;
    fprintf(stderr, "ppack: decode not implemented\n");
}

/* -------------------------------------------------------------------------
 * Command-line parsing
 * ------------------------------------------------------------------------- */

static void AdjPath(char *p)
{
    if (*p) {
        p += strlen(p);
        if (p[-1] != '/' && p[-1] != '\\') {
            *p++ = '/';
            *p = '\0';
        }
    }
}

void params(char *p)
{
    if (*p == '-') {
        switch (p[1]) {
        case 't':
        case 'd':
        case 'e':
            cmd_mode = p[1];
            break;
        case 'o':
            outfname = p + 2;
            break;
        case 'v':
            fverbose = isdigit((unsigned char)p[2]) ? atoi(p + 2) : 1;
            break;
        case 'k':
            fwait = 1;
            break;
        case 'r':
            store_name = p + 2;
            break;
        case 'm':
            fmethod |= (1 << atoi(p + 2));
            break;
        case 'b':
            sscanf(p + 2, "%lx", &bin_adrs);
            break;
        case 'p':
            strncpy(DataPath, p + 2, sizeof(DataPath) - 1);
            AdjPath(DataPath);
            break;
        case 'n':
            strncpy(FileName, p + 2, sizeof(FileName) - 1);
            FileName[sizeof(FileName) - 1] = '\0';
            break;
        case 'N':
#ifdef PPACK_HAVE_ICONV
            if (utf8_to_cp932(p + 2, FileName, sizeof(FileName)) != 0)
                exit(1);
#else
            fprintf(stderr,
                "ppack: -N requires iconv (not available in this build).\n"
                "       Use -n with raw CP932 bytes instead.\n");
            exit(1);
#endif
            break;
        case 'i':
            strncpy(IconName, p + 2, sizeof(IconName) - 1);
            break;
        }
    } else {
        infname = p;
    }
}

int main(int argc, char *argv[])
{
    int i;

    for (i = 1; i < argc; i++)
        params(argv[i]);

    switch (cmd_mode) {
    case 'd':
        if (outfname == NULL) outfname = "/dev/null";
        decode_pack(infname, outfname);
        break;
    case 't':
        decode_pack(infname, NULL);
        break;
    case 'e':
        if (outfname == NULL) outfname = "tmp.out";
        encode_pack(infname, outfname);
        break;
    default:
        fprintf(stderr, "%s", usage);
        return 1;
    }

    return 0;
}
