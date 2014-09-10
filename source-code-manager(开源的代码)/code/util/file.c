#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "filelist.h"

#define INT_SIZE	sizeof(uint32_t)


File File_Create(void)
{
	File f = (File)XMALLOC(sizeof(struct _file));
	f->filename = String_Create();

	f->deleted = false;
	f->mode = f->mtime = f->size = 0;
	sha_reset(f->sha);
	return f;
}

void File_Clone(File f, const File f1)
{
	String_clone(f->filename, f1->filename);
	f->mode = f1->mode;
	f->mtime = f1->mtime;
	f->size = f1->size;
	memcpy(f->sha, f1->sha, SHA_HASH_LENGTH);
}
void File_Delete(File f)
{
	String_Delete(f->filename);
	XFREE(f);
	return;
}

bool File_SetFileData(File f, const char *filename, const bool computeSha)
{
	bool returnValue = false;
	struct stat s;
	if(0 == stat(filename, &s))
	{
		returnValue = true;

		String_strcpy(f->filename, filename);

		/*Save the file properties*/
		f->mode  = s.st_mode;
		f->mtime = s.st_mtime;
		f->size  = s.st_size;

		/*compute the sha if necessary*/
		if(S_ISREG(s.st_mode) && (true == computeSha))
		{
			returnValue = sha_file(filename, f->sha);
		}
		else
		{
			f->sha[0] = '\0';
		}
	}
	else
	{
		LOG_ERROR("File_SetFileData: File '%s' doesn't exist", filename);
	}
	return returnValue;
}

int File_Serialize(File f, unsigned char * const buffer, const int bufferSize)
{
	int pos = 0;
	int Slen = strlen((char*)f->sha)+1;
	int mode, size, mtime, lenSha, lenFilename, totalLen;

	/*Total Length = sizeof(f->mode) + sizeof(f->size) + sizeof(f->mtime) + lenOfFilename + lenOfSha + SHA_HASH_LENGTH + strlen(Filename)*/
	totalLen = INT_SIZE * 5 + String_strlen(f->filename) + Slen;
	if(totalLen > bufferSize)
	{
		return 0;
	}
	mode = htonl(f->mode);
	size = htonl(f->size);
	mtime = htonl(f->mtime);
	lenSha = htonl(Slen);
	lenFilename = htonl(String_strlen(f->filename));

	memcpy(buffer + pos, &mode, INT_SIZE); 			pos += INT_SIZE;
	memcpy(buffer + pos, &size, INT_SIZE); 			pos += INT_SIZE;
	memcpy(buffer + pos, &mtime, INT_SIZE); 		pos += INT_SIZE;
	memcpy(buffer + pos, &lenSha, INT_SIZE); 		pos += INT_SIZE;
	memcpy(buffer + pos, &lenFilename, INT_SIZE); 	pos += INT_SIZE;
	memcpy(buffer + pos, f->sha, Slen); 			pos += Slen;

	memcpy(buffer + pos, s_getstr(f->filename), String_strlen(f->filename));
	pos += String_strlen(f->filename);

	return totalLen;
}
bool File_DeSerialize(File f, unsigned const char *data, const int dataSize)
{
	int pos = 0;
	char filename[1024];
	int lenSha, lenFilename;

	if(dataSize < INT_SIZE*5)
	{
		LOG_ERROR("invalid dataSize %d, min size %d", dataSize, INT_SIZE *5);
		return	false;
	}

	/*Get the size of each element in the record*/
	memcpy(&(f->mode),   data + pos, INT_SIZE);   pos += INT_SIZE;
	memcpy(&(f->size),   data + pos, INT_SIZE);   pos += INT_SIZE;
	memcpy(&(f->mtime),  data + pos, INT_SIZE);   pos += INT_SIZE;
	memcpy(&lenSha,      data + pos, INT_SIZE);   pos += INT_SIZE;
	memcpy(&lenFilename, data + pos, INT_SIZE);   pos += INT_SIZE;

	lenSha = ntohl(lenSha);
	f->mode = ntohl(f->mode);
	f->size = ntohl(f->size);
	f->mtime = ntohl(f->mtime);
	lenFilename = ntohl(lenFilename);
	if(lenSha > (SHA_HASH_LENGTH+1))
	{
		LOG_ERROR("SHA length %d is not correct value, it should have been %d", lenSha, SHA_HASH_LENGTH+1);
		return false;
	}
	if(dataSize != (INT_SIZE * 5 + lenSha + lenFilename))
	{
		LOG_ERROR("invalid dataSize %d, dataSize should have been %d, %d", dataSize, INT_SIZE * 5 + lenSha + lenFilename, lenSha);
		return false;
	}

	memcpy(f->sha, data + pos, lenSha);			pos += lenSha;
	memcpy(filename, data + pos, lenFilename); 	pos += lenFilename;

	filename[lenFilename] = '\0';
	String_strcpy(f->filename, filename);
	return true;
}

