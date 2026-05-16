#include <piece.h>
#define PASS2
#include "dry.h"

#ifdef TURBO
#include "turboctl.h"
#endif

unsigned char vbuff[128 * 88];
int update = 0;

// pass1.c
extern Rec_Pointer Ptr_Glob, Next_Ptr_Glob;
extern int Int_Glob;
extern Boolean Bool_Glob;
extern char Ch_1_Glob, Ch_2_Glob;
extern int Arr_1_Glob[50];
extern int Arr_2_Glob[50][50];

Boolean Done;

unsigned long Begin_Time, End_Time, User_Time;
float Microseconds, Dhrystones_Per_Second;

void start();

void pceAppInit()
{
    memset(vbuff, 0, sizeof vbuff);
    pceLCDDispStop();
    pceLCDSetBuffer(vbuff);
    pceFontSetPos(0, 0);
    update = 1;

    pceAppSetProcPeriod(80);
    pceLCDDispStart();
    pceLCDTrans();
}

void pceAppProc(int cnt)
{
    if(!Done)
       start();

    if (update)
    {
        pceLCDTrans();
        update = 0;
    }
}

void pceAppExit()
{
#ifdef TURBO
    turbo(0);
#endif
}

extern Boolean Reg;
#undef REG
#define REG register

void cls()
{
    memset(vbuff, 0, sizeof vbuff);
    pceFontSetPos(0, 0);
    pceLCDTrans();
}

void start()
{
    One_Fifty Int_1_Loc;
    REG One_Fifty Int_2_Loc;
    One_Fifty Int_3_Loc;
    REG char Ch_Index;
    Enumeration Enum_Loc;
    Str_30 Str_1_Loc;
    Str_30 Str_2_Loc;
    REG int Run_Index;
    REG int Number_Of_Runs = 100;


    /* Initializations */

    Next_Ptr_Glob = (Rec_Pointer)pceHeapAlloc(sizeof(Rec_Type));
    Ptr_Glob = (Rec_Pointer)pceHeapAlloc(sizeof(Rec_Type));

    Ptr_Glob->Ptr_Comp = Next_Ptr_Glob;
    Ptr_Glob->Discr = Ident_1;
    Ptr_Glob->variant.var_1.Enum_Comp = Ident_3;
    Ptr_Glob->variant.var_1.Int_Comp = 40;
    strcpy(Ptr_Glob->variant.var_1.Str_Comp,
           "DHRYSTONE PROGRAM, SOME STRING");
    strcpy(Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

    Arr_2_Glob[8][7] = 10;
    /* Was missing in published program. Without this statement,    */
    /* Arr_2_Glob [8][7] would have an undefined value.             */
    /* Warning: With 16-Bit processors and Number_Of_Runs > 32000,  */
    /* overflow may occur for this array element.                   */
/*
    printf("\n");
    printf("Dhrystone Benchmark, Version %s\n", Version);
    if (Reg)
    {
        printf("Program compiled with 'register' attribute\n");
    }
    else
    {
        printf("Program compiled without 'register' attribute\n");
    }
    printf("Using %s, HZ=%d\n", CLOCK_TYPE, HZ);
    printf("\n");
*/
    Done = false;
    while (!Done)
    {
        pceFontPrintf("PLEASE WAIT...\n");
        pceFontPrintf("Trying %d runs...", Number_Of_Runs);
        pceLCDTrans();

        /***************/
        /* Start timer */
        /***************/

#ifdef TURBO
        // pceTimerGetPrecisionCount() が turbo 切り替えをまたぐと値が正しくなくなるので、さきに有効化する
        turbo(-1);
#endif

        Begin_Time = pceTimerGetPrecisionCount();

        for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index)
        {

            Proc_5();
            Proc_4();
            /* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
            Int_1_Loc = 2;
            Int_2_Loc = 3;
            strcpy(Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
            Enum_Loc = Ident_2;
            Bool_Glob = !Func_2(Str_1_Loc, Str_2_Loc);
            /* Bool_Glob == 1 */
            while (Int_1_Loc < Int_2_Loc) /* loop body executed once */
            {
                Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
                /* Int_3_Loc == 7 */
                Proc_7(Int_1_Loc, Int_2_Loc, &Int_3_Loc);
                /* Int_3_Loc == 7 */
                Int_1_Loc += 1;
            } /* while */
            /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
            Proc_8(Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
            /* Int_Glob == 5 */
            Proc_1(Ptr_Glob);
            for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index)
            /* loop body executed twice */
            {
                if (Enum_Loc == Func_1(Ch_Index, 'C'))
                /* then, not executed */
                {
                    Proc_6(Ident_1, &Enum_Loc);
                    strcpy(Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
                    Int_2_Loc = Run_Index;
                    Int_Glob = Run_Index;
                }
            }
            /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
            Int_2_Loc = Int_2_Loc * Int_1_Loc;
            Int_1_Loc = Int_2_Loc / Int_3_Loc;
            Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
            /* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
            Proc_2(&Int_1_Loc);
            /* Int_1_Loc == 5 */

        } /* loop "for Run_Index" */

        /**************/
        /* Stop timer */
        /**************/
    
        End_Time = pceTimerGetPrecisionCount();
        User_Time = pceTimerAdjustPrecisionCount(Begin_Time, End_Time);
#ifdef TURBO
        turbo(0);
#endif

        if (User_Time < 2 * 1000 * 1000) // 2 seconds, in microseconds
        {
            pceFontPrintf("too few\n");
            cls();

            Number_Of_Runs = Number_Of_Runs * 10;
        }
        else {
            Done = true;
            break;
        }
    }
    Microseconds = (float) User_Time / ((float) Number_Of_Runs);
    Dhrystones_Per_Second = (float) Number_Of_Runs * 1000000.0 / (float) User_Time;

    cls();

    pceFontPrintf("number of runs:\n");
    pceFontPrintf("%10d\n", Number_Of_Runs);
    pceFontPrintf("total user time:\n");
    pceFontPrintf("%10ld us\n", (long)User_Time);
    pceFontPrintf("\n");
    pceFontPrintf("microseconds in 1 loop:\n");
    pceFontPrintf("%10ld\n", (long)Microseconds);
    pceFontPrintf("Dhrystones per Second:\n");
    pceFontPrintf("%10ld\n", (long)Dhrystones_Per_Second);
}
