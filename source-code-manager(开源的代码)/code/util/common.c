#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <zlib.h>
#include <assert.h>
#include "common.h"

#define MIN_MEMORY_TO_ALLOCATE	16

static int numOfBytesAllocated = 0;
void* XMALLOC(size_t size)
{
	void *ptr;
	if(MIN_MEMORY_TO_ALLOCATE > size)
		size = MIN_MEMORY_TO_ALLOCATE;
	numOfBytesAllocated += size;
	ptr = malloc(size);
	if(NULL == ptr)
	{
		LOG_ERROR("unable to allocate memory of size %d", size);
		exit(1);
	}
	return ptr;
}

void PrintAllocatedBytes(void)
{
	LOG_INFO("numOfBytesAllocated :%d", numOfBytesAllocated);
	return;
}
void* XREALLOC(void *p, size_t size)
{
	void *ptr;
	ptr = realloc(p, size);
	if(NULL == ptr)
	{
		LOG_ERROR("unable to [Re]allocate memory of size %d", size);
		exit(1);
	}
	return ptr;
}


int Nstrlen(const char *str)
{
	int len = 0 ;
	if(NULL != str)
	{
		while(str[len] != 0)
			len++;
	}
	return len;
}
int getTime(char buffer[64])
{
	time_t t;
	struct tm *tim;
	t = time(NULL);
	tim = localtime(&t);
	strftime(buffer, 64, "%a %b %d  %H:%M:%S  %Y", tim);
	return 0;
}

bool isItFolder(const char *name)
{
	struct stat s;
	if(0 == stat(name, &s))
	{
		if(S_ISDIR(s.st_mode))
			return true;
	}
	return false;
}

bool isItFile(const char *name)
{
	struct stat s;
	if(0 == stat(name, &s))
	{
		if(S_ISREG(s.st_mode))
			return true;
	}
	return false;
}



/*This below is got from zpipe.c, which comes part of the zlib library.
 * The 2 functions are used to compress and decompress files...*/

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
bool compressAndSave(const char *sourceFile, const char *destFile, int mode)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
	FILE *source, *dest;

	source = fopen(sourceFile, "r");
	if(NULL == source)
	{
		LOG_ERROR("unable to open file '%s'", sourceFile);
		return false;
	}
	dest = fopen(destFile, "w");
	if(NULL == dest)
	{
		LOG_ERROR("unable to open file '%s'", destFile);
		fclose(source);
		return false;
	}
    /* allocate deflate state */
    strm.zalloc = NULL;
    strm.zfree = NULL;
    strm.opaque = NULL;
    ret = deflateInit(&strm, Z_BEST_SPEED);
    if (ret != Z_OK)
        return ret;

    /* compress until end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return false;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return false;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
	fclose(source);
	fclose(dest);
	chmod(destFile, mode);
    /* clean up and return */
    (void)deflateEnd(&strm);
    return true;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
bool decompressAndSave(const char *sourceFile, const char *destFile, int mode)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];
	FILE *source, *dest;
	source = fopen(sourceFile, "r");
	if(NULL == source)
	{
		LOG_ERROR("unable to open file '%s'", sourceFile);
		return false;
	}
	dest = fopen(destFile, "w");
	if(NULL == destFile)
	{
		LOG_ERROR("unable to open file '%s'", destFile);
		fclose(source);
		return false;
	}
    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {
        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return false;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return false;
            }
            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

	fclose(source);
	fclose(dest);
	chmod(destFile, mode);
    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? true : false;
}
bool copyFile(const char *source, const char *destination, const int mode)
{
	bool returnValue = false;
	int fd_s, fd_d, len;
	char buffer[MAX_BUFFER_SIZE];

	fd_s = open(source, O_RDONLY);
	if(0 > fd_s)
	{
		LOG_ERROR("open(%s) failed(%d)", source, errno);
		goto EXIT;
	}
	fd_d = open(destination, O_CREAT | O_TRUNC | O_WRONLY, mode);
	if(0 > fd_d)
	{
		LOG_ERROR("open(%s) failed(%d)", destination, errno);
		close(fd_s);
		goto EXIT;
	}

	while((len = read(fd_s, buffer, MAX_BUFFER_SIZE)) != 0)
	{
		if(len != write(fd_d, buffer, len))
		{
			LOG_ERROR("write() failed(%d) to write specified number of bytes ", errno);
			returnValue = false;
			close(fd_s);
			close(fd_d);
			goto EXIT;
		}
	}

	returnValue = true;
	close(fd_s);
	close(fd_d);

	chmod(destination, mode); /*Set the mode of the files..*/
EXIT:
	return returnValue;
}
