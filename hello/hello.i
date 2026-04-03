# 1 "hello.c"
# 1 "<built-in>" 1
# 1 "<built-in>" 3
# 359 "<built-in>" 3
# 1 "<command line>" 1
# 1 "<built-in>" 2
# 1 "hello.c" 2
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 1 3
# 53 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
typedef unsigned char BYTE;



typedef void (*PCETPENT)( void );

PCETPENT pceVectorSetTrap(int no, PCETPENT adrs);

typedef void *PCEKSENT;

PCEKSENT pceVectorSetKs(int no, PCEKSENT adrs);






unsigned long pcePadGetDirect( void );
void pcePadGetProc( void );
unsigned long pcePadGet( void );
void pcePadSetTrigMode( int mode );



void pceUSBDisconnect( void );
void pceUSBReconnect( void );
int pceUSBSetupMode( int mode, void *param2, void *param3 );






void pceLCDDispStart( void );
void pceLCDDispStop( void );
void pceLCDTrans( void );
void pceLCDTransDirect( const unsigned char *lcd_direct );
void pceLCDTransRange( int xs, int ys, int xe, int ye );
unsigned char *pceLCDSetBuffer( unsigned char *pbuff );
int pceLCDSetOrientation( int dir );
int pceLCDSetBright( int bright );



int pceFlashErase( void *romp );
int pceFlashWrite( void *romp, const void *memp, int len );



void pceTimerSetCallback( int ch, int type, int time, void (*callback)( void ) );
unsigned long pceTimerGetPrecisionCount( void );
unsigned long pceTimerAdjustPrecisionCount( unsigned long st, unsigned long ed );






unsigned long pceTimerGetCount( void );



void pceTimerSetContextSwitcher( unsigned long (*pContextSwitcher)( unsigned long nowsp, int flag ) );



int pceCPUSetSpeed( int no );
# 141 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
typedef struct tagPCEWAVEINFO {
 volatile unsigned char stat;
 unsigned char type;
 unsigned short resv;
 const void *pData;
 unsigned long len;
 struct tagPCEWAVEINFO *next;
 void (*pfEndProc)( struct tagPCEWAVEINFO *);
} PCEWAVEINFO;


int pceWaveCheckBuffs( int ch );
int pceWaveDataOut( int ch, PCEWAVEINFO *pwave );
int pceWaveAbort( int ch );
int pceWaveSetChAtt( int ch, int att );
int pceWaveSetMasterAtt( int att );
void pceWaveStop( int hard );



const unsigned char *pceFontGetAdrs( unsigned short code );
unsigned short pceFontPut( int x, int y, unsigned short code );
void pceFontSetType( int type );
void pceFontSetTxColor( int color );
void pceFontSetBkColor( int color );
void pceFontSetPos( int x, int y );
int pceFontPutStr( const char *pstr );
int pceFontPrintf( const char *fotmat, ... );





int pceAppSetProcPeriod( int period );
void pceAppReqExit( int exitcode );
int pceAppExecFile( const char *fname, int resv );

typedef struct tagMEMBLK {
 unsigned char *top;
 unsigned long len;
} MEMBLK;

int pceAppGetHeap( MEMBLK *pmb );

void pceAppActiveResponse( int flag );
# 194 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
typedef struct tagFILEINFO {
 char filename[26 +1];
 unsigned char attr;
 unsigned long length;
 unsigned long adrs;
 unsigned char works[16];
} FILEINFO;

int pceFileFindOpen( FILEINFO *pfi );
int pceFileFindNext( FILEINFO *pfi );
int pceFileFindClose( FILEINFO *pfi );

int pceFileLoad( const char *fname, void *ptr );


typedef struct FILEACC {
 unsigned short valid;
 unsigned char resv2;
 unsigned char resv3;
 const unsigned char *aptr;
 unsigned long fsize;
 unsigned short chain;
 unsigned short bpos;
} FILEACC;




int pceFileOpen( FILEACC *pfa, const char *fname, int mode );
int pceFileReadSct( FILEACC *pfa, void *ptr, int sct, int len );
int pceFileWriteSct( FILEACC *pfa, const void *ptr, int sct, int len );
int pceFileClose( FILEACC *pfa );
int pceFileCreate( const char *fname, unsigned long size );
int pceFileDelete( const char *fname );


int pceFileApfSave( int key, const void *ptr, int len );
int pceFileApfLoad( int key, void *ptr, int len );

int pceFileWriteSector( void *ptr, int len );



typedef struct tagPCETIME {
 unsigned short yy;
 unsigned char mm;
 unsigned char dd;
 unsigned char hh;
 unsigned char mi;
 unsigned char ss;
 unsigned char s100;
} PCETIME;

void pceTimeSet( const PCETIME *ptime );
void pceTimeGet( PCETIME *ptime );


typedef struct tagPCEALTIME {
 unsigned long mode;
 PCETIME time;
} PCEALMTIME;






int pceTimeSetAlarm( const PCEALMTIME *ptime );
int pceTimeGetAlarm( PCEALMTIME *ptime );





typedef struct tagPCEPWRSTAT {
 unsigned char status;
 unsigned char resv;
 unsigned short battvol;
} PCEPWRSTAT;




void pcePowerSetReport( int mode );
void pcePowerGetStatus( PCEPWRSTAT *ps );
void pcePowerForceBatt( int fn );
int pcePowerEnterStandby( int flag );



void pceIRStartRx( unsigned char *pData, int len );
void pceIRStartTx( const unsigned char *pData, int len );
void pceIRStartRxEx( unsigned char *pData, int len, int mode, int (*callback)(int rlen) );
void pceIRStartTxEx( const unsigned char *pData, int len, int mode, int (*callback)(void) );
void pceIRStartRxPulse( int mode, void (*rxproc)( int flag, unsigned short time ), int timeout );
void pceIRStartTxPulse( int mode, int (*txproc)( int flag ) );
void pceIRStop( void );
int pceIRGetStat( void );



typedef struct tagUSBCOMINFO {
 unsigned char signature[16];
} USBCOMINFO;

void pceUSBCOMSetup( USBCOMINFO *puci );
void pceUSBCOMStartRx( unsigned char *pData, int len );
void pceUSBCOMStartTx( const unsigned char *pData, int len );
int pceUSBCOMStop( void );
int pceUSBCOMGetStat( void );
# 314 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
int pceHeapGetMaxFreeSize( void );
int pceHeapFree( void *memp );
void *pceHeapAlloc( unsigned long size0 );

typedef struct tagHEAPMEM {
 unsigned short mark;
 unsigned short owner;
 struct tagHEAPMEM *chain;
} HEAPMEM;







typedef struct tagSYSTEMINFO {
 unsigned short size;
 unsigned short hard_ver;
 unsigned short bios_ver;
 unsigned short bios_date;
 unsigned long sys_clock;
 unsigned short vdde_voltage;
 unsigned short resv1;
 unsigned char *sram_top;
 unsigned char *sram_end;
 unsigned char *pffs_top;
 unsigned char *pffs_end;
} SYSTEMINFO;

const SYSTEMINFO *pceSystemGetInfo( void );

void pceDebugSetMon( int mode );


typedef char *va_list;


int pcesprintf( char *buff, const char *format, ... );
int pcevsprintf( char *buff, const char *format, va_list argp );
unsigned long pceCRC32( const void *ptr, unsigned long len );
# 388 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
typedef struct _pceAPPHEAD {
 unsigned long signature;
 unsigned short sysver;
 unsigned short resv1;
 void (*initialize)( void );
 void (*periodic_proc)( int cnt );
 void (*pre_terminate)( void );
 int (*notify_proc)( int type, int param );
 unsigned long stack_size;
 unsigned char *bss_end;
} pceAPPHEAD;
# 422 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
typedef struct _pffsFileHEADER {
 unsigned char mark;
 unsigned char type;
 unsigned short ofs_data;
 unsigned short ofs_name;
 unsigned short ofs_icon;
 unsigned long resv2;
 unsigned long top_adrs;
 unsigned long length;
 unsigned long crc32;
} pffsFileHEADER;
# 452 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
typedef struct tagpffsMARK {
 unsigned long ptr;
 unsigned long resv;
 char signature[24];
} pffsMARK;

typedef struct tagDIRECTORY {
 char name[24];
 unsigned char attr;
 unsigned char resv;
 unsigned short chain;
 unsigned long size;
} DIRECTORY;

typedef struct tagFAT {
 unsigned short chain;
} FAT;

typedef struct tagpffsMASTERBLOCK {
 pffsMARK mark;
 DIRECTORY dir[96];
 FAT fat[496];
} pffsMASTERBLOCK;
# 483 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 3
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/draw.h" 1 3
# 25 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/draw.h" 3
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Std.h" 1 3
# 25 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Std.h" 3
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 1 3
# 26 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Std.h" 2 3
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/stdlib.h" 1 3
# 24 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/stdlib.h" 3
typedef unsigned long size_t;





typedef unsigned short wchar_t;



typedef struct {
  int rem;
  int quot; } div_t;

typedef struct {
  long rem;
  long quot; } ldiv_t;
# 52 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/stdlib.h" 3
extern unsigned char *ansi_ucStartAlloc;
extern unsigned char *ansi_ucEndAlloc;
extern unsigned char *ansi_ucNxtAlcP;
extern unsigned char *ansi_ucTblPtr;
extern unsigned long ansi_ulRow;


extern unsigned int seed;

typedef int fn_t( const void *, const void * );




extern void abort( void );
extern void exit( int );
extern int atexit( void (*)(void) );
extern char * getenv( const char * );
extern int system( const char * );

extern void * malloc( size_t );
extern void * calloc( size_t, size_t );
extern void * realloc( void *, size_t );
extern void free( void * );
extern int ansi_InitMalloc(unsigned long, unsigned long);

extern int atoi( const char * );
extern long atol( const char * );
extern double atof( const char * );
extern long strtol( const char *, char **, int );
extern unsigned long strtoul( const char *, char **, int );
extern double strtod( const char *, char ** );

extern int abs( int );
extern long labs( long );

extern div_t div( int, int );
extern ldiv_t ldiv( long, long );

extern int rand( void );
extern void srand( unsigned int );

extern void * bsearch( const void *, const void *,
                        size_t, size_t, fn_t * );

extern void qsort( void *, size_t, size_t, fn_t * );
# 27 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Std.h" 2 3


typedef struct{
 unsigned char left;
 unsigned char top;
 unsigned char right;
 unsigned char bottom;
}RECTP;


typedef unsigned long DWORD;
typedef int BOOL;

typedef unsigned short WORD;
typedef float FLOAT;
typedef FLOAT *PFLOAT;

typedef int INT;
typedef unsigned int UINT;
typedef unsigned int *PUINT;



typedef char CHAR;
typedef short SHORT;
typedef long LONG;
# 26 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/draw.h" 2 3
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Bmp.h" 1 3
# 33 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Bmp.h" 3
typedef struct{
 short w;
 short h;
 BYTE *buf;
}PIECE_VRAM;




typedef struct {
 DWORD head;
 DWORD fsize;
 BYTE bpp;
 BYTE mask;
 short w;
 short h;
 DWORD buf_size;
}PBMP_FILEHEADER;
# 69 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/PIECE_Bmp.h" 3
typedef struct{
 PBMP_FILEHEADER header;
 BYTE *buf;
 BYTE *mask;
}PIECE_BMP;

extern BOOL PBM_CreateVram( PIECE_BMP *pbmp, PBMP_FILEHEADER *pbhead );
extern BOOL PBM_ReleaseVram( PIECE_BMP *pbmp );
extern BOOL PBM_Load_2B( PIECE_BMP *pbmp, char *fname );
extern BOOL PBM_Release_2B( PIECE_BMP *pbmp );
# 27 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/draw.h" 2 3
# 72 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/draw.h" 3
typedef struct{
 PIECE_VRAM *dest;
 short dx, dy;
 short dw, dh;

 PIECE_BMP *src;
 short sx, sy;

 RECTP clip;

 BYTE disp;
 BYTE param;





 BYTE type;

 BYTE layer;
}DRAW_OBJECT;



extern void pceLCDPoint(long color, long x, long y);
extern void pceLCDLine(long color, long x1, long y1, long x2, long y2);
extern void pceLCDPaint(long color, long x1, long y1, long x2, long y2);
extern void pceLCDSetObject(DRAW_OBJECT *obj, PIECE_BMP *src, int dx, int dy, int sx, int sy, int w, int h, int param );
extern int pceLCDDrawObject(DRAW_OBJECT dobj );
# 484 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/piece.h" 2 3
# 2 "hello.c" 2
# 1 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/string.h" 1 3
# 26 "/home/autch/src/llvm-c33/hello/..//sysroot/s1c33-none-elf/include/string.h" 3
char *memchr( ) ;
int memcmp( ) ;
char *memcpy( ) ;
char *memmove( ) ;
char *memset( ) ;
char *strcat( ) ;
char *strchr( ) ;
int strcmp( ) ;
char *strcpy( ) ;
size_t strcspn( ) ;
char *strerror( ) ;
size_t strlen( ) ;
char *strncat( ) ;
int strncmp( ) ;
char *strncpy( ) ;
char *strpbrk( ) ;
char *strrchr( ) ;
size_t strspn( ) ;
char *strstr( ) ;
char *strtok( ) ;
# 3 "hello.c" 2





unsigned char vbuff[128 * 88];
int update = 0;

void pceAppInit(void)
{
  memset(vbuff, 0, sizeof vbuff);

  pceLCDDispStop();
  pceLCDSetBuffer(vbuff);
  pceFontSetPos(0, 0);
  pceFontPutStr("Hello, world\nfrom " "LLVM" " toolchain");
  update = 1;

  pceAppSetProcPeriod(50);
  pceLCDDispStart();
}

void pceAppProc(int cnt)
{
  if(update) {
    pceLCDTrans();
    update = 0;
  }
  if(pcePadGet() & 0x4000)
    pceAppReqExit(0);
}

void pceAppExit(void)
{
}
