#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(){
	int ret = chroot("/home");
	if(ret == -1){
		printf("%s\n", strerror(errno));	
	}
	else{
		printf("success\n");
	}
	ret = chdir("/");
	if(ret == -1){
		printf("2--error\n");	
	}else{
		printf("success\n");
	}
	execl("/bin/ls","ls",NULL);
	return 0;
}
