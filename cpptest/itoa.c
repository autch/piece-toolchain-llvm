/**
 * @brief 数値を文字列に変換する。
 * 
 * @param val 変換したい値
 * @param buf 変換結果を受け取るバッファ
 * @param radix 基数。2, 8, 10, 16 が指定できる。
 * @return int 変換後の文字列の長さ。'\0' は含まない。
 */
int itoa(int val, char* buf, int radix)
{
	char* pnt = buf;
	char* pnt2 = buf;
	int tmp;

	do {
		tmp = val % radix;
		if(tmp < 10) {
			*pnt++ = tmp + '0';
		} else {
			*pnt++ = tmp - 10 + 'A';
		}
		val /= radix;
	} while(val != 0);

	*pnt-- = '\0';

	while(pnt2 < pnt) {
		tmp = *pnt;
		*pnt-- = *pnt2;
		*pnt2++ = tmp;
	}

	return pnt2 - buf + 1;
}

/**
 * @brief 数値を文字列に変換する。幅指定とパディングに対応。
 * 
 * @param val 変換したい値
 * @param buf 変換結果を受け取るバッファ
 * @param radix 基数。2, 8, 10, 16 が指定
 * @param width 最小幅。変換後の文字列がこの幅に満たない場合、padで指定した文字で埋める。
 * @param pad 埋める文字。通常は' 'や'0'が指定される。
 * @return int 変換後の文字列の長さ。'\0' は含まない。
 */
int itoa_pad(int val, char* buf, int radix, int width, char pad)
{
	char* pnt = buf;
	char* pnt2 = buf;
	int tmp;

	do {
		tmp = val % radix;
		if(tmp < 10) {
			*pnt++ = tmp + '0';
		} else {
			*pnt++ = tmp - 10 + 'A';
		}
		val /= radix;
	} while(val != 0);

	while(pnt - buf < width) {
		*pnt++ = pad;
	}

	*pnt-- = '\0';

	while(pnt2 < pnt) {
		tmp = *pnt;
		*pnt-- = *pnt2;
		*pnt2++ = tmp;
	}

	return pnt2 - buf + 1;
}
