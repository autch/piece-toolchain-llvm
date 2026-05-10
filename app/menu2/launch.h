
#define FMAX 90				// ファイルの最大数
#define FNMAX (12+1)		// ファイル名の最大サイズ
#define CPMAX (24+1)		// キャプションの最大サイズ

typedef struct tagFILES {
	char name[FNMAX];		// ファイル名
	char caption[CPMAX];	// キャプション
	unsigned long adrs;		// ファイルのアドレス
	unsigned long length;	// ファイルの長さ
	unsigned char iconf;	// アイコンの有無
} FILES;

extern FILES files[FMAX];	// ファイル情報格納
extern unsigned char filec;	// ファイルの総数 (files[] 内の有効な個数)

void getdir( void );		// ファイル情報を得ます。files[] に格納されます。
int geticondata(char *filename, char *buff);	// アイコンデータを得ます。
void run( FILES *pf );		// FILES 構造体のファイルを起動します。
void go_standby( void );	// スタンバイに入ります。
