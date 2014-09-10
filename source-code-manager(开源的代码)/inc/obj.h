#ifndef _OBJ_H
#define _OBJ_H
#include "commit.h"
#include "strings.h"
bool getBranchName(String const s);
bool setBranchName(String const s);

bool getCurrentIndexFile(String s);
bool readIndexFile(FileList index, String indexfile);
bool readTree(FileList tree, ShaBuffer treeSha);

bool copyFileToCache(File f);
bool moveFileFromCacheToRepo(File f);
bool copyFileFromRepo(File f);

bool copyTreeToRepo(File f);
bool copyTreeFromRepo(ShaBuffer tree, const char *dest, int mode);

bool getCurrentCommit(Commit c, ShaBuffer currentCommit);
bool setCurrentCommit(ShaBuffer commitSha);
bool compareIndexWithWorkingArea(void);
#endif
