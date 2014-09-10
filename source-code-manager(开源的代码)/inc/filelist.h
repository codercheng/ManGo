#ifndef _FILE_LIST_H
#define _FILE_LIST_H
#include "strings.h"
#include "sha.h"

/*File operations*/
struct _file;
typedef struct _file *File;

struct _file
{
	String filename;
	uint32_t mode, mtime, size;
	ShaBuffer sha;
	bool deleted;
};


File File_Create(void);
void File_Delete(File f);
void File_Clone(File f, const File f1);
bool File_SetFileData(File f, const char *filename, const bool computeSha);

/*Converts the file data into machine independent form
 * and places the output in buffer.
 * Return the size of data written into buffer */
int File_Serialize(File f, unsigned char * const buffer, const int bufferSize);
bool File_DeSerialize(File f, unsigned const char *data, const int dataSize);

/**********End of file operations********************************/

/* Public API's for FileList. By Default the list is sorted during add.*/
struct _filelist;
typedef struct _filelist *FileList;

typedef enum
{
	FILE_NEW = 0,
	FILE_DELETED,
	FILE_MODIFIED,
	FILE_LAST_VALUE
}DifferenceType;

typedef int (*fn_difference)(File ref, File n, DifferenceType, void*);

/*Operations on list*/
FileList FileList_Create(void);
void FileList_Delete(FileList f);
void FileList_ResetList(FileList f);
inline uint32_t FileList_GetListLength(FileList f);
inline bool FileList_Find(FileList f, const char *filename, uint32_t * const pos);
File* FileList_GetListDetails(const FileList f, uint32_t * const listLength);
bool FileList_InsertFile(FileList f, const char* filename, const bool computeSha);
bool FileList_RemoveFile(FileList f, const char *filename, const bool recursive);
bool FileList_MergeList(FileList masterList, const FileList newList);
bool FileList_GetDifference(const FileList reference, const FileList list, fn_difference function, void *data);

/*Function to convert list into format which is transportable accross
 * network or machines*/
bool FileList_Serialize(FileList f, const char *filename);
bool FileList_DeSerialize(FileList f, const char *filename);

/*Get the contents of the directory into the list.*/
bool FileList_GetDirectoryConents(FileList f, const char *path, const bool recursive, const bool computeSha);

/*Print the list*/
void FileList_PrintList(const FileList f, const bool recursive, const bool longlist);
#endif
