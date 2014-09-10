#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include "common.h"
#include "sha1.h"
#include "files.h"

#include "sock.h"


int main(int argc, char **argv){
 	int sock_fd;
 	create_and_conn(&sock_fd, "127.0.0.1", 9999);
 	while(1){
	 	message msg;
	 	printf("input the cmd(0:add  1:get  2:send [3:add 4:get])\n");
	 	int input;
	 	scanf("%d", &input);
	 	getchar();//去除stdin中的'\n'
	 	switch(input){
	 		case TEST_ADD: {
	 			msg.cmd = TEST_ADD;
	 			int ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
				printf("file need to transfer:");
				char path [256];
				scanf("%s",path);
				printf("-------------start send----------\n");
				Send(path, sock_fd);
				printf("-------------end send ----------\n");
				break;
				
	 		}

	 		case TEST_GET: {
	 			msg.cmd = TEST_GET;
	 			int ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
				printf("file need to Get:");
				char path [256];
				scanf("%s",path);
				getchar();
				char version[32];
				scanf("%s", version);
				getchar();
				int byLatestVersion, recursive;
				scanf("%d %d", &byLatestVersion, &recursive);
				printf("-------------start Get----------\n");
				clientGetFile(sock_fd, path, version, recursive, byLatestVersion);
				printf("-------------end Get ----------\n");
				break;
	 		}

	 		case ADD_FILES:{
	 			msg.cmd = ADD_FILES;
	 			int ret = send(sock_fd, &msg, sizeof(msg), 0);
	 			if(ret == -1){
			 		perror("send cmd msg\n");
				}
	 			
/*	 			addinfo info;*/
/*	 			strcpy(info.path, "test.txt");*/
/*	 			strcpy(info.comment, "init add test.txt");*/
/*	 			ShaBuffer sha;*/
/*				sha_file(info.path, sha);*/
/*				strcpy(info.sha1, (char *)sha);*/
/*				*/
/*				printf("sha:%s\n",sha);*/
/*				*/
/*				*/
/*				ret = send(sock_fd, &info, sizeof(info), 0);*/
/*	 			if(ret == -1){*/
/*			 		perror("send info\n");*/
/*				}*/
/*				int needSendFile;*/
/*				printf("get needSendFile info\n");*/
/*				ret = recv(sock_fd, &needSendFile, sizeof(needSendFile), 0);*/
/*				if(ret == -1){*/
/*			 		perror("recv needSendFile info\n");*/
/*				}*/
/*				printf("end get needSendFile info\n");*/
/*				if(needSendFile) {*/
/*					printf("begin send file\n");*/
/*					Send(info.path, sock_fd);*/
/*					printf("end send file\n");*/
/*				} else {*/
/*					printf("file is same as the server\n");*/
/*				}*/
/*			 	*/
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
	 	

	 	//getFile(sock_fd);
 	}
 	
 	
 	close(sock_fd);
 	return 0;

}
