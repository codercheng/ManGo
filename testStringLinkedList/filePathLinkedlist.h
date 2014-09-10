/*
* this file define a linked list to save the path::version string array;
* it may be very easy to implement this class in C++, but i want a C style one
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#include <string.h>

typedef struct FilePathListNode_ {
	char filename[256];
	char sha1[41];
	time_t m_time;
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


/*
* aim: if isFile, push path::version to the file list
*	   else if idDir, push path to the file list
*/
void push_to_list(FilePathList *filelist, char *path, char *version,\
	 char *sha1, time_t m_time, int isDir) {
	FilePathListNode * newpath = (FilePathListNode *)malloc(sizeof(FilePathListNode));
	if(!isDir) {
		sprintf(newpath->filename, "%s::%s", path, version);
		sprintf(newpath->sha1, "%s", sha1);
		newpath->m_time = m_time;
		newpath->next = NULL;
		push_back(filelist, newpath);
	} else {
		sprintf(newpath->filename, "%s", path);
		newpath->next = NULL;
		push_back(filelist, newpath);
	}
	
}

void unit_test(FilePathList *list) {
	if(list->size == 0)
		return;
	printf("list size: %d\n", list->size);
	FilePathListNode *tmp = list->head;
	while(tmp != list->tail) {
		printf("%s--%s--%ld\n", tmp->filename, tmp->sha1, tmp->m_time);
		tmp = tmp->next;
	}
	printf("%s--%s--%ld\n", tmp->filename, tmp->sha1, tmp->m_time);
}
