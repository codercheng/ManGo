#include <stdio.h>
#include <time.h>
#include <string.h>

typedef struct {
	int action;
	long time;
	char sha1[41];
	char path[218];
}record;

int writePending(int action, char *sha1, char *path) {
	FILE *fp = fopen("pending", "a+");
	if(fp == NULL) {
		fprintf(stderr, "open pending file error\n");
		return -1;
	}
	record r;
	r.action = action;
	r.time = time(NULL);
	strcpy(r.sha1, sha1);
	strcpy(r.path, path);
	fwrite(&r, sizeof(r), 1, fp);
	
	fclose(fp);
	return 0;
}

int readPending() {
	FILE *fp = fopen("pending", "r");	
	if(fp == NULL) {
		printf("open file pending error\n");
		return -1;
	}
	while(1) {
		record r;
		fread(&r, sizeof(r), 1, fp);
		if (feof(fp)) break;
		printf("-%d-, -%ld-, -%s-, -%s-\n", r.action, r.time, r.sha1, r.path);
	}
	fclose(fp);
	return 0;
}

int resetPending() {
	FILE *fp = fopen("pending", "w");
	if(fp == NULL) {
		printf("open file pending error\n");
		return -1;
	}
	fclose(fp);
	return 0;
}

int main() {
	printf("*****************write**************************\n");
	writePending(1, "xxooxxooxx111", "woshi_1");
	writePending(0, "xxooxxooxx222", "woshi_2");
	printf("*****************read***************************\n");
	readPending();
	printf("*****************reset**************************\n");
	resetPending();
	printf("************************************************\n");
	
	return 0;
}
