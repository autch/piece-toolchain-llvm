#define SIMPLE_USE_MACRO

#define SIMPLE_SPRITE_MAX (64)  /*シンプルライブラリのスプライト枚数*/

#define iabs(num) ( ((num)<0)?(-(num)):(num) )

typedef struct SPRITE_WORK {
  char  x,y,sx,sy;      /*表示座標,spmove開始座標*/
  char  dir,ctrl;       /*移動方向,移動状態*/
  unsigned char chr,ed; /*キャラクタ番号,移動ドット数*/
  int   si;             /*移動カウンタ*/
  short sp;             /*移動スピード*/
  char  atari[70];      /*あたり用文字列*/
} SPR_WORK;

extern SPR_WORK simple_sprite_reg[64]; /*スプライトレジスタ*/

extern char simple_msgbuff[512]; /*メッセージテンポラリ*/

extern char *simple_frame_ptr; /*フレームバッファポインタ*/

/*
   シンプルライブラリ内部エラー表示用
*/
void simple_error(char *format,...);

/*
   内部ドットセットルーチン
*/
#ifdef SIMPLE_USE_MACRO
#define simple_pset(pset_x,pset_y,pset_color) do{ \
  char *p=simple_frame_ptr; \
  int nextline=32,simple_pset_x=(pset_x),simple_pset_y=(pset_y); \
  int simple_pset_color=(pset_color); \
  p+=simple_pset_y*32*3+(simple_pset_x>>3); \
  simple_pset_x=1<<(simple_pset_x&7); \
  simple_pset_y=~simple_pset_x; \
  if(simple_pset_color<2){ \
    if(simple_pset_color<1){ \
      if(simple_pset_color<0){ \
	*p&=simple_pset_y;p+=nextline; *p&=simple_pset_y;p+=nextline; *p|=simple_pset_x; \
      }else{ \
	*p&=simple_pset_y;p+=nextline; *p&=simple_pset_y;p+=nextline; *p&=simple_pset_y; \
      } \
    }else{ \
      *p&=simple_pset_y;p+=nextline; *p|=simple_pset_x;p+=nextline; *p&=simple_pset_y; \
    } \
  }else if(simple_pset_color==2){ \
    *p|=simple_pset_x;p+=nextline;*p&=simple_pset_y;p+=nextline;*p&=simple_pset_y; \
  }else{ \
    *p|=simple_pset_x;p+=nextline;*p|=simple_pset_x;p+=nextline;*p&=simple_pset_y; \
  } \
}while(0)
#else
void simple_pset(int x,int y,int color);
#endif
