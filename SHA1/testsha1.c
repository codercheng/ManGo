#include "sha1.h"
#include <stdio.h>

int main(){
	ShaBuffer sha;
	char file[256];
	scanf("%s",file);
	sha_file(file, sha);
	printf("sha:%s\n",sha);
	return 0;
}
