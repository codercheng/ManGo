#ifndef STRING_H_
#define STRING_H_
#include <stdlib.h>
#include "common.h"



struct _string;
typedef struct _string *String;

String String_Create(void);
void String_Delete(String s);

void String_add(String s, const String s1);
void String_clone(String s, const String s1);

void String_SetSize(String s, const uint32_t size);

void String_strcat(String s, const char *str);
void String_strcpy(String s, const char *str);

int String_strlen(const String s);
int String_strcmp(const String s, const char *s1);
int String_compare(const String s, const String s1);
const char* s_getstr(const String s);
int String_format(String s, const char *format, ...);
void String_NormalizeFolderName(String s);
void String_NormalizeFileName(String s);
#endif
