/*
* 把出错处理，还有一些共用函数都整合到这个类中来
*/
#ifndef COMMON_H_
#define COMMON_H_

#include <stdio.h>
#include <string.h>

#ifndef __cplusplus

typedef enum {
	false = 0,
	true  = 1
}bool;
#endif

typedef enum {
	ADD_FILES,
	GET_FILES,
	SEND_FILES,
	TEST_ADD,
	TEST_GET,
	INIT
}command;

typedef struct message_{
	command cmd;
}message;

typedef struct {
	char path[256];
	char sha1[41];
	long m_time;
	char comment[512];
}addinfo;



bool isItFile(const char *name);
bool isItFolder(const char *name);
int getTime(char buffer[64]);



#endif