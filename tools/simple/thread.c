/////////////////////////////////////////////////////////////////////////////
//
//             /
//      -  P  /  E  C  E  -
//           /                 mobile equipment
//
//              Library Programs
//
//
// PIECE 簡易スレッドライブラリ : Ver 0.80
//
// Copyright (C)2001 AUQAPLUS Co., Ltd. / OeRSTED, Inc. all rights reserved.
//
// Coded by Katsumasa Tsuneyoshi
//
// Comments:
//
//  v0.50 2001.11.17 bug fix
//  v0.60 2001.12.27 割り込みルーチン内でwaitしない
//  v0.70 2002.01.14 thread_lockをレベルで行わない
//  v0.80 2002.01.14 pceTimerSetContextSwitcherを使用するように変更
//

#include <s1c33cpu.h>
#include <piece.h>
#include "thread.h"

#define MAX_TH     (3)                                 /*スレッドの数*/
#define WAIT_ID    (MAX_TH+1)                          /*wait用スレッドのID*/
#define STACK_SIZE (8192/4)                            /*スレッドのスタック*/

#define NO (0)
#define YES (!NO)

static int stack[MAX_TH][STACK_SIZE]; /*スタック*/
static int wait_stack[256/4];         /*ウェイト用スタック*/

/* 0 ～ (MAX_TH-1)/子 MAX_TH/親 (MAX_TH+1)/wait専用*/
static int *sp[MAX_TH+2];             /*スタックポインタ*/
static int th_ct[MAX_TH+2];           /*wait用減算カウンタ 0なら実行*/
static char state[MAX_TH+2];          /*ステータス*/

static int now_id;                    /*現在実行中のスレッドＩＤ*/
static int old_ct;                    /*前回の1msカウンタの値*/

static char lock_flag;                /*YES:切り替え停止中*/
static char timer_flag;               /*YES:切り替え停止中に割り込み発生*/

static unsigned long change_th_check_isr(unsigned long now_sp,int flag);
static unsigned long change_th_check(unsigned long now_sp);
static void in_thread_create(THREAD_FUNC first_func,THREAD_FUNC return_func,int **sp);
static void thread_exec_as(unsigned long sp);
static void thread_unlock_imask();
static void pdwait(int n);
static void wait_loop();

/*
   スレッド機構初期化
*/
void thread_gr_init()
{
  int i;
  for(i=0;i<MAX_TH;i++)
    state[i]=TH_NULL;
  state[MAX_TH]=TH_EXEC;
  now_id=MAX_TH;
  sp[WAIT_ID]=&wait_stack[sizeof(wait_stack)/4];
  in_thread_create(wait_loop,thread_return,&sp[WAIT_ID]);
  lock_flag=NO;
  timer_flag=NO;

  old_ct=pceTimerGetCount();
  pceTimerSetContextSwitcher(change_th_check_isr);
}

/*
   スレッド機構終了
*/
void thread_gr_exit()
{
  pceTimerSetContextSwitcher(NULL);
}


/*
   スレッド機構一時停止
*/
void thread_gr_stop()
{
  pceTimerSetContextSwitcher(NULL);
}

/*
   スレッド機構再開
*/
void thread_gr_cont()
{
  old_ct=pceTimerGetCount();
  pceTimerSetContextSwitcher(change_th_check_isr);
}

/*
   スタックを切り替えてジャンプ
*/

void thread_exec_as(unsigned long sp)
{
  asm(
	"\tld.w	%sp,%r12\n"
	"\tpopn	%r1\n"
	"\tld.w	%ahr,%r1\n"
	"\tld.w	%alr,%r0\n"
	"\tpopn	%r15\n"
	"\treti\n");
}

/*
   スレッドの終了（関数から抜けた）
   親の場合だけはなにもせずreturn
*/
void thread_return()
{
  unsigned long new_sp;
  thread_lock();
  if(now_id==MAX_TH){ /*親なのでリターン*/
    thread_unlock();
    return;
  }
  state[now_id]=TH_NULL;
  new_sp=change_th_check(0);
  thread_unlock_imask();
  thread_exec_as(new_sp);
}

/*
   新規スレッドの作成
   in: スレッドの関数
   out:負の数:作成失敗　正の数:スレッドのID
*/
int thread_create(THREAD_FUNC ptr)
{
  int i;
  thread_lock();
  for(i=0;i<MAX_TH;i++){
    if(state[i]==TH_NULL)
      break;
  }
  if(i==MAX_TH){
    i=-1; /*空きなし*/
  }else{
    sp[i]=&stack[i][STACK_SIZE];
    in_thread_create(ptr,thread_return,&sp[i]);
    th_ct[i]=0;
    state[i]=TH_EXEC;
  }
  thread_unlock();
  return i;
}

static void in_thread_create(THREAD_FUNC first_func,THREAD_FUNC return_func,int **sp)
{
  int i;
  *--*sp=(int)return_func;
  *--*sp=(int)first_func;
  *--*sp=0x10; /*psr*/
  for(i=0;i<18;i++)
    *--*sp=0;
}

/*
   スレッドの強制終了（親なら終了しません）
   in: スレッドのID
   out:正の数:正常終了
*/
int thread_terminate(int id)
{
  if(id<MAX_TH)
    if(id==now_id)
      thread_return();   /*そのまま自分自身が消滅*/
    else
      state[id]=TH_NULL; /*なにがあっても状態をつぶす*/
  return 0;
}

/*
   現在実行中（自分自身）のＩＤを取得
   out:自分自身のID
*/
int thread_currentid()
{
  return now_id;
}

/*
   指定ＩＤスレッドの現在の状態を取得
   in: スレッドID
   out:状態
*/
int thread_status(int id)
{
  return state[id];
}

/*
   時間待ち
   in: 待ち時間（時間は1ms単位）
   　他のスレッドの処理によりreturnが遅れることがあります
*/
void in_thread_wait(int time,unsigned long pre_sp)
{
  int ni;
  unsigned long new_sp;
  thread_lock();
  ni=now_id;
  th_ct[ni]=time;
  state[ni]=TH_WAIT;
  new_sp=change_th_check(pre_sp);
  thread_unlock_imask();
  thread_exec_as(new_sp);
}

void dummy_asm_func()
{
  asm(
	"\t.global	thread_wait\n"
	"thread_wait:\n"
	"\tld.w	%r4,%r0\n"
	"\tld.w	%r0,0x10\n"
	"\tpushn	%r0\n"
	"\tld.w	%r0,%r4\n"
	"\tpushn	%r15\n"
	"\tld.w	%r0,%alr\n"
	"\tld.w	%r1,%ahr\n"
	"\tpushn	%r1\n"
	"\tld.w	%r13,%sp\n"
	"\tjp	in_thread_wait\n");
}

/*
   指定スレッドが終了するのを待つ
   in: スレッドのID
   out:正の数:正常終了 負の数:異常終了（自分のID指定など）
*/
int thread_wait_term(int id)
{
  if(id==now_id)
    return -1;
  while(-1){
    if(state[id]==TH_NULL)
      break;
    thread_wait(1); /*1ms待つ*/
  }
  return 0;
}

/*
   スレッドの切り替わりを禁止
*/
void thread_lock()
{
  lock_flag=YES;
}

/*
   スレッドの切り替わりを許可
*/
void thread_unlock()
{
  if(timer_flag)
    thread_wait(0);
  lock_flag=NO;
}

/*
   スレッドの切り替わりを許可しつつ、割り込み禁止
*/
void thread_unlock_imask()
{
  SET_IL7;
  lock_flag=NO;
}

/*
   新しい実行用スタックポインタを求める
*/
unsigned long change_th_check_isr(unsigned long now_sp,int flag)
{
  if(lock_flag||flag){
    timer_flag=YES;
    return now_sp;
  }
  return change_th_check(now_sp);
}

unsigned long change_th_check(unsigned long now_sp)
{
  int i,ni=now_id,dct;
  sp[ni]=(int *)now_sp;
  /*waitしているタイマの計算*/
  i=pceTimerGetCount();
  dct=i-old_ct;
  old_ct=i;
  if(dct>100) dct=1;  /*ループしたときのために*/
  for(i=0;i<(MAX_TH+1);i++){
    if(state[i]==TH_WAIT)
      if((th_ct[i]-=dct)<=0)
	state[i]=TH_EXEC;
  }
  /*次に実行するものを決める*/
  for(i=0;i<(MAX_TH+1);i++){
    if((++ni)>MAX_TH) ni=0;
    if(state[ni]==TH_EXEC)
      break;
  }
  if(i==(MAX_TH+1)){/*みんなwait中*/
    ni=WAIT_ID;
  }
  now_id=ni;
  timer_flag=NO;
  return((unsigned long)(sp[ni]));
}

/*
パワーダウンのためのウェイト
*/
void pdwait( int n )
{
  asm( "\tld.w %r10,%r12" );
  //asm( "xld.w %r11,0x1000000" );
  asm(
	"\text     32\n"
	"\text      0\n"
	"\tld.w    %r11, 0\n"
	);
  asm( "\tld.w %r12,%r11" );
  asm( "\tmac %r10" );
}

void wait_loop()
{
  while(1){
    pdwait(200);
    thread_wait(0);
  }
}
