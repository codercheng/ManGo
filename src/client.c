#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include "common.h"
#include "sha1.h"
#include "files.h"

#include "sock.h"

#define WORKING_FOLDER_TAG  ".wokingfolder.tag"
#define PATH_LENGTH 256

int getRealPath(char *path, char *realPath);
bool checkFileExist(char *path);
bool checkSetWorkingFolder();

int main(int argc, char **argv){
	if(argc != 3) {
		printf("usage:[./evclient2] [ip] [port]\n");
		return -1;
	}
 	int sock_fd;
 	create_and_conn(&sock_fd, argv[1], atoi(argv[2]));
 	// if(!checkSetWorkingFolder()){
 	// 	return -1;
 	// }
 	while(1){
	 	message msg;

	 	printf("+------------------------------------------------------+\n");
		printf("|                input the cmd                         |\n");
		printf("+------------------------------------------------------+\n");
		printf("|0: init    1:add        2:get    3:diff    4:history  |\n");
		printf("|5: do tag  6: show tag  7:get version by tag          |\n");
		printf("+------------------------------------------------------+\n");
	 	//printf("input the cmd 0:init  1:add  2:get  3:diff  4:history\n");
	 	printf("cmd:");
	 	int input;
	 	scanf("%d", &input);
	 	getchar();//去除stdin中的'\n'
	 	switch(input){
	 		case INIT: {
	 			//create a tag in current working folder
	 			//to indicate this is the WC
	 			char mode[] = "0444"; /*read-only*/
	 			int ret;
	 			int m = strtol(mode, 0, 8);
	 			ret = creat(WORKING_FOLDER_TAG, m);
				if(ret == -1)
				{
					perror("creat file");
				}
				break;
	 		}
	 		case ADD: {
	 			int ret;
				printf("file need to transfer:");
				char path [256];
				char realpath[256];
				scanf("%s", path);
				getchar();

				if(!checkFileExist(path)){
					printf("Error:%s doesn't exist in your working folder!\n", path);
					goto EXIT;
				}
				/*get real path, that is, have prefix*/
				ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;
				}
#ifdef _DEBUG_
				printf("real path:%s\n", realpath);
#endif

				char comment[512];
				printf("comment:");
				fgets(comment, sizeof(comment), stdin);
				comment[strlen(comment)-1] = '\0';

				//mes send after all is ok
				msg.cmd = ADD;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
#ifdef _DEBUG_
				printf("-------------start send----------\n");
#endif
				Send(realpath, sock_fd, comment);
#ifdef _DEBUG_
				printf("-------------end send ----------\n");
#endif

				char pwd[PATH_LENGTH];
				getcwd(pwd, PATH_LENGTH);
				chdir(pwd);
				break;
				
	 		}

	 		case GET: {
	 			int ret;
	 			
				printf("file need to Get:");
				char path [256];
				char realpath[256];
				memset(realpath, 0, sizeof(realpath));
				scanf("%s",path);

				/*get real path, that is, have prefix*/
				/*import: chdir is call, so every cmd, we chdir to the WC, and operate
				* the full name
				*/
				ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;//return -1;
				}
#ifdef _DEBUG_
				printf("real path:%s\n", realpath);
#endif

				getchar();
				char version[32];
				scanf("%s", version);
				getchar();
				int byLatestVersion, recursive;
				scanf("%d %d", &byLatestVersion, &recursive);

				//msg send after all is ok
				msg.cmd = GET;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
#ifdef _DEBUG_
				printf("-------------start Get----------\n");
#endif
				clientGetFile(sock_fd, realpath, version, recursive, byLatestVersion);
#ifdef _DEBUG_
				printf("-------------end Get ----------\n");
#endif

				//import: 
				char pwd[PATH_LENGTH];
				getcwd(pwd, PATH_LENGTH);
				chdir(pwd);
				break;
	 		}

	 		case DIFF: {
	 			//1. diff local with server latest version/history version
	 			//2. diff server two versions
	 			break;
	 		}
	 		case HISTORY: {
	 			printf("input the item:");
	 			char path[256];
	 			char realpath[256];
	 			memset(realpath, 0, sizeof(realpath));
	 			scanf("%s", path);
	 		// 	/*check item exist*/
	 		// 	if(!checkFileExist(path)){
				// 	printf("Error:%s doesn't exist in your working folder!\n", path);
				// 	goto EXIT;
				// }
				int ret;
	 			ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;//return -1;
				}
#ifdef _DEBUG_
				printf("realpath:%s-\n", realpath);
#endif

				//msg send after all is ok
				msg.cmd = HISTORY;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}

				//printf("-------------start show history----------\n");
				
				clientGetHistory(sock_fd, realpath);
				//printf("------------- end show history ----------\n");

				char pwd[PATH_LENGTH];
				getcwd(pwd, PATH_LENGTH);
				chdir(pwd);
	 			break;
	 		}
	 		case TAG: {
	 			printf("+------------------------------------------------------+\n");
	 			printf("|input the item name and version number you want to tag|\n");
	 			printf("+------------------------------------------------------+\n");
	 			printf("|usage:[item name] [version num] [tag name]            |\n");
	 			printf("|>>>test.c 12 v1.0.1                                   |\n");
	 			printf("|to tag latet version you can do like this:            |\n");
	 			printf("|>>>test.c latest v1.0.1                               |\n");
	 			printf("+------------------------------------------------------+\n");
	 			printf("input:");
	 			char path[256];
	 			char versionNumber[32];
	 			char realpath[256];
	 			char tagName[32];
	 			memset(realpath, 0, sizeof(realpath));
	 			scanf("%s %s %s", path, versionNumber, tagName);

	 			int ret;
	 			ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;//return -1;
				}

				//msg send after all is ok
				msg.cmd = TAG;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}

				clientDoTag(sock_fd, realpath, versionNumber, tagName);

				char pwd[PATH_LENGTH];
				getcwd(pwd, PATH_LENGTH);
				chdir(pwd);
	 			break;
	 		}

	 		case SHOW_TAG: {
	 			printf("input item name:");
				int ret;
				char path[256];
	 			char realpath[256];
	 			scanf("%s", path);

				ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;//return -1;
				}


	 			msg.cmd = SHOW_TAG;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
	 			clientGetTag(sock_fd, realpath);
	 			break;
	 		}

	 		case GET_VERSION_BY_TAG: {
	 			printf("input item name:");
				int ret;
				char path[256];
	 			char realpath[256];
	 			scanf("%s", path);

				ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;//return -1;
				}
				char tagName[32];
				printf("input tag name:");
				scanf("%s", tagName);

				msg.cmd = GET_VERSION_BY_TAG;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
	 			int version = clientGetVersionByTagName(sock_fd, tagName, realpath);
	 			if(version == -1) {
	 				printf("ERROR: the tag on this item is not exist!\n");
	 			} else {
	 				printf("version :%d\n", version);
	 			}
	 			break;
	 		}

	 		
	 		default: {
	 			
	 			printf("Error: wrong cmd\n");
	 			return -1;
	 		}

	 	}
	 	break;

	 	//getFile(sock_fd);
 	}
 	
EXIT:
 	close(sock_fd);
 	return 0;

}
// void setWorkingFolder() {
// 	//create a tag in current working folder
// 	//to indicate this is the WC
// 	printf("Do you want to set the current dir as working folder??(Y/N):");
// 	char choice;
// 	scnaf("%c", &choice);
// 	if('y' == tolower(choice)) {
// 		char mode[] = "0444"; /*read-only*/
// 		int ret;
// 		int m = strtol(mode, 0, 8);
// 		ret = creat(WORKING_FOLDER_TAG, m);
// 		if(ret == -1)
// 		{
// 			perror("creat file");
// 		}
// 	} else if ('n' == tolower(choice)) {

// 	}

// }

bool checkFileExist(char *path) {
	if(access(path, F_OK) != 0) { /*file not exist*/
		return false;
	}
	return true;
}

/*
* according to the working fold, get the real path name.
* e.g.
* the path is 'file.txt', but the working folder is not in 
* this folder, it is in the parent folder 'src'. so the real
* path name should be 'src/file.txt' which is the identification
* in the server
*
* how: serch the parent folder to search the wc-tag file
*/
int getRealPath(char *path, char *realpath) {
	char pwd[PATH_LENGTH];
	getcwd(pwd, PATH_LENGTH);
	char tmppwd[PATH_LENGTH];
	strcpy(tmppwd, pwd);
	/*get the real path*/
	while(1) {
		char tmp[PATH_LENGTH];
		if(access(WORKING_FOLDER_TAG, F_OK) != 0) { /*file not exist*/
			if(strcmp(tmppwd, "/") == 0)
			{
				printf("*** Set a WC first! ****\n");
				chdir(pwd);
				return -1;
			}
			getParent(tmp, tmppwd);
			if(strcmp(tmp, "") == 0)
				strcpy(tmp, "/");
			chdir(tmp);
			memset(tmppwd, 0, sizeof(tmppwd));
			strcpy(tmppwd, tmp);
		} else {
			break;
		}
	}
	/*chdir is set, when get finish*/
	//chdir(pwd);//wrong

	if(strlen(tmppwd) != strlen(pwd)) { /*path is not real path*/
		char *p;
		if(strcmp(tmppwd, "/") == 0) {
			p = pwd + 1;
		} else {
			p = pwd + strlen(tmppwd)+1; /*discard the front '/'*/
		}
		sprintf(realpath, "%s/%s", p, path);
	} else {
		strcpy(realpath, path);
	}

	return 0;
}

bool checkSetWorkingFolder() {
	char pwd[PATH_LENGTH];
	getcwd(pwd, PATH_LENGTH);
	char tmppwd[PATH_LENGTH];
	strcpy(tmppwd, pwd);
	
	while(1) {
		char tmp[PATH_LENGTH];
		if(access(WORKING_FOLDER_TAG, F_OK) != 0) { /*file not exist*/
			if(strcmp(tmppwd, "/") == 0)
			{
				printf("*** Set a WC first! ****\n");
				chdir(pwd);
				return false;
			}
			getParent(tmp, tmppwd);
			if(strcmp(tmp, "") == 0)
				strcpy(tmp, "/");
			chdir(tmp);
			memset(tmppwd, 0, sizeof(tmppwd));
			strcpy(tmppwd, tmp);
		} else {
			break;
		}
	}
	/*chdir is set, when get finish*/
	chdir(pwd);
	return true;
}