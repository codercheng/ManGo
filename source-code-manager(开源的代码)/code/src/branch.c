#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "cmds.h"
#include "sha.h"
#include "strings.h"
#include "filelist.h"
#include "common.h"
#include "scm.h"
#include "obj.h"
#include "commit.h"


bool printAllBranches(void)
{
	File *list;
	String branch;
	uint32_t num, i, len = strlen(SCM_BRANCH_FOLDER);
	FileList f;
	bool returnValue = false;

	f = FileList_Create();
	branch = String_Create();
	if(getBranchName(branch) == false)
	{
		LOG_ERROR("current branch not set!!");
		goto EXIT;
	}

	FileList_GetDirectoryConents(f, SCM_BRANCH_FOLDER,false,false);
	list = FileList_GetListDetails(f,&num);

	for(i = 0; i < num; i++)
	{
		if(strcmp(s_getstr(list[i]->filename)+len+3, s_getstr(branch)) == 0)
			LOG_INFO("*%s", s_getstr(list[i]->filename)+len+3);
		else
			LOG_INFO(" %s", s_getstr(list[i]->filename)+len+3);
	}
	returnValue = true;
EXIT:
	FileList_Delete(f);
	String_Delete(branch);
	return returnValue;
}

bool createNewBranch(String branchName)
{
	FILE *fp;
	bool returnValue = false;
	String temp = String_Create();
	String currentBranch = String_Create();
	Commit currentCommit = Commit_Create();
	ShaBuffer commitSha;
	String_format(temp, "%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName));
	if(isItFolder(s_getstr(temp)))
	{
		LOG_ERROR("branch '%s' already exist's. try with another name", s_getstr(branchName));
		goto EXIT;
	}
	if(false == getCurrentCommit(currentCommit,commitSha))
	{
		LOG_ERROR("commit not set in the current branch");
		goto EXIT;
	}
	if(0 != mkdir(s_getstr(temp), SCM_FOLDER_PERMISSION))
	{
		LOG_ERROR("fatal: mkdir('%s') failed(%d)", s_getstr(temp), errno);
		goto EXIT;
	}
	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_BRANCH_CACHE_FOLDER);
	if(0 != mkdir(s_getstr(temp), SCM_FOLDER_PERMISSION))
	{
		LOG_ERROR("fatal: mkdir('%s') failed(%d)", s_getstr(temp), errno);
		goto EXIT;
	}

	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_INDEX_FILENAME);
	copyTreeFromRepo(currentCommit->tree, s_getstr(temp), SCM_HEAD_FILE_PERMISSION);


	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branchName), SCM_COMMIT_FILENAME);
	fp = fopen(s_getstr(temp), "w");
	if(NULL == fp)
	{
		LOG_ERROR("unable to write into commit file '%s'", s_getstr(temp));
		goto EXIT;
	}
	fprintf(fp, "%s\n", commitSha);
	fclose(fp);
	LOG_INFO("created branch '%s' pointing to commit '%s'", s_getstr(branchName), commitSha);
EXIT:
	Commit_Delete(currentCommit);
	String_Delete(temp);
	String_Delete(currentBranch);
	return returnValue;
}

int difference(File ref, File n,DifferenceType type, void *data)
{
	String branch = (String)data;
	String temp = String_Create();
	switch(type)
	{
		case FILE_NEW:
				if(S_ISDIR(n->mode))
					mkdir(s_getstr(n->filename), n->mode);
				else
				{
					/*if file is not found in repo then check in cache*/
					String_format(temp, "%s/%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branch), SCM_BRANCH_CACHE_FOLDER, n->sha);
					if(isItFile(s_getstr(temp)))
					{
						/*File Found in cache*/
						decompressAndSave(s_getstr(temp),s_getstr(n->filename),n->mode);
					}
					else if(false == copyFileFromRepo(n))
					{
						/*File not found in repo... what to do now?*/
					}

				}
				break;
		case FILE_DELETED:
				if(isItFile(s_getstr(ref->filename)))
					unlink(s_getstr(ref->filename));
				else
				{
					String_format(temp, "rm -rf %s", s_getstr(ref->filename));
					system(s_getstr(temp));
					String_Delete(temp);
					return 0; //skip this folder
				}
				break;
		case FILE_MODIFIED:
				if(S_ISREG(n->mode))
				{
					/*if file is not found in repo then check in cache*/
					String_format(temp, "%s/%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branch), SCM_BRANCH_CACHE_FOLDER, n->sha);
					if(isItFile(s_getstr(temp)))
					{
						/*File Found in cache*/
						decompressAndSave(s_getstr(temp),s_getstr(n->filename),n->mode);
					}
					else if(false == copyFileFromRepo(n))
					{
						/*File not found in repo... what to do now?*/
					}
				}
				else
				{
					chmod(s_getstr(n->filename), n->mode);
				}
				break;
		case FILE_LAST_VALUE:
		default:
				break;
	}

	String_Delete(temp);
	return 1;
}
bool setWorkingAreaToBranch(String branch)
{
	String temp;
	bool returnValue = false;
	FileList currentList, nextList;

	currentList = FileList_Create();
	nextList = FileList_Create();
	temp = String_Create();

	/*check is the branch exist*/
	String_format(temp, "%s/%s", SCM_BRANCH_FOLDER, s_getstr(branch));
	if(false == isItFolder(s_getstr(temp)))
	{
		LOG_INFO("branch '%s' doesn't exist", s_getstr(branch));
		goto EXIT;
	}
	/*if the working area differes from index, then backout from branching*/
	if(true == compareIndexWithWorkingArea())
	{
		LOG_ERROR("working changes not reflected in index, add them to index and try again");
		goto EXIT;
	}
	if(false == readIndexFile(currentList, NULL))
	{
		goto EXIT;
	}
	String_format(temp, "%s/%s/%s", SCM_BRANCH_FOLDER, s_getstr(branch), SCM_INDEX_FILENAME);
	if(false == FileList_DeSerialize(nextList, s_getstr(temp)))
		goto EXIT;
	/* Now copy all the file of this branch to work area. The copy is done in
	 * difference() function. We first analyse what has modified in these 2 branch
	 * and just restore the changes.*/
	if(false == FileList_GetDifference(currentList,nextList,difference, branch))
	{
		goto EXIT;
	}
	setBranchName(branch);
	returnValue = true;
EXIT:
	FileList_Delete(nextList);
	FileList_Delete(currentList);
	String_Delete(temp);
	return returnValue;
}
int cmd_branch(int argc, char *argv[])
{
	String s, branchName;
	bool createBranch, setBranch;

	s = String_Create();
	branchName = String_Create();
	createBranch = setBranch = false;

	if(argc >= 3)
	{
		int o;
		while((o = getopt(argc, argv, "hc:s:")) != -1)
		{
			switch(o)
			{
				case 'c':
					String_strcpy(branchName, optarg);
					createBranch = true;
					break;
				case 's':
					String_strcpy(branchName, optarg);
					setBranch = true;
					break;
				case 'h':
				default:
					LOG_ERROR("usage %s %s [-c <create branch>] [-s <set branch>]\nWhen run with no arguments prints all the branchs", argv[0], argv[1]);
					goto EXIT;
			}
		}
	}
	if((true == setBranch) && (true == createBranch))
	{
		LOG_ERROR("Lets do one at a time!! first create the branch and then try to set it");
		goto EXIT;
	}
	if((false == setBranch)	&& (false == createBranch))
	{
		printAllBranches();
	}

	if(true == createBranch)
	{
		createNewBranch(branchName);
	}
	if(true == setBranch)
	{
		if(true == setWorkingAreaToBranch(branchName))
			LOG_INFO("Switch to branch '%s'", s_getstr(branchName));
	}
EXIT:
	String_Delete(s);
	String_Delete(branchName);
	return 0;
}
