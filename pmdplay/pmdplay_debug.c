#include <piece.h>
#include "pcestdint.h"

extern int g_fDraw;    // pmdplay.c

extern int g_fDebug;   // pmdplay.c
extern int g_dbgPage;   // pmdplay.c
void Cls();    // pmdplay.c

static void ShowDebugPage0(void)
{
	pceFontSetPos(0, 0);
	pceFontPrintf("(debug removed)");
}

static void ShowDebugPage1(void) {}
static void ShowDebugPage2(void) {}
static void ShowDebugPage3(void) {}

void ShowDebug()
{
	Cls();
	if (g_dbgPage == 0)
		ShowDebugPage0();
	else if (g_dbgPage == 1)
		ShowDebugPage1();
	else if (g_dbgPage == 2)
		ShowDebugPage2();
	else
		ShowDebugPage3();
	g_fDraw = 1;
}
