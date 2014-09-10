#include "filePathLinkedlist.h"

int main()
{
	FilePathList *list = (FilePathList *)malloc(sizeof(FilePathList));
	init(list);
	
	int i;
	for(i=0; i<11; i++) {
		//FilePathListNode * newpath = (FilePathListNode *)malloc(sizeof(FilePathListNode));
		//sprintf(newpath->filename, "%s::%d", "chengshuguang/test.c", i);
		//newpath->next = NULL;
		push_to_list(list, "cheng/test.c", "21", "shshshshshshsh", 123455677, 0);
	}
	unit_test(list);

	return 0;
}
