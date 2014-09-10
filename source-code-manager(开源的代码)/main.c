#include <string.h>
#include <sys/stat.h>
#include "cmds.h"
#include "sha.h"
#include "filelist.h"

typedef int (*command_fn)(int argc, char *argv[]);
struct commands
{
	char commandName[16];
	command_fn function;
	char *helpmsg;
};


int main(int argc, char *argv[])
{
	struct commands cmds[] = {  {"sha", 			cmd_sha			, "compute hash of the files"},
								{"version",			cmd_version 	, "display version "},
								{"init",			cmd_init		, "intialize the repo"},
								{"branch",			cmd_branch		, "displays the current branch"},
								{"status",			cmd_status		, "show's current status wrt to index file"},
								{"add",			    cmd_add			, "adds file's or folders into the index file"},
								{"ls",			    cmd_ls			, "prints the file's"},
								{"commit",			cmd_commit		, "commits the current changes into repo"},
								{"info",			cmd_info		, "prints information about a commit"},
								{"rm",				cmd_rm			, "remove file from list"},
								{"checkout",		cmd_checkout	, "checkout a file/folder or a commit"},
							 };
	const int commands_len = sizeof(cmds)/sizeof(*cmds);

	int i, returnValue = 1;

	if(1 == argc)
	{
		LOG_INFO("Currently supported commands\n");
		for(i = 0; i < commands_len; i++)
		{
			LOG_INFO("    %-16s  %s", cmds[i].commandName, cmds[i].helpmsg);
		}
		return 0;
	}
	for(i = 0; i < commands_len; i++)
	{
		if(0 == strcmp(cmds[i].commandName, argv[1]))
		{
			returnValue = cmds[i].function(argc, argv);
			break;
		}
	}
	if(i == commands_len)
	{
		LOG_ERROR("command '%s' not found, to see list of commands run '%s' without arguments", argv[1], argv[0]);
	}
	return returnValue;
}
