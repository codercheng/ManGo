#include <stdio.h>
#include <time.h>
#include <string.h>


typedef struct {
	int action;
	long time;
	char sha1[41];
	char path[218];
}record;

int main() {
	FILE *fp = fopen("pending", "r");	
	if(fp == NULL) {
		printf("open file pending error\n");
		return -1;
	}
	while(1) {
		record r;
		fread(&r, sizeof(r), 1, fp);
		if (feof(fp)) break;
		printf("%d, %ld, %s, %s\n", r.action, r.time, r.sha1, r.path);
	}
	fclose(fp);
	return 0;
}
