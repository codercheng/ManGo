#ifndef SOCK_H_
#define SOCK_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#define USE_SENDFILE_   1

/*也是可以留着在配置里面修改的*/
#define BUFSIZE 4096*1024

#define HANDLE_ERROR_EXIT(error_string) do{perror(error_string);exit(-1);}while(0)

//typedef struct{
//	char filename[236];
//	char filelen[16];
//	int cmd;
//}FileHeader;//256 byte

/*server*/
void create_bind_listen(int *listen_fd, int port, int backlog){

	*listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == *listen_fd){
		HANDLE_ERROR_EXIT("can not create sock");
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_port = htons(port);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int ret;
	ret = bind(*listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if(-1 == ret){
		HANDLE_ERROR_EXIT("can not bind");
	}
	ret = listen(*listen_fd, backlog);
	if(-1 == ret){
		HANDLE_ERROR_EXIT("can not listen");
	}
}

/*client*/
void create_and_conn(int *sock_fd, char *ip, int port){
	*sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == *sock_fd){
		HANDLE_ERROR_EXIT("can not create sock");
	}
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip);

	int ret;
	ret = connect(*sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr));
	if(-1 == ret){
		HANDLE_ERROR_EXIT("can not bind");
	}
}

///*send file info*/
//void SendFileInfo(char *path, int conn_fd, off_t size, int end){
//	FileHeader header;
//	//printf("^^^^^^%s^^^%d\n",path,(int)size);
//	strncpy(header.filename, path, strlen(path)+1);
//	//header.filename[strlen(header.filename)]='\0';
//	sprintf(header.filelen, "%d", (int)size);
//	header.cmd = end;//0 not end

//	int ret;
//	ret = send(conn_fd, &header, sizeof(header), 0);
//	if(-1 == ret){
//		perror("send file name");
//	}
//	if(end != 1)
//		printf("filename:%s, file len:%s Byte\n",header.filename,header.filelen);
//}

//void processPathName(char *);//declare
//void getFile(int conn_fd){
//	while(1){
//		/*get file name*/
//		int ret;
//		FileHeader header;
//		ret = recv(conn_fd, &header, sizeof(header), 0);
//		if(-1 == ret){
//			perror("recv fileinfo");
//		}
//		if(header.cmd == 1)//end of transfer
//			break;	
//		
//		processPathName(header.filename);// now , main for mkdir recursively
//	
//		printf("------->%s<---\n",header.filename);
//		char cwd[512];
//		getcwd(cwd,512);
//		printf("cwd:%s\n",cwd);
//	
//		/*if file exists, override or cancel*/
//		int flag = 1;//write or not
//		/*file exist*/
//		if(access(header.filename,F_OK) == 0){
//			//fprintf(stderr,"current dir have a file with the same name[%s]!", header.filename);
//			printf("current dir have a file with the same name[%s]! continue?(yes or no)\n", header.filename);
//			
//			while(1){
//				char cmd[10];
//				fgets(cmd, 10, stdin);
//				cmd[strlen(cmd)-1]='\0';// discard '\n' 
//				printf("cmd:%s-\n",cmd);
//				if(!strncmp(cmd,"no",2)){
//					flag = 0;
//					break;
//				}else if(!strncmp(cmd, "yes", 3)){
//					break;
//				}else{
//					 printf("invalid input! please input again[yes or no]!\n");
//				}
//			}
//		}
//		if(flag){
//			FILE *fp = fopen(header.filename, "w");
//			int byteleft;
//			byteleft = atoi(header.filelen);


//			int len;
//			char buf[BUFSIZE];
//			while(byteleft){
//				printf("byte left:%d\n",byteleft);
//				len = recv(conn_fd, buf,byteleft>=BUFSIZE?BUFSIZE-1:byteleft, 0);//代码应该在这个地方有问题
//				printf("len recv:%d\n",len);
//				if(len == 0){
//					printf("server quit...\n");
//					break;
//				}else if(len < 0){
//					printf("receive error...\n");
//					break;
//				}
//				byteleft-=len;
//				fwrite(buf, sizeof(char), len, fp);
//				//printf("---------------\n%s\n--------------\n",buf);
//				memset(buf, 0, sizeof(buf));
//			
//			}
//			fclose(fp);
//		}
//	}
//}


///*send file*/
//void SendFile(char *path, int conn_fd, off_t size){

//	SendFileInfo(path, conn_fd, size, 0);//发送文件名
//#ifdef USE_SENDFILE_
//	//struct stat s;
//	int filefd = open(path, O_RDONLY);
//	sendfile(conn_fd, filefd, NULL, size);
//	close(filefd);
//#else
//	FILE *fp = fopen(path, "r");
//	if(fp == NULL){
//		perror("open file");
//	}
//	int file_size;
//	int len;
//	char buf[BUFSIZE];
//	while((file_size = fread(buf, sizeof(char), BUFSIZE, fp)) > 0){
//		len = send(conn_fd, buf, file_size, 0);
//		memset(buf, 0, sizeof(buf));
//		if(len < 0)	{
//			perror("send");
//		}
//		else if(len == 0)
//		{
//			perror("client quit the connection");
//		}
//	}
//	fclose(fp);
//#endif
//}
///*send dir*/
//void SendDir(char *path, int conn_fd){
//	struct dirent *temp_path;
//	struct stat s;
//	DIR *dir;
//	char newpath[512];

//	dir = opendir(path);
//	if(dir == NULL){
//		perror("opendir error");
//		return;
//	}
//	while((temp_path = readdir(dir))!=NULL){
//		
//		if(!strcmp(temp_path->d_name,".")||!strcmp(temp_path->d_name,".."))
//			continue;
//		sprintf(newpath, "%s/%s", path, temp_path->d_name);
//		
//		lstat(newpath, &s);
//		if(S_ISDIR(s.st_mode)){
//			SendDir(newpath, conn_fd);
//		}else if(S_ISREG(s.st_mode)){
//			SendFile(newpath, conn_fd, s.st_size);
//		}
//	}
//	closedir(dir);
//}
///*send file or dir*/
//void Send(char *path, int conn_fd){
//	struct stat s;
//	lstat(path, &s);
//	if(S_ISREG(s.st_mode)){
//		SendFile(path, conn_fd, s.st_size);
//	}else if(S_ISDIR(s.st_mode)){
//		SendDir(path, conn_fd);
//	}
//	SendFileInfo(path, conn_fd, 0, 1);//1 for end of file transfer
//}


///*
//* parse the fullpath name(/chengshuguang/a.txt), get the filename(a.txt) and the parent dir(/chenshuguang/).
//*/

//int getFileName(char* filename,char *parent, const char* fullpath, int maxlen) {
//  char strtmp[128], *head;
//  //char strtmp2[128], *head2;
//  const char *tail;
//  int tmp;
//  tmp = strlen(fullpath);
//  tail = fullpath + tmp - 1;
//  while (*tail == '/') /*去掉结尾所有的‘/’*/
//    tail--;
//  head = strtmp;
// 
//  while (tail>=fullpath && *tail!='/') /*得到filename的倒序*/
//    *head++ = *tail--;
//  strncpy(parent, fullpath, tail-fullpath+1);
//  parent[tail-fullpath+1] = '\0';
//  head--;
//  
//  while (maxlen>0 && head>=strtmp) {
//    *filename++ = *head--;
//    maxlen--;
//  }
//  if (maxlen>0) {
//    *filename = '\0';
//    return 0; //
//  }
//  else return -1; /*文件名太长*/
//}


///*
//*1. make dir if the dir does not exit
//*2. create the file if does not exit
//*/
//void processPathName(char *fullpath){
//	char *temp;
//	temp = fullpath;
//	int dircount = 0;
//	while(1){
//		char dir[128];
//		while((*temp == '/'||*temp ==' ') && *temp != '\n')
//			temp++;
//		char *slash = strchr(temp,'/');
//		if(slash == NULL)
//			break;
//		strncpy(dir,temp,slash-temp);
//		dir[slash-temp]='\0';
//		if(access(dir,F_OK)){
//			if(mkdir(dir, 0755) < 0){
//			fprintf(stderr, "make dir [%s] error\n", dir);
//			}
//		}	
//		chdir(dir);
//		temp = slash +1;
//		dircount++;
//		//在这个地方处理文件名怎么样？？，感觉不错
//		//
//		//
//		//
//		//
//		//Todo:
//	}
//	while(dircount--){
//		chdir("..");//返回原来的目录
//	}
//}

#endif
