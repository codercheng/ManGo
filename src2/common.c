#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include "common.h"

int getTime(char buffer[64]){
	time_t t;
	struct tm *tim;
	t = time(NULL);
	tim = localtime(&t);
	strftime(buffer, 64, "%a %b %d  %H:%M:%S  %Y", tim);
	return 0;
}

bool isItFolder(const char *name){
	struct stat s;
	if(0 == stat(name, &s)){
		if(S_ISDIR(s.st_mode))
			return true;
	}
	return false;
}

bool isItFile(const char *name){
	struct stat s;
	if(0 == stat(name, &s)){
		if(S_ISREG(s.st_mode))
			return true;
	}
	return false;
}
