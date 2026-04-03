#include <piece.h>
#include <string.h>

#define DRAW_NORMAL     (DRW_NOMAL)

extern const unsigned char JIEN_BMP[];
static unsigned char vScreen[128 * 88];
int g_bDirty;

#define MarkDirty() (g_bDirty = 1)
#define ClearDirty() (g_bDirty = 0)

DRAW_OBJECT obj_jien;
PIECE_BMP jien;
void LoadBitmapFromBuffer(PIECE_BMP* pBmp, const char* pSource);
void Refresh();
void Cls();

int x, y, dx, dy;

void Move()
{
	if(x + dx < 0 || x + dx + jien.header.w > 128)
		dx *= -1;
	if(y + dy < 0 || y + dy + jien.header.h > 88)
		dy *= -1;

	x += dx;
	y += dy;
}

void Draw()
{
    pceLCDSetObject(&obj_jien, &jien, x, y,
                   0, 0, jien.header.w, jien.header.h, DRAW_NORMAL);
    pceLCDDrawObject(obj_jien);
    MarkDirty();

    Move();
}

char str[20];

char* _strcpy(char* dest, const char* src)
{
    char* p = dest;
    while(*src) {
        *p++ = *src++;
    }
    *p = 0;
    return dest;
}

void pceAppInit()
{
    MarkDirty();
    pceLCDDispStop();
    pceLCDSetBuffer(vScreen);
    pceAppSetProcPeriod(50);
    Cls();

    LoadBitmapFromBuffer(&jien, JIEN_BMP);
    pceFontSetPos(0,0);
    pceFontPrintf("w:%d,h:%d", jien.header.w, jien.header.h);

    x = y = 0;
    dx = dy = 1;
    Draw();

    Refresh();
    pceLCDDispStart();
}

void pceAppProc(int cnt)
{
   Draw();
   Refresh();
}

void pceAppExit()
{
}

void Refresh()
{
    if(!g_bDirty) return;
    pceLCDTrans();
    ClearDirty();
}

void Cls()
{
    memset(vScreen, 0, 128 * 88);
    MarkDirty();
}

void LoadBitmapFromBuffer(PIECE_BMP* pBmp, const char* pSource)
{
    char* p = pSource;
    PBMP_FILEHEADER* pHeader;

    pHeader = &pBmp->header;
    memcpy(pHeader, p, sizeof(PBMP_FILEHEADER));
    p += sizeof(PBMP_FILEHEADER);
    pBmp->buf = p;
    p += (pHeader->w * pHeader->h) >> 2;
    pBmp->mask = p;
}
