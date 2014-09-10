#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


void getParent(char *parent, const char *fullpath);

int main()
{
	char path [256];
	char realpath[256];
	scanf("%s",path);

	char pwd[256];
	getcwd(pwd, 256);
	printf("-----------pwd:%s\n", pwd);
	char tmppwd[256];
	strcpy(tmppwd, pwd);
	/*get the real path*/
	while(1) {
		char tmp[256];
		if(access("b1", F_OK) != 0) { /*file not exist*/
			if(strcmp(tmppwd, "/") == 0)
			{
				printf("*** Set a WC first! ****\n");
				chdir(pwd);
				return -1;
			}
			getParent(tmp, tmppwd);
			if(strcmp(tmp, "") == 0)
				strcpy(tmp, "/");
			printf("%s\n",tmp);
			chdir(tmp);
			memset(tmppwd, 0, sizeof(tmppwd));
			strcpy(tmppwd, tmp);
		} else {
			break;
		}
	}
	//chdir(pwd);

	printf("++%s\n", tmppwd);
	if(strlen(tmppwd) != strlen(pwd)) { /*path is not real path*/
		char *p;
		if(strcmp(tmppwd, "/") == 0) {
			p = pwd + 1;
		} else {
			p = pwd + strlen(tmppwd)+1; /*discard the front '/'*/
		}
		printf("plus:%s\n", p);
		sprintf(realpath, "%s/%s", p, path);
		printf("realpath:%s\n", realpath);
	} else {
		strcpy(realpath, path);
	}
	

	printf("realpath:%s\n", realpath);
	
	getcwd(pwd, 256);
	printf("-----------pwd:%s\n", pwd);
	return 0;
}


/*
* 从文件路径中解析出它的父目录(chengshuguang/a.txt--->chengshuguang)
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