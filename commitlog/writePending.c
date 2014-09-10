#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>


typedef struct {
	int action;
	long time;
	char sha1[41];
	char path[218];
}record;

int main() {
	FILE *fp = fopen("pending", "a+");	
	int i;
	for (i=0; i<2; i++) {
		record r;
		r.action = i;
		r.time = time(NULL);
		strcpy(r.sha1, "xxooxxooxx");
		strcpy(r.path, "/chengshuguang/mango/web/test");
		fwrite(&r, sizeof(r), 1, fp);
	}
	fclose(fp);
	return 0;
}
