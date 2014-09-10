#ifndef _COMMIT_H_
#define _COMMIT_H_
#include <time.h>
#include "scm.h"
#include "sha.h"
#include "strings.h"
#include "common.h"

struct _Commit
{
	ShaBuffer 	tree;
	ShaBuffer 	parent0, parent1;
	time_t		rawtime;
	String		author;
	String		message;
};

typedef struct _Commit *Commit;

void Commit_Reset(Commit s);
Commit Commit_Create(void);
void Commit_Delete(Commit c);
bool Commit_SetTree(Commit c, const ShaBuffer tree);
bool Commit_SetParent(Commit c, const ShaBuffer parent0, const ShaBuffer parent1);
bool Commit_SetMessage(Commit c, const char *message);
bool Commit_SetAuthor(Commit c, const char *authorName, const char *authorEmailId);

bool WriteCommitFile(Commit s, ShaBuffer commit);
void PrintCommit(Commit s);
bool Commit_ReadCommitFile(Commit c, const ShaBuffer commitSha);
bool Commit_WriteCommitFile(Commit c, ShaBuffer commitSha);
#endif
