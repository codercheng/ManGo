#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "common.h"
#include "sha1.h"
#include "files.h"

#include "sock.h"

#define WORKING_FOLDER_TAG  ".wokingfolder.tag"
#define PATH_LENGTH 256

int getRealPath(char *path, char *realPath);
bool checkFileExist(char *path);

int main(int argc, char **argv){
 	int sock_fd;
 	create_and_conn(&sock_fd, "127.0.0.1", 9999);
 	while(1){
	 	message msg;
	 	printf("input the cmd(0:add  1:get  2:send [3:add 4:get 5:wc-tag])\n");
	 	int input;
	 	scanf("%d", &input);
	 	getchar();//去除stdin中的'\n'
	 	switch(input){
	 		case INIT: {
	 			//create a tag in current working folder
	 			//to indicate this is the WC
	 			char mode[] = "0444";
	 			int ret;
	 			int m = strtol(mode, 0, 8);
	 			ret = creat(WORKING_FOLDER_TAG, m);
				if(ret == -1)
				{
					perror("creat file");
				}
				break;
	 		}
	 		case TEST_ADD: {
	 			int ret;
				printf("file need to transfer:");
				char path [256];
				char realpath[256];
				scanf("%s",path);

				if(!checkFileExist(path)){
					printf("Error:%s doesn't exist!\n", path);
					goto EXIT;
				}
				/*get real path, that is, have prefix*/
				ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;
				}
				printf("real path:%s\n", realpath);


				//mes send after all is ok
				msg.cmd = TEST_ADD;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}

				printf("-------------start send----------\n");
				Send(realpath, sock_fd);
				printf("-------------end send ----------\n");
				break;
				
	 		}

	 		case TEST_GET: {
	 			int ret;
				printf("file need to Get:");
				char path [256];
				char realpath[256];
				scanf("%s",path);

				/*get real path, that is, have prefix*/
				ret = getRealPath(path, realpath);
				if(ret == -1) {
					goto EXIT;//return -1;
				}
				printf("real path:%s\n", realpath);

				getchar();
				char version[32];
				scanf("%s", version);
				getchar();
				int byLatestVersion, recursive;
				scanf("%d %d", &byLatestVersion, &recursive);

				//msg send after all is ok
				msg.cmd = TEST_GET;
	 			ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
				printf("-------------start Get----------\n");
				clientGetFile(sock_fd, realpath, version, recursive, byLatestVersion);
				printf("-------------end Get ----------\n");
				break;
	 		}

	 		case ADD_FILES:{
	 			msg.cmd = ADD_FILES;
	 			int ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
	 			break;
	 		}
	 		case GET_FILES:{
	 			msg.cmd = GET_FILES;
			 	int ret = send(sock_fd, &msg, sizeof(msg), 0);
			 	if(ret == -1){
			 		perror("send msg\n");
				}
				//getFile(sock_fd);
	 			break;
	 		}
	 		case SEND_FILES:{
	 			msg.cmd = SEND_FILES;
			 	int ret = send(sock_fd, &msg, sizeof(msg), 0);
			 	if(ret == -1){
			 		perror("send msg\n");
				}
				//getFile(sock_fd);
	 			break;
	 		}
	 	}
	 	break;

	 	//getFile(sock_fd);
 	}
 	
EXIT:
 	close(sock_fd);
 	return 0;

}

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
	chdir(pwd);

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