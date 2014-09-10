/*
* this file define a linked list to save the path::version string array;
* it may be very easy to implement this class in C++, but i want a C style one
*/

#include <stdio.h>
#include <stdlib.h>
//#include <time.h>
//#include <string.h>

typedef struct FilePathListNode_ {
	char filename[256];
	char sha1[41];
	char m_time[32];
	int isDir;
	struct FilePathListNode_ *next;
}FilePathListNode;

typedef struct {
	FilePathListNode *head;
	FilePathListNode *tail;
	int size;
}FilePathList;

void init(FilePathList *list) {
	list->head = list->tail = NULL;
	list->size = 0;
}


void push_back(FilePathList *list, FilePathListNode *node) {
	if(list->head == NULL) {
		list->head = node;
		list->tail = node;
	} else {
		list->tail = list->tail->next = node;
	}
	list->size++;
}

void unit_test(FilePathList *list) {
	if(list->size == 0)
		return;
	printf("list size: %d\n", list->size);
	FilePathListNode *tmp = list->head;
	while(tmp != list->tail) {
		if(!tmp->isDir)
			printf("<%s--%s--%s> dir:%d\n", tmp->filename, tmp->sha1, tmp->m_time, tmp->isDir);
		else
			printf("<%s> dir:%d\n", tmp->filename, tmp->isDir);
		
		//printf("%s\n", tmp->filename);
		tmp = tmp->next;
	}
	if(!tmp->isDir)
		printf("<%s--%s--%s> dir:%d\n", tmp->filename, tmp->sha1, tmp->m_time, tmp->isDir);
	else
		printf("<%s> dir:%d\n", tmp->filename, tmp->isDir);
	//printf("%s\n", tmp->filename);
}


//strLinkedList is used to 
//record the version number
typedef struct strNode{
	char str[28];
	struct strNode *next;
}strNode;

typedef struct {
	strNode *head;
	strNode *tail;
}strLinkedList;

void initStr(strLinkedList *list) {
	list->head = list->tail = NULL;
}

void push_back_str(strLinkedList *list, strNode *node) {
	if(list->head == NULL) {
		list->head = node;
		list->tail = node;
	} else {
		list->tail = list->tail->next = node;
	}
}
