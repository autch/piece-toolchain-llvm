//S1C33209/221/222テクニカルマニュアル『図4.1 メモリマップ』(s1c33209_221_222j.pdf B-II-4-4)参照
//内蔵I/Oメモリの正式なアドレスは0x40000～0x4FFFFだが、0x30000～0x3FFFF,及び,0x50000～0x5FFFFにもミラーが見える。
//volatile struct c_IOtag* const plc_IO = (volatile struct c_IOtag*)0x30000;	//①ミラーアドレス	┐
volatile struct c_IOtag* const plc_IO = (volatile struct c_IOtag*)0x40000;	//②正式なアドレス	├どれでも可。即値をロードする場合は①を使用するとコードサイズを削減出来る。例はclippce.cの_precision_pad_isrを参照せよ。
//volatile struct c_IOtag* const plc_IO = (volatile struct c_IOtag*)0x50000;	//③ミラーアドレス	┘
