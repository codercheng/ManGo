#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cmds.h"
#include "sha.h"
#include "strings.h"
#include "filelist.h"
#include "common.h"
#include "obj.h"
#include "scm.h"

bool getBranchName(String const s)
{
	FILE *fp;
	char branchName[MAX_BUFFER_SIZE], type[MAX_BUFFER_SIZE];
	bool returnValue = false;
	if(false == isItFile(SCM_HEAD_FILE))
	{
		LOG_ERROR("fatal: getBranchName() HEAD file doesn't exist");
		goto EXIT;
	}
	fp = fopen(SCM_HEAD_FILE, "r");
	if(NULL == fp)
	{
		LOG_ERROR("fatal: getBranchName(): unable to open HEAD file");
		goto EXIT;
	}
	if(2 != fscanf(fp, "%s %s", type, branchName))
		goto EXIT;
	if(NULL == strstr(type, "branch:"))
	{
		LOG_ERROR("getBranchName() HEAD file format unrecogized!!");
		fclose(fp);
		goto EXIT;
	}
	String_strcpy(s, branchName);
	returnValue = true;
	fclose(fp);
EXIT:
	return returnValue;
}
bool setBranchName(String const s)
{
	FILE *fp;
	bool returnValue = false;

	fp = fopen(SCM_HEAD_FILE, "w");
	if(NULL == fp)
	{
		LOG_ERROR("fatal: setBranchName(): unable to open HEAD file");
		goto EXIT;
	}
	fprintf(fp, "branch: %s\n", s_getstr(s));
	returnValue = true;
	fclose(fp);
EXIT:
	return returnValue;
}
bool getCurrentIndexFile(String s)
{
	bool returnValue = false;
	String s1 = String_Create();

	if(false == getBranchName(s1))
		goto EXIT;
	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(s1), SCM_INDEX_FILENAME);
	returnValue = true;
EXIT:
	String_Delete(s1);
	return returnValue;
}

bool readIndexFile(FileList index, String indexfile)
{
	bool flag = false;
	String filename = String_Create();
	if(true == getCurrentIndexFile(filename))
	{
		if(true == FileList_DeSerialize(index, s_getstr(filename)))
		{
			if(NULL != indexfile)
			String_clone(indexfile, filename);
			flag = true;
		}
	}
	String_Delete(filename);
	return flag;
}

bool readTree(FileList tree, ShaBuffer treeSha)
{
	bool returnValue = false;
	String filename = String_Create(), temp = String_Create();
	String_format(filename, "%s/%s", SCM_TREE_FOLDER, treeSha);
	String_format(temp, "%s/%s", SCM_TEMP_FOLDER, treeSha);
	decompressAndSave(s_getstr(filename),s_getstr(temp), S_IRWXU | S_IRWXO | S_IRWXG);
	returnValue = FileList_DeSerialize(tree,s_getstr(temp));
	unlink(s_getstr(temp));
	String_Delete(filename);
	String_Delete(temp);
	return returnValue;
}
bool getCurrentCommitFile(String s)
{
	bool returnValue = false;
	String s1 = String_Create();

	if(false == getBranchName(s1))
		goto EXIT;
	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(s1), SCM_COMMIT_FILENAME);
	returnValue = true;
EXIT:
	String_Delete(s1);
	return returnValue;
}


bool setCurrentCommit(ShaBuffer currentCommit)
{
	int fd, dummy;
	bool returnValue = false;
	String s = String_Create(), s1 = String_Create();

	if(strlen((char*)currentCommit) != SHA_HASH_LENGTH)
		goto EXIT;
	if(false == getBranchName(s1))
		goto EXIT;
	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(s1), SCM_COMMIT_FILENAME);
	fd = open(s_getstr(s), O_WRONLY);
	if(fd > 0)
	{
		String_format(s, "%s\n", currentCommit);
		dummy = write(fd, s_getstr(s), String_strlen(s));
		close(fd);
		returnValue = true;
	}

EXIT:
	String_Delete(s);
	String_Delete(s1);
	return returnValue;
}
bool getCurrentCommit(Commit c, ShaBuffer currentCommit)
{
	struct stat st;
	bool returnValue = false;
	String s = String_Create(), s1 = String_Create();

	if(false == getBranchName(s1))
		goto EXIT;
	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(s1), SCM_COMMIT_FILENAME);
	sha_reset(currentCommit);
	if((0 == stat(s_getstr(s), &st)) && (SHA_HASH_LENGTH <= st.st_size))
	{
		int fd = open(s_getstr(s), O_RDONLY);
		if(fd > 0)
		{
			uint32_t dummy;
			dummy = read(fd, currentCommit, SHA_HASH_LENGTH);
			close(fd);
			if(false == Commit_ReadCommitFile(c,currentCommit))
			{
				sha_reset(currentCommit);
				goto EXIT;
			}
		}
		returnValue = true;
	}
EXIT:
	String_Delete(s);
	String_Delete(s1);
	return returnValue;
}

/*copies files to the branch cache...*/
bool copyFileToCache(File f)
{
	bool returnValue = false;
	if(S_ISREG(f->mode))
	{
		String s, branchName;
		if(strlen((char*)f->sha) !=  SHA_HASH_LENGTH)
		{
			LOG_ERROR("copyFileToRepo: trying to copy file whose sha is not computed %d", strlen((char*)f->sha));
			return false;
		}
		s = String_Create();
		branchName = String_Create();

		if(false == getBranchName(branchName))
		{
			goto EXIT;
		}
		String_format(s, "%s/%s", SCM_BLOB_FOLDER, f->sha);

		returnValue = true;
		/*Check if this file already exist in the repo*/
		if(false == isItFile(s_getstr(s)))
		{
			/*check if the file exist in the cache. if it doesn't then copy it to the cache..*/
			String_format(s, "%s/%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_BRANCH_CACHE_FOLDER, f->sha);
			if(false == isItFile(s_getstr(s)))
				returnValue = compressAndSave(s_getstr(f->filename), s_getstr(s), SCM_OBJECT_FILE_PERMISSION);
		}
EXIT:
		String_Delete(s);
		String_Delete(branchName);
	}
	return returnValue;
}

bool copyTreeToRepo(File f)
{
	bool returnValue = false;
	String s;
	s = String_Create();

	String_format(s, "%s/%s", SCM_TREE_FOLDER, f->sha);

	returnValue = true;
	/*Check if this file already exist in the repo*/
	if(false == isItFile(s_getstr(s)))
	{
		returnValue = compressAndSave(s_getstr(f->filename), s_getstr(s), SCM_OBJECT_FILE_PERMISSION);
	}
	String_Delete(s);
	return returnValue;
}
bool copyTreeFromRepo(ShaBuffer tree, const char *dest, int mode)
{
	bool returnValue = false;
	String s;
	s = String_Create();

	String_format(s, "%s/%s", SCM_TREE_FOLDER, tree);

	/*Check if this file already exist in the repo*/
	if(true == isItFile(s_getstr(s)))
	{
		returnValue = decompressAndSave(s_getstr(s), dest, mode);
		returnValue = true;
	}
	else
	{
		LOG_ERROR("FATAL: tree '%s' not found", tree);
	}
	String_Delete(s);
	return returnValue;
}
/*copies file from the branch cache to the blob repo...*/
bool moveFileFromCacheToRepo(File f)
{
	bool returnValue = false;

	if(S_ISREG(f->mode))
	{

		String s, branchName, s1;
		if(strlen((char*)f->sha) !=  SHA_HASH_LENGTH)
		{
			LOG_ERROR("copyFileToRepo: trying to copy file whose sha is not computed %d", strlen((char*)f->sha));
			return false;
		}
		s = String_Create();
		s1 = String_Create();
		branchName = String_Create();

		if(false == getBranchName(branchName))
		{
			goto EXIT;
		}
		String_format(s1, "%s/%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_BRANCH_CACHE_FOLDER, f->sha);
		String_format(s, "%s/%s", SCM_BLOB_FOLDER, f->sha);
		/*check if the file is in the cache... if it not then it should already be in repo.
		 * if it is not in repo, then where is it?*/
		if(true == isItFile(s_getstr(s1)))
		{
			rename(s_getstr(s1), s_getstr(s));
			chmod(s_getstr(s), SCM_OBJECT_FILE_PERMISSION);
			returnValue = true;
		}
		else if(false == isItFile(s_getstr(s)))
		{
			LOG_ERROR("file '%s' is not in cache nor in repo... (this should neven happen, state may be invalid)", s_getstr(f->filename));
			LOG_ERROR("copying file from working area to repo..");
		}
EXIT:
		String_Delete(s);
		String_Delete(s1);
		String_Delete(branchName);
	}

	return returnValue;
}

bool copyFileFromRepo(File f)
{
	String path, branchName;
	bool returnValue = false;
	if(!S_ISREG(f->mode))
		return false;
	path = String_Create();
	branchName = String_Create();
	if(false == getBranchName(branchName))
		goto EXIT;

	String_format(path, "%s/%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_BRANCH_CACHE_FOLDER, f->sha);
	if(!isItFile(s_getstr(path)))
	{
		String_format(path, "%s/%s", SCM_BLOB_FOLDER, f->sha);
		if(!isItFile(s_getstr(path)))
		{
			LOG_ERROR("copyFiletoWorkArea: blob '%s' not found in repo", f->sha);
			goto EXIT;
		}
	}
	returnValue = decompressAndSave(s_getstr(path),s_getstr(f->filename),f->mode);
	if(false == returnValue)
	{
		LOG_ERROR("copyFiletoWorkArea: unable to copy '%s' back to working area", s_getstr(f->filename));
	}
EXIT:
	String_Delete(path);
	String_Delete(branchName);
	return returnValue;
}

struct diff
{
	int n, d, m;
};
static int differences(File ref, File n, DifferenceType type, void *data)
{
	struct diff *p = (struct diff*)data;
	switch(type)
	{
		case FILE_NEW:
			p->n++;
			break;
		case FILE_DELETED:
			p->d++;
			break;
		case FILE_MODIFIED:
			if(S_ISREG(ref->mode))
			{
				if(ref->mode !=  n->mode)
					p->m++;
				else
				{
					sha_file(s_getstr(n->filename),n->sha);
					if(false == sha_compare(ref->sha, n->sha))
					p->m++;
				}
			}
			else if(ref->mode !=  n->mode)
				p->m++;
			break;
		case FILE_LAST_VALUE:
		default:
			break;
	}
	return 1;
}

/*Returns true if the working area is not same as the indexfile*/
bool compareIndexWithWorkingArea(void)
{
	bool returnValue = false;
	FileList indexlist, workingArea;
	struct diff d = {0, 0, 0};
	indexlist = FileList_Create();
	workingArea = FileList_Create();


	if(false == readIndexFile(indexlist, NULL))
		goto EXIT;

	if(FileList_GetListLength(indexlist) == 0)
	{
		LOG_ERROR("index file is empty!!!");
		goto EXIT;
	}
	FileList_GetDirectoryConents(workingArea, "./", true, false);
	FileList_GetDifference(indexlist, workingArea, differences, &d);
	if(d.m > 0)
		returnValue = true;
EXIT:
	FileList_Delete(indexlist);
	FileList_Delete(workingArea);
	return returnValue;
}
