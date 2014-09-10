#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <stdarg.h>
#include "strings.h"

#define MIN_STRING_SIZE		16

struct _string
{
	char *str;
	uint32_t strLen, strSize;
};

void String_SetSize(String s, const uint32_t size)
{
	if(s->strSize < size)
	{
		s->str = (char*)XREALLOC(s->str, size);
		memset(s->str+s->strSize, 0, size - s->strSize);
		s->strSize = size;
	}

	return;
}

String String_Create(void)
{
	String s;
	s = (String)XMALLOC(sizeof(struct _string));
	s->strLen = 0;
	s->strSize = MIN_STRING_SIZE;
	s->str = (char*)XMALLOC(s->strSize);
	memset(s->str, 0, s->strSize);
	return s;
}

void String_Delete(String s)
{
	XFREE(s->str);
	XFREE(s);
	return;
}

void String_clone(String s, const String s1)
{
	if(s1->strLen >= s->strSize)
	{
		String_SetSize(s, MIN_STRING_SIZE + s1->strLen);
	}
	s->strLen = s1->strLen;
	strncpy(s->str, s1->str, s1->strLen);
	return;
}

void String_add(String s, const String s1)
{
	if((s1->strLen + s->strLen) >= s->strSize)
	{
		String_SetSize(s, MIN_STRING_SIZE + s1->strLen + s->strLen);
	}
	s->strLen = s1->strLen+s->strLen - 1;
	strncat(s->str, s1->str, s1->strLen);
	return;
}

int String_compare(const String s, const String s1)
{
	return strcmp(s->str, s1->str);
}

int String_strcmp(const String s, const char *s1)
{
	return strcmp(s->str, s1);
}
void String_strcpy(String s, const char *str)
{
	if(NULL != str)
	{
		int len = strlen(str)+1;

		if(len >= s->strSize)
		{
			String_SetSize(s, MIN_STRING_SIZE + len);
		}
		s->strLen = len;
		strncpy(s->str, str, len);
	}
	else
	{
		LOG_ERROR("String_strcpy: trying to copy NULL string");
	}
	return;
}
void String_strcat(String s, const char *str)
{
	if(NULL != str)
	{
		int len = strlen(str);
		if((len+s->strLen) >= s->strSize)
		{
			String_SetSize(s, MIN_STRING_SIZE + len + s->strLen);
		}
		s->strLen += len;
		strncat(s->str, str, len);
	}
	return;
}

const char* s_getstr(const String s)
{
	return (const char*)s->str;
}

int String_strlen(const String s)
{
	return (int)(s->strLen-1);
}

void String_DebugPrint(const String s)
{
	printf("\nLen : %d", s->strLen);
	printf("\nSize : %d", s->strSize);
	printf("\nstr : %s", s->str);
	printf("\n");
}

int String_format(String s, const char *format, ...)
{
	int len = MIN_STRING_SIZE;
	va_list v;
	do
	{
		va_start(v, format);
		String_SetSize(s, len+MIN_STRING_SIZE);
		len = vsnprintf(s->str, s->strSize, format, v);
		va_end(v);
	}
	while(len >= s->strSize);
	s->strLen = len+1;
	return len;
}

/*Converts the foldername to the following format
 * ./<foldername>
 * */
void String_NormalizeFolderName(String s)
{
	if(isItFolder(s->str) || isItFile(s->str))
	{
	char *ptr, buffer[1024];
	ptr = realpath(s->str, NULL);
	getcwd(buffer, 1024);
	if(strncmp(ptr, buffer, strlen(buffer)) == 0)
	{
	    int len = strlen(buffer);
		strcpy(s->str, ".");
		strcat(s->str, ptr+len);
		s->strLen = strlen(s->str);
	}
	else
	{
		LOG_INFO("fatal: '%s' is outsize repo!", s->str);
		exit(1);
	}
	XFREE(ptr);
	}
	else
		String_NormalizeFileName(s);
	return;
}

void String_NormalizeFileName(String s)
{
	if(strncmp(s->str, "./", 2) != 0)
	{
		String_SetSize(s, s->strLen+5);
		memmove(s->str+2, s->str, s->strLen);
		s->str[0] = '.';
		s->str[1] = '/';
		s->strLen = s->strLen+2;
		s->str[s->strLen] = 0;
	}
	return;
}
