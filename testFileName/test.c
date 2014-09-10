#include <stdio.h>
#include <string.h>

/*
* parse the fullpath name(/chengshuguang/a.txt), get the filename(a.txt) and the parent dir(/chenshuguang/).
*/

int getFileName(char* filename,char *parent, const char* fullpath, int maxlen) {
  char strtmp[128], *head;
  //char strtmp2[128], *head2;
  const char *tail;
  int tmp;
  tmp = strlen(fullpath);
  tail = fullpath + tmp - 1;
  while (*tail == '/') /*去掉结尾所有的‘/’*/
    tail--;
  head = strtmp;
 
  while (tail>=fullpath && *tail!='/') /*得到filename的倒序*/
    *head++ = *tail--;
  strncpy(parent, fullpath, tail-fullpath+1);
  parent[tail-fullpath+1] = '\0';
  head--;
  
  while (maxlen>0 && head>=strtmp) {
    *filename++ = *head--;
    maxlen--;
  }
  if (maxlen>0) {
    *filename = '\0';
    return 0; //
  }
  else return -1; /*文件名太长*/
}
/*
* 从文件路径中解析出它的父目录
*/
void getParent(char *parent, const char *fullpath) {
	const char *tail;
	int tmp;
	
	tmp = strlen(fullpath);
	tail = fullpath + tmp - 1;
	while (*tail == '/') /*去掉结尾所有的‘/’*/
    	tail--;
    while (tail>=fullpath && *tail!='/') /*找到倒数第一个slash*/
        tail--;
    while (*tail == '/') /*去掉结尾所有的‘/’*/
    	tail--;
    strncpy(parent, fullpath, tail-fullpath+1);
    parent[tail-fullpath+1] = '\0';
}
/*
* the server stores the files in the format:[filename::version]
* for example, readMe of version 2 will be stored by the name readMe::2 
*/
/*char *getNewFileName(char *filename, int version) {*/
/*	char newname[256];*/
/*	sprintf(newname, "%s::%d", filename, version);*/
/*	return newname;*/
/*}*/

int main() {
	//char filename[128];
	//char parent[128];
	//getFileName(filename, parent, "chengshuguang/test.c", 128);
	//printf("%s\n%s\n",filename, parent);
	//getParent(parent, "/test.c/");
	//printf("%s\n", parent);
	char *path = "/chengshuguang/";
	char newname[256];
	sprintf(newname, "%s::%d", path, 23);
	printf("-%s-\n", newname);
	return 0;
}
