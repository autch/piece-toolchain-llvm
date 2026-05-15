/*
   ２つの文字列に同じ文字が存在するかチェック
   発見すれば、その文字を返す
*/
int inchr(char *str1,char *str2)
{
  int i,j;
  for(i=0;i<256;i++){
    if(str1[i]==0)
      break;
    for(j=0;j<256;j++){
      if(str2[j]==0)
	break;
      if(str1[i]==str2[j])
	return str1[i];
    }
  }
  return 0;
}
