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

#define TH_NULL (0) /*空き*/
#define TH_EXEC (1) /*実行中*/
#define TH_WAIT (2) /*待機中*/

typedef void (*THREAD_FUNC)(void);
/*********************************************************
 全体管理関数 呼び出しはメインルーチンからのみ呼び出すこと
*********************************************************/
/*スレッド機構初期化*/
void thread_gr_init();
/*スレッド機構終了*/
void thread_gr_exit();
/*スレッド機構一時停止*/
void thread_gr_stop();
/*スレッド機構再開*/
void thread_gr_cont();

/*********************************************************
 スレッド関連関数
*********************************************************/
/*
   新規スレッドの作成
   in: スレッドの関数
   out:負の数:作成失敗　正の数:スレッドのID
*/
int thread_create(THREAD_FUNC ptr);

/*
   （自分自身の）スレッドの終了
   親の場合だけはなにもせずreturn
*/
void thread_return();

/*
   スレッドの強制終了（親なら終了しません）
   in: スレッドのID
   out:正の数:正常終了
*/
int thread_terminate(int id);

/*
   指定スレッドが終了するのを待つ
   in: スレッドのID
   out:正の数:正常終了 負の数:異常終了（自分のID指定など）
*/
int thread_wait_term(int id);

/*
   現在実行中（自分自身）のＩＤを取得
   out:自分自身のID
*/
int thread_currentid();

/*
   指定ＩＤスレッドの現在の状態を取得
   in: スレッドID
   out:状態
*/
int thread_status(int id);

/*
   時間待ち
   in: 待ち時間（時間は1ms単位）
   　他のスレッドの処理によりreturnが遅れることがあります
*/
void thread_wait(int time);

/*
   スレッドの切り替わりを禁止
*/
void thread_lock();

/*
   スレッドの切り替わりを許可
*/
void thread_unlock();
