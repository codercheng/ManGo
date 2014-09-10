#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "scm.h"
#include "filelist.h"

static bool setBranchInHEAD(const char *branchName)
{
	int fd, ch;
	bool returnValue = false;
	String s = String_Create();
	fd = open(SCM_HEAD_FILE, O_CREAT | O_TRUNC | O_WRONLY, SCM_HEAD_FILE_PERMISSION);
	if(-1 == fd)
	{
		LOG_ERROR("fatal: setBranch: open('%s') failed(%d)", SCM_HEAD_FILE, errno);
		goto EXIT;
	}
	/*Write the branch name to 'head' file*/
	String_format(s, "branch: %s", branchName);
	if(write(fd, s_getstr(s), String_strlen(s)) != String_strlen(s))
	{
		LOG_ERROR("write() failed with errno %d", errno);
		close(fd);
		goto EXIT;
	}
	/*write a new line character*/
	ch = '\n';
	ch = write(fd, &ch, 1);
	close(fd);
	returnValue = true;
EXIT:
	String_Delete(s);
	return returnValue;
}

static bool CreateFirstIndexFile(const char *path)
{
	FileList f;
	bool returnValue;
	f = FileList_Create();
	FileList_ResetList(f);
	FileList_InsertFile(f,".",false);
	returnValue = FileList_Serialize(f, path);
	FileList_Delete(f);
	return returnValue;
}

static bool createEmptyCommitFile(const char *path)
{
	int fd, dummy;
	char *str = "\0";
	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, SCM_HEAD_FILE_PERMISSION);
	if(fd < 0)
	{
		LOG_ERROR("createEmptyCommitFile: open('%s') failed(%d)", path, errno);
		return false;
	}
	dummy = write(fd, str, 1);
	close(fd);
	return true;
}
int cmd_init(int argc, char *argv[])
{
	int i;
	String s;
	char *folders[] = { SCM_FOLDER,
						SCM_BRANCH_FOLDER,
						SCM_OBJECTS_FOLDER,
						SCM_BLOB_FOLDER,
						SCM_COMMIT_FOLDER,
						SCM_TREE_FOLDER,
						SCM_TEMP_FOLDER};
	const int size = sizeof(folders)/sizeof(folders[0]);

	if(true == isItFolder(".scm"))
	{
		LOG_ERROR("fatal: delete the .scm folder then try again");
		return 1;
	}

	/*create all the folders*/
	for(i = 0; i < size; i++)
	{
		if(0 !=	mkdir(folders[i], SCM_FOLDER_PERMISSION))
		{
			LOG_ERROR("fatal: mkdir('%s') failed(%d)", folders[i], errno);
			return 1;
		}
	}

	s = String_Create();
	String_format(s, "%s/%s", SCM_BRANCH_FOLDER, SCM_DEFAULT_BRANCH);
	/*Create the branch folder*/
	if(0 != mkdir(s_getstr(s), SCM_FOLDER_PERMISSION))
	{
		LOG_ERROR("mkdir('%s') failed(%d)", s_getstr(s), errno);
		String_Delete(s);
		return 1;
	}

	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, SCM_DEFAULT_BRANCH, SCM_BRANCH_CACHE_FOLDER);
	/*Create the branch folder*/
	if(0 != mkdir(s_getstr(s), SCM_FOLDER_PERMISSION))
	{
		LOG_ERROR("mkdir('%s') failed(%d)", s_getstr(s), errno);
		String_Delete(s);
		return 1;
	}
	/*set the branch name in 'HEAD' file*/
	setBranchInHEAD(SCM_DEFAULT_BRANCH);

	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, SCM_DEFAULT_BRANCH, SCM_INDEX_FILENAME);
	/*Create a empty index file*/
	if(false == CreateFirstIndexFile(s_getstr(s)))
	{
		LOG_INFO("CreateEmptyIndexFile() failed()");
	}

	String_format(s, "%s/%s/%s", SCM_BRANCH_FOLDER, SCM_DEFAULT_BRANCH, SCM_COMMIT_FILENAME);
	createEmptyCommitFile(s_getstr(s));
	String_Delete(s);
	return 0;
}
