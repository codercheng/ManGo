#include <stdio.h>
#include <time.h>
#include <string.h>

int main() {
	FILE *fp = fopen("pending", "w");	
	fclose(fp);
	return 0;
}
