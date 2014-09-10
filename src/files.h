#ifndef FILES_H_
#define FILES_H_

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
#include <time.h>
#include <utime.h>
#include "sha1.h"
#include "conn_pool.h"
#include "mysql_encap.h"
#include "common.h"
#include "filePathLinkedlist.h"

#define USE_SENDFILE_   1

/*也是可以留着在配置里面修改的*/
#define BUFSIZE 4096*1024
#define DIFF_AND_MERGE_TOOL_PATH "/home/chengshuguang/meld-1.8.4/bin/meld"

#define HANDLE_ERROR_EXIT(error_string) do{perror(error_string);exit(-1);}while(0)


//for add cmd
typedef struct {
	char filename[256]; // file path
	char filelen[16];   //file size
	char sha1[41];  	//sha1 of the file
	long m_time;        //modified time 
	int  isEnd;         //end of the file/dir transfer
	int  isDir;         
	char comment[512]; //comment of the add operation
}FileHeader;


//for get cmd
//client send to server
typedef struct {
	char filename[256];    //file path
	int  byLatestVersion; //is to get the latest version
	char  versionNum[32];       //when get history, what version 
	//int  isDir;            //get file? or folder?
	int  isRecursive;      //when isDir, get the folder recusively?
}GetHeader;

//when get, server send to client
//used in Get cmd, in server
typedef struct {
	char filename[256];
	char filelen[16];
	char sha1[41];
	char m_time[31];
	int isDir;
	int isEnd;
}FileInfo;

typedef struct {
	char versionNumber[32];
	char user[32];
	char comment[512];
}HistroyInfo;


typedef struct {
	char filename[256];
	char versionNumber[32];
	char tagName[32];
}TagInfo;


/*
* this struct is used when op finish 
* and return op result to client
*/
typedef struct {
	int  success;
	char description[128];
}TaskResult;

/*
* @path: file path to send
* @conn_fd: sock fd of the connection
* @size: file len
* @isDir: whether the item to send is dir
* @end: is the end of this transfer of files/dir
*/
void SendFileInfo(char *path, int conn_fd, off_t size, time_t m_time, int isDir, int end, char *comment) {
	FileHeader header;
	if (end != 1) {
		strncpy(header.filename, path, strlen(path)+1);
		sprintf(header.filelen, "%d", (int)size);
	
		ShaBuffer sha;
		sha_file(header.filename, sha);
		strcpy(header.sha1, (char *)sha);
		header.m_time = m_time;
#ifdef _DEBUG_
		printf("sha:%s\n",sha);
#endif
		
		//comment 这个地方要改善，目前设置为空
		//*header.comment = '\0';
		strcpy(header.comment, comment);
		header.isDir = isDir;
	}
	
	header.isEnd = end;//0 not end
	
	
	int ret;
	ret = send(conn_fd, &header, sizeof(header), 0);
	if(-1 == ret){
		perror("send file name");
	}
#ifdef _DEBUG_
	if(end != 1)
		printf("filename:%s, file len:%s Byte\n",header.filename,header.filelen);
#endif
}

void getFileInner(int sockfd, char *, char *, int);
void getParent(char *parent, const char *fullpath);
//void processPathName(char *);//declare
void getFile(int conn_fd, MysqlEncap *sql_conn) {
#ifdef _DEBUG_
	printf("server get file start in files.h:\n");
#endif

	while(1){
		/*get file name*/
		int ret;
		FileHeader header;
		ret = recv(conn_fd, &header, sizeof(header), 0);
		if(-1 == ret){
			perror("recv fileinfo");
		} else if(0 == ret) {
			printf("client quit!\n");
			return;
		}
#ifdef _DEBUG_
		if (header.isDir == 1) {
			printf("------the header is dir-----\n");
		}
#endif
		if(header.isEnd == 1)//end of transfer
			break;	
			
		
		//it is a dir
		if(header.isDir == 1) {
#ifdef _DEBUG_
			printf("++++++DIR+++++++++\n");
#endif
			char sql[512];
			memset(sql, 0, sizeof(sql));

			//if the dir doesn't exist in the server, then mkdir
			if(access(header.filename, F_OK) != 0) {
				if(mkdir(header.filename, 0755) < 0){
				fprintf(stderr, "make dir [%s] error\n", header.filename);
				}
				//find parent id
				char parent[256];
				getParent(parent, header.filename);//get parent path name
				char *pItemId;
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "select itemId from testdb.item where path = '%s';", parent);

				sql_conn->ExecuteQuery(sql);
				if(sql_conn->FetchRow()) {
					pItemId = sql_conn->GetField("itemId");
#ifdef _DEBUG_
					printf("-----------pItemId:%s\n", pItemId);
#endif

				} else {
					//this condition can not happen
#ifdef _DEBUG_
					printf("just for test empty of FetchRow\n");
#endif
					pItemId = '\0';
				}

				//insert the dir item into the item table of the db
				//sql_conn->StartTransaction();//开启一个事务
#ifdef _DEBUG_
				printf("----temp test----%s-\n",parent);
#endif

				//插入到item表中
				sprintf(sql, "insert into testdb.item values(NULL, '%s', 1, '0', '%s', 0);", pItemId, header.filename);
				sql_conn->Execute(sql);
				
				//can be merge into the above sql expression
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "select itemId from testdb.item where path = '%s';", header.filename);
				sql_conn->ExecuteQuery(sql);
				if(!sql_conn->FetchRow()) {
					perror("[select after insert], but the item is not exist!\n");
				}
		
				//the dir is not exist, so the comment should be [create folder [folder name]]
				memset(header.comment, 0, sizeof(header.comment));
				sprintf(header.comment, "create folder [%s]", header.filename);
				//插入到history表中
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "insert into testdb.history values(NULL, %s, '%s', 1, 1, '0', '%s', '%s', 0, 'simon', 1, 0);",\
						 sql_conn->GetField("itemId"), pItemId, header.comment, header.filename);		 
				sql_conn->Execute(sql);
		
				//sql_conn->EndTransaction();

			}

		} else { //文件
#ifdef _DEBUG_
			printf("++++++File+++++++++\n");
#endif
			char sql[512];

			int neeGetFile = 0;//是否改变
			char* itemId;
			char* pItemId;
			int version;

			memset(sql, 0, sizeof(sql));
			sprintf(sql, "select itemId, pItemId, sha1, version, m_time from testdb.item where path = '%s';", header.filename);
			sql_conn->ExecuteQuery(sql);
			if(sql_conn->FetchRow()) {  /*file 存在*/
	
				version = atoi(sql_conn->GetField("version"));
		
				itemId = sql_conn->GetField("itemId");
				pItemId = sql_conn->GetField("pItemId");
				//printf("item id:%s\n", sql_conn->GetField("itemId"));	
				//比较修改时间
				char modify_time[20];
				sprintf(modify_time, "%ld", header.m_time);
				if (strcmp(modify_time,sql_conn->GetField("m_time"))==0) {
					//修改时间都是相同的，所以根本不用发送文件。。。
					//。。。。
#ifdef _DEBUG_
					printf("here1\n");
#endif
					ret = send(conn_fd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需不要传送文件
					if (ret == -1) {
						perror("error when send neeGetFile\n");
					}
			
				} else if (strcmp(header.sha1, sql_conn->GetField("sha1"))==0) {
					//修改时间不一样，但是内容没用改变，也不用发送文件
					//。。。。
#ifdef _DEBUG_
					printf("here2\n");
#endif
					ret = send(conn_fd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需不要传送文件
					if (ret == -1) {
						perror("error when send neeGetFile\n");
					}
			
				} else {
					//要发送文件，同时修改表中的数据（版本加一，修改时间，等）
					//。。。。
#ifdef _DEBUG_
					printf("here3\n");
#endif
			
					neeGetFile = 1;
			
					/*
					*为了保证多线程下，线程得到版本号就能正确的拿到文件，所以在文件传输成功后在修改版本号
					*这样在文件传送过程中，别的线程只能拿到未更行的版本，这样就不出错也不用加锁 
					*另外，由于文件传送可能会花费比较长的时间，所以传送前先把数据库连接返回连接池，完成后在取连接
					*/
					//conn_pool->ReleaseOneConn(sql_conn);//归还数据库连接
			
					ret = send(conn_fd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需要传送文件
					if (ret == -1) {
						perror("error when send neeGetFile\n");
					}
					//应该是先传送文件之后在修改表信息，所以表信息要最后修改
					if(neeGetFile) {
#ifdef _DEBUG_
						printf("begin get file\n");
#endif
						getFileInner(conn_fd, header.filename, header.filelen, version+1);
#ifdef _DEBUG_
						printf("end get file\n");
#endif
					}
					//sql_conn = conn_pool->GetOneConn();//重新获得一个数据库连接
			
					//sql_conn->StartTransaction();//开启一个事务
			
					memset(sql, 0, sizeof(sql));
					sprintf(sql, "update testdb.item set version = %d, sha1 = '%s', m_time = %ld where itemId = %s;", \
							version+1, header.sha1, header.m_time, itemId/*sql_conn->GetField("itemId")*/);
					sql_conn->Execute(sql);
			
					//修改上一版history，把islatest改为0
					memset(sql, 0, sizeof(sql));
					sprintf(sql, "update testdb.history set isLatest = 0 where itemId = %s and version = %d;", \
								/*sql_conn->GetField("itemId")*/itemId, version);
					sql_conn->Execute(sql);
			
			
					//插入到history表中
					memset(sql, 0, sizeof(sql));
					sprintf(sql, "insert into testdb.history values(NULL, %s, %s, %d, 1, '%s', '%s', '%s', %ld, 'simon', 1, 0);",\
							 /*sql_conn->GetField("itemId")*/itemId, pItemId, version+1, header.sha1, header.comment, header.filename, header.m_time);		 
					sql_conn->Execute(sql);
			
					//sql_conn->EndTransaction();
			
				}
			} else {//file doesn't exist, 新添加的文件
#ifdef _DEBUG_
				printf("file does not exist\n");
#endif

				//find the parent id of the file
				char parent[256];
				getParent(parent, header.filename);//get parent path name
				char *pItemId;
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "select itemId from testdb.item where path = '%s';", parent);

				sql_conn->ExecuteQuery(sql);
				if(sql_conn->FetchRow()) {
					pItemId = sql_conn->GetField("itemId");
#ifdef _DEBUG_
					printf("-----------pItemId:%s\n", pItemId);
#endif
					
				} else {
					pItemId = '\0';
				}


				neeGetFile = 1;
		
				//conn_pool->ReleaseOneConn(sql_conn);//归还数据库连接
			
				ret = send(conn_fd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需要传送文件
				if (ret == -1) {
					perror("error when send neeGetFile\n");
				}
				//应该是先传送文件之后在修改表信息，所以表信息要最后修改
				if(neeGetFile) {
#ifdef _DEBUG_
					printf("begin get file\n");
#endif
					getFileInner(conn_fd, header.filename, header.filelen, 1);
#ifdef _DEBUG_
					printf("end get file\n");
#endif
				}
				//sql_conn = conn_pool->GetOneConn();//重新获得一个数据库连接
		
				//是否需要transaction
				//sql_conn->StartTransaction();//开启一个事务
		
				memset(sql, 0, sizeof(sql));
				//插入到item表中
				sprintf(sql, "insert into testdb.item values(NULL, '%s', 1, '%s', '%s', %ld);", pItemId, header.sha1, header.filename, header.m_time);
				sql_conn->Execute(sql);
		
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "select itemId from testdb.item where path = '%s';", header.filename);
				sql_conn->ExecuteQuery(sql);
				if(!sql_conn->FetchRow()) {
					perror("[select after insert], but the item is not exist!\n");
				}
		
				//插入到history表中
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "insert into testdb.history values(NULL, %s, '%s', 1, 1, '%s', '%s', '%s', %ld, 'simon', 1, 0);",\
						 sql_conn->GetField("itemId"), pItemId, header.sha1, header.comment, header.filename, header.m_time);		 
				sql_conn->Execute(sql);
		
				//sql_conn->EndTransaction();
			}
			//update the version of the parent
			if (neeGetFile) { /*file changed*/
				char pathtmp[256];
				char parent[256];
				strcpy(pathtmp, header.filename);

				/*if increase parent's version, the comment should indicate the file itself
				* e.g.
				* fix a bug[files.h]
				*/
				sprintf(header.comment, "%s[%s]", header.comment, header.filename);

				while(1) {
					getParent(parent, pathtmp);//get parent path name
					//no parent
					if(strcmp(parent, "") == 0)
						break;

					char *pItemId;
					int versionTmp;
					char *path;
					char *itemId;

					memset(sql, 0, sizeof(sql));
					sprintf(sql, "select itemId, pItemId, version, path from testdb.item where path = '%s';", parent);

					sql_conn->ExecuteQuery(sql);
					if(sql_conn->FetchRow()) {
						itemId = sql_conn->GetField("itemId");
						pItemId = sql_conn->GetField("pItemId");
						versionTmp = atoi(sql_conn->GetField("version"));
						path = sql_conn->GetField("path");
					} else {
						break;
					}

					memset(sql, 0, sizeof(sql));
					sprintf(sql, "update testdb.item set version = %d where itemId = %s;", \
							versionTmp+1, itemId);
					sql_conn->Execute(sql);
			
					//修改上一版history，把islatest改为0
					memset(sql, 0, sizeof(sql));
					sprintf(sql, "update testdb.history set isLatest = 0 where itemId = %s and version = %d;", \
								itemId, versionTmp);
					sql_conn->Execute(sql);
			
			
					//插入到history表中
					memset(sql, 0, sizeof(sql));
					
					sprintf(sql, "insert into testdb.history values(NULL, %s, %s, %d, 1, '0', '%s', '%s', 0, 'simon', 1, 0);",\
							 itemId, pItemId, versionTmp+1, header.comment, path);		 
					sql_conn->Execute(sql);
					//forget this can be a disaster
					strcpy(pathtmp, parent);
				}
			}

			
			//conn_pool->ReleaseOneConn(sql_conn);
		}
	}
}

void getFileInner(int conn_fd, char *path, char *len, int version) {
#ifdef _DEBUG_
	printf("------->%s<---\n",path);
#endif
	char cwd[512];
	getcwd(cwd,512);
#ifdef _DEBUG_
	printf("cwd:%s\n",cwd);
#endif

	/*
	* the server stores the files in the format:[filename::version]
	* for example, readMe of version 2 will be stored by the name readMe::2 
	*/
	char newname[256];
	sprintf(newname, "%s::%d", path, version);

	/*if file exists, override or cancel*/
	int flag = 1;//write or not
	/*file exist*/
	if(access(newname,F_OK) == 0){
		//fprintf(stderr,"current dir have a file with the same name[%s]!", header.filename);
		printf("current dir have a file with the same name[%s]! continue?(yes or no)\n", newname);
		
		while(1){
			char cmd[10];
			fgets(cmd, 10, stdin);
			cmd[strlen(cmd)-1]='\0';// discard '\n' 
			printf("cmd:%s-\n",cmd);
			if(!strncmp(cmd,"no",2)){
				flag = 0;
				break;
			}else if(!strncmp(cmd, "yes", 3)){
				break;
			}else{
				 printf("invalid input! please input again[yes or no]!\n");
			}
		}
	}
	if(flag){
		FILE *fp = fopen(newname, "w");
		if(fp == NULL) {
			printf("open file [%s] error!\n", path);
		}
		int byteleft;
		byteleft = atoi(len);


		int len;
		char buf[BUFSIZE];
		while(byteleft){
#ifdef _DEBUG_
			printf("byte left:%d\n",byteleft);
#endif
			len = recv(conn_fd, buf,byteleft>=BUFSIZE?BUFSIZE-1:byteleft, 0);//代码应该在这个地方有问题
#ifdef _DEBUG_
			printf("len recv:%d\n",len);
#endif
			if(len == 0){
				printf("server quit...\n");
				break;
			}else if(len < 0){
				printf("receive error...\n");
				break;
			}
			byteleft-=len;
			fwrite(buf, sizeof(char), len, fp);
			//printf("---------------\n%s\n--------------\n",buf);
			memset(buf, 0, sizeof(buf));
		
		}
		fclose(fp);
	}
}


void SendFileInner(char *path, int conn_fd, off_t size);//declare

/*send file*/
void SendFile(char *path, int conn_fd, off_t size, time_t m_time, char *comment) {	
	
	SendFileInfo(path, conn_fd, size, m_time, 0, 0, comment);//发送文件名
	SendFileInner(path, conn_fd, size);
	/*
	int needSendFile;
	printf("get needSendFile info\n");
	int ret = recv(conn_fd, &needSendFile, sizeof(needSendFile), 0);
	if(ret == -1){
 		perror("recv needSendFile info\n");
	}
	printf("end get needSendFile info\n");
	if (!needSendFile)
		return;
	
#ifdef USE_SENDFILE_
	//struct stat s;
	int filefd = open(path, O_RDONLY);
	sendfile(conn_fd, filefd, NULL, size);
	close(filefd);
#else
	FILE *fp = fopen(path, "r");
	if(fp == NULL){
		perror("open file");
	}
	int file_size;
	int len;
	char buf[BUFSIZE];
	while((file_size = fread(buf, sizeof(char), BUFSIZE, fp)) > 0){
		len = send(conn_fd, buf, file_size, 0);
		memset(buf, 0, sizeof(buf));
		if(len < 0)	{
			perror("send");
		}
		else if(len == 0)
		{
			perror("client quit the connection");
		}
	}
	fclose(fp);
#endif
	*/
}

/*send file*/
void SendFileInner(char *path, int conn_fd, off_t size) {	
	int needSendFile;
#ifdef _DEBUG_
	printf("get needSendFile info\n");
#endif
	int ret = recv(conn_fd, &needSendFile, sizeof(needSendFile), 0);
	if(ret == -1){
 		perror("recv needSendFile info\n");
	} else if(0 == ret) {
		printf("client quit!\n");
		return;
	}
#ifdef _DEBUG_
	printf("end get needSendFile info\n");
	printf("++++++path:%s+++len:%d+\n", path, (int)size);
#endif
	if (!needSendFile)
		return;
#ifdef _DEBUG_
	printf("need send file [%s]\n", path);
#endif

#ifdef USE_SENDFILE_
	//struct stat s;
	//printf("here1---send--file\n");
	int filefd = open(path, O_RDONLY);
	sendfile(conn_fd, filefd, NULL, size);
	close(filefd);
#else
	//printf("here2---***send***--file\n")
	FILE *fp = fopen(path, "r");
	if(fp == NULL){
		perror("open file");
	}
	int file_size;
	int len;
	char buf[BUFSIZE];
	while((file_size = fread(buf, sizeof(char), BUFSIZE, fp)) > 0){
		len = send(conn_fd, buf, file_size, 0);
		memset(buf, 0, sizeof(buf));
		if(len < 0)	{
			perror("send");
		}
		else if(len == 0)
		{
			perror("client quit the connection");
		}
	}
	fclose(fp);
#endif
}
/*send dir*/
void SendDir(char *path, int conn_fd, char *comment) {
	//首先判断path是否是'/'结尾
	//	if (path[strlen(path)-1]!='/') {
	//		path[strlen(path)] = '/';
	//		path[strlen(path)] = '\0';
	//	}
#ifdef _DEBUG_
	printf("in send dir :%s\n", path);
#endif
	SendFileInfo(path, conn_fd, 0, 0, 1, 0, comment);
#ifdef _DEBUG_
	printf("----send dir----\n");
#endif
	
	struct dirent *temp_path;
	struct stat s;
	DIR *dir;
	char newpath[512];

	dir = opendir(path);
	if(dir == NULL){
		perror("opendir error");
		return;
	}
	while((temp_path = readdir(dir))!=NULL){
		
		if(!strcmp(temp_path->d_name,".")||!strcmp(temp_path->d_name,".."))
			continue;
		sprintf(newpath, "%s/%s", path, temp_path->d_name);
		
		lstat(newpath, &s);
		if(S_ISDIR(s.st_mode)){
			SendDir(newpath, conn_fd, comment);
		}else if(S_ISREG(s.st_mode)){
			SendFile(newpath, conn_fd, s.st_size, s.st_mtime, comment);
		}
	}
	closedir(dir);
}
/*send file or dir*/
void Send(char *path, int conn_fd, char *comment){
	struct stat s;
	lstat(path, &s);
	if(S_ISREG(s.st_mode)){
		SendFile(path, conn_fd, s.st_size, s.st_mtime, comment);
	}else if(S_ISDIR(s.st_mode)){
		SendDir(path, conn_fd, comment);
	}
	SendFileInfo(path, conn_fd, 0, 0, 0, 1, NULL);//1 for end of file transfer
}

/*
* 从文件路径中解析出它的父目录(chengshuguang/a.txt--->chengshuguang)
*/
void getParent(char *parent, const char *fullpath) {
	const char *tail;
	int tmp;
	
	tmp = strlen(fullpath);
	tail = fullpath + tmp - 1;
	while (*tail == '/') /*去掉结尾所有的‘/’*/
    	tail--;
    while (tail>=fullpath && *tail!='/') /*找到倒数第一个slash*/
        tail--;
	while (*tail == '/') /*去掉结尾所有的‘/’*/
		tail--;
    strncpy(parent, fullpath, tail-fullpath+1);
    parent[tail-fullpath+1] = '\0';
}

/*
* parse the fullpath name(/chengshuguang/a.txt), get the filename(a.txt) and the parent dir(/chenshuguang/).
*/

int getFileName(char* filename,char *parent, const char* fullpath, int maxlen) {
  char strtmp[128], *head;
  //char strtmp2[128], *head2;
  const char *tail;
  int tmp;
  tmp = strlen(fullpath);
  tail = fullpath + tmp - 1;
  while (*tail == '/') /*去掉结尾所有的‘/’*/
    tail--;
  head = strtmp;
 
  while (tail>=fullpath && *tail!='/') /*得到filename的倒序*/
    *head++ = *tail--;
  strncpy(parent, fullpath, tail-fullpath+1);
  parent[tail-fullpath+1] = '\0';
  head--;
  
  while (maxlen>0 && head>=strtmp) {
    *filename++ = *head--;
    maxlen--;
  }
  if (maxlen>0) {
    *filename = '\0';
    return 0; //
  }
  else return -1; /*文件名太长*/
}


/*
*1. make dir if the dir does not exit
*2. create the file if does not exit
*/
void processPathName(char *fullpath){
	char *temp;
	temp = fullpath;
	int dircount = 0;
	while(1){
		char dir[128];
		while((*temp == '/'||*temp ==' ') && *temp != '\n')
			temp++;
		char *slash = strchr(temp,'/');
		if(slash == NULL)
			break;
		strncpy(dir,temp,slash-temp);
		dir[slash-temp]='\0';
		if(access(dir,F_OK) != 0){
			if(mkdir(dir, 0755) < 0){
				fprintf(stderr, "make dir [%s] error\n", dir);
			}
		} else { /*dir exist*/
			break;
		}	
		chdir(dir);
		temp = slash +1;
		dircount++;
		//在这个地方处理文件名怎么样？？，感觉不错
		//
		//
		//
		//
		//Todo:
	}
	while(dircount--){
		chdir("..");//返回原来的目录
	}
}



/*
* ************************************************************************
* 					     client get cmd
* ************************************************************************
*/
//declare
void clientGetFileCore(char *path, int conn_fd, char *filelen, char *m_time);
void clientGetTempFile(int filefd, int conn_fd, char *filelen);
void clientGetFileInner(int sockfd);

int clientGetFile(int sockfd, char *path, char *versionNum, int isRecursive, int byLatestVersion) {
	GetHeader getheader;
	strncpy(getheader.filename, path, strlen(path)+1);
	getheader.byLatestVersion = byLatestVersion;
	//getheader.versionNum = versionNum;
	strncpy(getheader.versionNum, versionNum, strlen(versionNum)+1);
	//getheader.isDir = isItFolder(path)? 1 : 0;
	getheader.isRecursive = isRecursive;

	int ret;
	ret = send(sockfd, &getheader, sizeof(getheader), 0);
	if(-1 == ret){
		perror("client get in files.h");
	}
	clientGetFileInner(sockfd);
	return 1;
}

void clientGetFileInner(int sockfd) {
#ifdef _DEBUG_
	printf("client get file start[files.h]:\n");
#endif
	int fileExistInServer = 0;
	while(1){
		/*get file info*/
		int ret;
		FileInfo fileinfo;
		ret = recv(sockfd, &fileinfo, sizeof(FileInfo), 0);
		if(-1 == ret){
			perror("recv fileinfo");
		} else if(0 == ret) {
			printf("server quit!\n");
			return;
		}
		if(fileinfo.isEnd == 1)//end of transfer
			break;
		fileExistInServer = 1; /*if not just send the end tag, indicate the file exist*/
#ifdef _DEBUG_
		printf("client-----%s----isDir:%d\n", fileinfo.filename, fileinfo.isDir);
#endif
		printf("start  getting '%s' >>>\n", fileinfo.filename);
		if(fileinfo.isDir == 1) {
			//if the dir doesn't exist in the client, then mkdir
			//access() exist return 0, no exist return -1
			if(access(fileinfo.filename, F_OK) != 0) {
				if(mkdir(fileinfo.filename, 0755) < 0)	{
					fprintf(stderr, "make dir [%s] error\n", fileinfo.filename);
				}
			}
		} else {	/*file*/

			if(access(fileinfo.filename, F_OK) != 0) { /*file not exist*/
				int neeGetFile = 1;
				ret = send(sockfd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需要传送文件
				if (ret == -1) {
					perror("error when send neeGetFile\n");
				}
				/*
				* begin getting file
				*/
				clientGetFileCore(fileinfo.filename, sockfd, fileinfo.filelen, fileinfo.m_time);


			} else {/*file exist*/
				struct stat s;
				lstat(fileinfo.filename, &s);

				int neeGetFile = 1;

				//first, compare the m_time, 
				//if same, don't need to get file from server
				//else, inform the client to select [discard, replace, merge]
				//if discard, no need to get file
				//else, need get file,
				time_t file_mtime = atoi(fileinfo.m_time);
				if(file_mtime == s.st_mtime) {
					neeGetFile = 0;
					ret = send(sockfd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需不要传送文件
					if (ret == -1) {
						perror("error when send neeGetFile\n");
					}
					//printf("Your working folder has an exactly same item!\n");
				} else {
					ShaBuffer sha;
					sha_file(fileinfo.filename, sha);
					if(strcmp(fileinfo.sha1, (char *)sha) == 0) {/*sha1 is same*/
						neeGetFile = 0;
						ret = send(sockfd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需不要传送文件
						if (ret == -1) {
							perror("error when send neeGetFile\n");
						}
						//printf("Your working folder has an exactly same item!\n");
					} else {
						printf("+------------------------------------------------------+\n");
						printf("|Getting file: %s\n", fileinfo.filename);
						printf("+------------------------------------------------------+\n");
						printf("|Warning:your working folder has a file with same name!|\n");
						printf("|      0: discard the file in the server               |\n");
						printf("|      1: replace the local one                        |\n");
						printf("|      2: merge the local one and the one from server  |\n");
						printf("+------------------------------------------------------+\n");
						printf("your choice: ");
						int choice = 0;
						scanf("%d", &choice);
						getchar();
						if(choice == 0) {
							neeGetFile = 0;
							ret = send(sockfd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需不要传送文件
							if (ret == -1) {
								perror("error when send neeGetFile\n");
							}
						} else {
							ret = send(sockfd, &neeGetFile, sizeof(neeGetFile), 0);//告诉客户端需不要传送文件
							if (ret == -1) {
								perror("error when send neeGetFile\n");
							}

							if(choice == 1) {
								//replace
								clientGetFileCore(fileinfo.filename, sockfd, fileinfo.filelen, fileinfo.m_time);

							} else if(choice == 2) {
								char tempServerFileName[32];
								strcpy(tempServerFileName, "/tmp/file_in_server");
								clientGetFileCore(tempServerFileName, sockfd, fileinfo.filelen, fileinfo.m_time);

								char diff_cmd[256];
								sprintf(diff_cmd, "%s %s %s", DIFF_AND_MERGE_TOOL_PATH, tempServerFileName, fileinfo.filename);
#ifdef _DEBUG_
								printf("%s\n", diff_cmd);
#endif
								system(diff_cmd);
								remove(tempServerFileName);
							}
						}
					} 
				

				}
				
			}

		}
		printf("finish getting '%s' <<<\n", fileinfo.filename);

	}
	if(!fileExistInServer) {
		printf("sorry, the item doesn't exist in server!\n");
	}

}
void clientGetTempFile(int filefd, int conn_fd, char *filelen){
	int byteleft;
	byteleft = atoi(filelen);

	int len;
	char buf[BUFSIZE];
	while(byteleft) {
#ifdef _DEBUG_
		printf("byte left:%d\n",byteleft);
#endif
		len = recv(conn_fd, buf,byteleft>=BUFSIZE?BUFSIZE-1:byteleft, 0);//代码应该在这个地方有问题
#ifdef _DEBUG_
		printf("len recv:%d\n",len);
#endif
		if(len == 0) {
			printf("server quit...\n");
			break;
		}else if(len < 0) {
			printf("receive error...\n");
			break;
		}
		byteleft-=len;
		write(filefd, buf, sizeof(buf));
		memset(buf, 0, sizeof(buf));
	}
}

void clientGetFileCore(char *path, int conn_fd, char *filelen, char *m_time) {
	/*
	*1. make dir if the dir does not exit
	*2. create the file if does not exit
	*/
	processPathName(path);//

	FILE *fp = fopen(path, "w+");
	if(fp == NULL) {
		printf("open file [%s] error!\n", path);
	}
	int byteleft;
	byteleft = atoi(filelen);


	int len;
	char buf[BUFSIZE];
	while(byteleft) {
#ifdef _DEBUG_
		printf("byte left:%d\n",byteleft);
#endif
		len = recv(conn_fd, buf,byteleft>=BUFSIZE?BUFSIZE-1:byteleft, 0);//代码应该在这个地方有问题
#ifdef _DEBUG_
		printf("len recv:%d\n",len);
#endif
		if(len == 0) {
			printf("server quit...\n");
			break;
		}else if(len < 0) {
			printf("receive error...\n");
			break;
		}
		byteleft-=len;
		fwrite(buf, sizeof(char), len, fp);
		memset(buf, 0, sizeof(buf));
	
	}
	
	fclose(fp);

	/*
	* modify the file's m_time
	* attention: this must be after fclose(fp), otherwise, m_time will be now 
	*/
	struct utimbuf timebuf;
	timebuf.modtime = atoi(m_time);
	timebuf.actime = atoi(m_time);
	//printf("---------m_time:%s--%d\n", m_time, atoi(m_time));
	utime(path, &timebuf);

}


// front declare
void pushNewPathIntoList(FilePathList *filelist, char *path, char *version, char *sha1, char *m_time, int isDir);
void getFolderFileListByLatestVersion(MysqlEncap *sql_conn, FilePathList *filelist, char *path, int isRecursive);
void getFolderFileListByHistory(MysqlEncap *sql_conn, FilePathList *filelist, char *path, char *versionNum,\
						 	 int isRecursive);
void getItemByHistoryRecursively(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId, const char *historyId, int isRecursive);

void serverSendFileInner(int sockfd, FilePathList *filelist);//declare
int serverSendFile(int sockfd, MysqlEncap *sql_conn) {
	GetHeader getheader;
	char sql[512];
	//memset(sql, 0, sizeof(sql));

	int ret;
	ret = recv(sockfd, &getheader, sizeof(getheader), 0);
	if(-1 == ret){
			perror("recv getheader");
	} else if(0 == ret) {
		printf("client quit!\n");
		return -1;
	}
	//filelist is a linked list to record all the file's path::version
	FilePathList *filelist = (FilePathList *)malloc(sizeof(FilePathList));
	init(filelist);

	//TO DO
	//then, if it is file, just use the path::version to get the file directly.
	//
	//otherwise,  it is a folder, 
	//if is by latest version, the server just search the item table to get
	//the path::version array
	//else by history version, search the history table and inner join with item
	// table to get the path::version array

	//then, send the file back to client
	//if client have a item with same name

	/*
	* by checking whether sha1 is "0", we can decide whether the item is file or not
	*/
	int isFile = 1;
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select sha1 from testdb.item where path = '%s';", getheader.filename);
	sql_conn->ExecuteQuery(sql);
	if(sql_conn->FetchRow()) {
		if(strcmp(sql_conn->GetField("sha1"), "0") == 0) {
			isFile = 0;
		}
	}

	if(isFile/*isItFile(getheader.filename)*/) { /*get file*/
		if(getheader.byLatestVersion) {
			//search the item table to get the version num
			//char *version;
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "select version, sha1, m_time from testdb.item where path = '%s';", getheader.filename);
			sql_conn->ExecuteQuery(sql);
			if(sql_conn->FetchRow()) {
				pushNewPathIntoList(filelist, getheader.filename, sql_conn->GetField("version"),\
						 sql_conn->GetField("sha1"), sql_conn->GetField("m_time"), 0);
				//version = sql_conn->GetField("version");
			}
			
		} else {
			//according to the path::version, get the file directly
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "select sha1, m_time from testdb.history where path = '%s' and version = %s;", \
								getheader.filename, getheader.versionNum);
			sql_conn->ExecuteQuery(sql);
			if(sql_conn->FetchRow()) {
				pushNewPathIntoList(filelist, getheader.filename, getheader.versionNum,\
						 sql_conn->GetField("sha1"), sql_conn->GetField("m_time"), 0);
				//version = sql_conn->GetField("version");
			}

			//pushNewPathIntoList(filelist, getheader.filename, getheader.versionNum, 0);
		}
	} else {      /*get folder*/
		if(getheader.byLatestVersion) {
			//it's a folder, so we just need to search the item table
			getFolderFileListByLatestVersion(sql_conn, filelist, getheader.filename, getheader.isRecursive);

		} else {
			//search the history table and be attention to the historyId Limit
			getFolderFileListByHistory(sql_conn, filelist, getheader.filename, getheader.versionNum, getheader.isRecursive);
		}
	}
	//for debug
#ifdef _DEBUG_
	unit_test(filelist);
#endif
	serverSendFileInner(sockfd, filelist);
	//now, we get the filelist to send


	return true;
}

//used in get cmd, in server
void serverSendFileInner(int sockfd, FilePathList *filelist) {
	// if(filelist->size == 0)
	// 	return;
	FilePathListNode *tmp = filelist->head;
	int loop = filelist->size;
	while(loop--) {
		FileInfo fileinfo; //= (FileInfo *)malloc(sizeof(FileInfo));
		
		if(tmp->isDir) {
			strcpy(fileinfo.filename, tmp->filename);
			strcpy(fileinfo.sha1, "0");
			strcpy(fileinfo.m_time, "0");
			fileinfo.isDir = tmp->isDir;
			fileinfo.isEnd = 0;
			sprintf(fileinfo.filelen, "%d", 0);
		} else {
			//strstr()
			char *t = strrchr(tmp->filename, ':');//first :: get last ':'
			strncpy(fileinfo.filename, tmp->filename, t-(tmp->filename)-1);
			fileinfo.filename[t-(tmp->filename)-1] = '\0';
			strcpy(fileinfo.sha1, tmp->sha1);
			strcpy(fileinfo.m_time, tmp->m_time);
			fileinfo.isDir = tmp->isDir;
			fileinfo.isEnd = 0;


			struct stat s;
			lstat(tmp->filename, &s);
			sprintf(fileinfo.filelen, "%d", (int)s.st_size);
		}

		int ret;
		ret = send(sockfd, &fileinfo, sizeof(fileinfo), 0);
		if(-1 == ret){
			perror("send fileinfo");
		}
#ifdef _DEBUG_
		printf("send file info, isDir:%d\n", fileinfo.isDir);
#endif
		if(!tmp->isDir) {
			SendFileInner(tmp->filename, sockfd, atoi(fileinfo.filelen));
		}

		tmp = tmp->next;
	}
#ifdef _DEBUG_
	printf("-----------------------end package is going to send--------------------\n");
#endif
	//send end transfer tag
	FileInfo endinfo;
	endinfo.isEnd = 1;
	strcpy(endinfo.filename, "0");
	strcpy(endinfo.sha1, "0");
	strcpy(endinfo.m_time, "0");
	endinfo.isDir = 0;
	sprintf(endinfo.filelen, "%d", 0);
	int ret;
	ret = send(sockfd, &endinfo, sizeof(endinfo), 0);
	if(-1 == ret){
		perror("send fileinfo");
	}


}



/*
* aim: if isFile, push path::version to the file list
*	   else if idDir, push path to the file list
*/
void pushNewPathIntoList(FilePathList *filelist, char *path, char *version, \
				char *sha1, char *m_time, int isDir) {
	FilePathListNode * newpath = (FilePathListNode *)malloc(sizeof(FilePathListNode));
	if(!isDir) {
		sprintf(newpath->filename, "%s::%s", path, version);
		sprintf(newpath->sha1, "%s", sha1);
		sprintf(newpath->m_time, "%s", m_time);
		//newpath->m_time = m_time;
		newpath->isDir = isDir;
		newpath->next = NULL;
		push_back(filelist, newpath);
	} else {
		sprintf(newpath->filename, "%s", path);
		newpath->next = NULL;
		newpath->isDir = isDir;
		strcpy(newpath->sha1, "");
		strcpy(newpath->m_time, "");
		//newpath->sha1 = NULL;
		//newpath->m_time = NULL;
		push_back(filelist, newpath);
	}
	
}



//by latest version
void getSubFiles(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId) {
	char sql[512];
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select path, version, sha1, m_time from testdb.item where pItemId = %s and sha1 != '0';", pItemId);
	sql_conn->ExecuteQuery(sql);
	while(sql_conn->FetchRow()) {
		pushNewPathIntoList(filelist, sql_conn->GetField("path"),\
			sql_conn->GetField("version"), sql_conn->GetField("sha1"), sql_conn->GetField("m_time"), 0);
	}
}
/*
// //by latest version
// void getSubFolders(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId) {
// 	char sql[512];
	
// 	memset(sql, 0, sizeof(sql));
// 	sprintf(sql, "select path from testdb.item where pItemId = %s and sha1 = '0';", pItemId);
// 	sql_conn->ExecuteQuery(sql);
// 	while(sql_conn->FetchRow()) {
// 		pushNewPathIntoList(filelist, sql_conn->GetField("path"),\
// 			NULL, 1);
// 	}
// }
*/
//by latest version
void getItemRecursively(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId, int isRecursive) {
	char pItemIdTmp[32];
	strcpy(pItemIdTmp, pItemId);
	/*get sub files*/
	getSubFiles(sql_conn, filelist, pItemIdTmp);

	/*get sub folders and recursively call this func*/
	if(isRecursive) {
		char sql[512];
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select itemId, path from testdb.item where pItemId = %s and sha1 = '0';", pItemIdTmp);
		sql_conn->ExecuteQuery(sql);

		/*
		* strLinkedList is a C-style string linked list, with variable length
		*/
		strLinkedList *strlist = (strLinkedList *)malloc(sizeof(strLinkedList));
		initStr(strlist);

		int isEmpty = 1;
		while(sql_conn->FetchRow()) {
			isEmpty = false;
			pushNewPathIntoList(filelist, sql_conn->GetField("path"),\
				NULL, NULL, 0, 1);
			strNode *strnode = (strNode *)malloc(sizeof(strNode));
			strcpy(strnode->str, sql_conn->GetField("itemId"));
			push_back_str(strlist, strnode);
			//getItemRecursively(sql_conn, filelist, sql_conn->GetField("itemId"), isRecursive);
		}
		if(isEmpty)
			return;//condition to break the recursion


		strNode *tmp = strlist->head;
		while(tmp != strlist->tail) {
#ifdef _DEBUG_
			printf("--------folder itemId:%s\n", tmp->str);
#endif
			getItemRecursively(sql_conn, filelist, tmp->str, isRecursive);
			tmp = tmp->next;
		}
		//tail
		getItemRecursively(sql_conn, filelist, tmp->str, isRecursive);
#ifdef _DEBUG_
		printf("--------folder itemId:%s\n", tmp->str);
#endif

	}
}
//by latest version
void getFolderFileListByLatestVersion(MysqlEncap *sql_conn, FilePathList *filelist, char *path, int isRecursive) {
	char sql[512];
	//first, get itemId(as pitemid below)
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select itemId from testdb.item where path = '%s';", path);
	sql_conn->ExecuteQuery(sql);

	char *itemId;

	if(sql_conn->FetchRow()) {
		itemId = sql_conn->GetField("itemId");
	} else {
		return;
	}
	//push the parent folder into the file list
	pushNewPathIntoList(filelist, path, NULL, NULL, 0, 1);
	getItemRecursively(sql_conn, filelist, itemId, isRecursive);
}



//by history version
void getSubFiles(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId, const char *historyId) {
	char sql[512];
	
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select path, max(version)version, sha1, m_time from testdb.history where pItemId = %s and historyId < %s and \
	 				sha1 != '0' group by itemId;", pItemId, historyId);
	sql_conn->ExecuteQuery(sql);
	while(sql_conn->FetchRow()) {
		pushNewPathIntoList(filelist, sql_conn->GetField("path"),\
			sql_conn->GetField("version"), sql_conn->GetField("sha1"), sql_conn->GetField("m_time"), 0);
	}
}
/*
// //by history version
// void getSubFolders(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId, char *historyId) {
// 	char sql[512];
	
// 	memset(sql, 0, sizeof(sql));
// 	sprintf(sql, "select path, max(version)version from testdb.history where pItemId = %s and historyId < %s and \
// 	 				sha1 = '0' group by itemId;", pItemId, historyId);
// 	sql_conn->ExecuteQuery(sql);
// 	while(sql_conn->FetchRow()) {
// 		pushNewPathIntoList(filelist, sql_conn->GetField("path"),\
// 			NULL, 1);
// 	}
// }
*/
//by history verison
void getFolderFileListByHistory(MysqlEncap *sql_conn, FilePathList *filelist, char *path, char *versionNum,\
						 	 int isRecursive) {
	char sql[512];
	//first, get the history id and itemId(as pitemid below)
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select historyId, itemId from testdb.history where path = '%s' and version = %s;",\
					 path, versionNum);
	sql_conn->ExecuteQuery(sql);

	// because historyId will be used as the same value
	// and cannot be changed after getting the init value
	// it need a memory address, can not use the address in sql, which will change when next sql-search happen.
	// so, a historyId show have it's own memory
	//char *historyId;
	char historyId[32];// can be pointer... need to ...
	char itemId[32];
	//char *itemId;

	if(sql_conn->FetchRow()) {
		//historyId = sql_conn->GetField("historyId");
		strcpy(historyId, sql_conn->GetField("historyId"));
		strcpy(itemId, sql_conn->GetField("itemId"));
		//itemId = sql_conn->GetField("itemId");
	} else {
		return;
	}
	pushNewPathIntoList(filelist, path, NULL, NULL, 0, 1);
	//
	getItemByHistoryRecursively(sql_conn, filelist, itemId, historyId, isRecursive);
}
void getItemByHistoryRecursively(MysqlEncap *sql_conn, FilePathList *filelist, char *pItemId, const char *historyId, int isRecursive) {
	/*
	* it is important to copy the pItemId and historyId into a new char[]
	* when getSubFiles finish, the pItemId and historyId will change...
	*/
	char historyIdTmp[32];
	char pItemIdTmp[32];
	strcpy(historyIdTmp, historyId);
	strcpy(pItemIdTmp, pItemId);
#ifdef _DEBUG_
	printf("1.---------%s:%s------\n", pItemIdTmp, historyIdTmp);
#endif
	getSubFiles(sql_conn, filelist, pItemIdTmp, historyIdTmp);
#ifdef _DEBUG_
	printf("2.---------%s:%s------\n", pItemIdTmp, historyIdTmp);
#endif
	if(isRecursive) {
		char sql[512];
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select itemId, path, max(version)version from testdb.history where pItemId = %s and historyId < %s and \
	 				sha1 = '0' group by itemId;", pItemIdTmp, historyIdTmp);
		sql_conn->ExecuteQuery(sql);

		/*
		* strLinkedList is a C-style string linked list, with variable length
		*/
		strLinkedList *strlist = (strLinkedList *)malloc(sizeof(strLinkedList));
		initStr(strlist);

		int isEmpty = 1;
		while(sql_conn->FetchRow()) {
			isEmpty = false;
			pushNewPathIntoList(filelist, sql_conn->GetField("path"),\
				NULL, NULL, 0, 1);
			strNode *strnode = (strNode *)malloc(sizeof(strNode));
			//sprintf(strnode->str, "%s|%s", sql_conn->GetField("itemId"), sql_conn->GetField(historyId));
			sprintf(strnode->str, "%s", sql_conn->GetField("itemId"));
			//strcpy(strnode->str, sql_conn->GetField("itemId"));
			push_back_str(strlist, strnode);
			//getItemRecursively(sql_conn, filelist, sql_conn->GetField("itemId"), isRecursive);
		}
		if(isEmpty)
			return;//condition to break the recursion


		strNode *tmp = strlist->head;
		while(tmp != strlist->tail) {
			// printf("--------folder itemId:%s\n", tmp->str);
			getItemByHistoryRecursively(sql_conn, filelist, tmp->str, historyId, isRecursive);
			tmp = tmp->next;
		}
		//tail
		
		getItemByHistoryRecursively(sql_conn, filelist, tmp->str, historyId, isRecursive);
		//printf("--------folder itemId:%s\n", tmp->str);

	}
}



/*
* ************************************************************************
* 					     show history cmd
* ************************************************************************
*/
void clientGetHistory(int sockfd, char *path) {
	int ret;
	ret = send(sockfd, path, 256, 0);// important: sizeof(path) = 4
	if(-1 == ret){
		perror("client get history");
	}

	int count;
	ret = recv(sockfd, &count, sizeof(count), 0);
	if(-1 == ret) {
		perror("recv history count");
	} else if(0 == ret) {
		printf("server quit!\n");
		return;
	}
	if(count != 0) {
		printf("+-------+----------+----------------------------------------------------------+\n");
		printf("|version| user name| comment                                                  |\n");
		printf("+-------+----------+----------------------------------------------------------+\n");
	}
	int i;
	for(i=0; i<count; i++) {
		HistroyInfo historyinfo;
		ret = recv(sockfd, &historyinfo, sizeof(historyinfo), 0);
		if(-1 == ret) {
			perror("recv history info");
		} else if(0 == ret) {
			printf("server quit!\n");
			return;
		}
		printf("|%7s|%10s|%-58s|\n", historyinfo.versionNumber, historyinfo.user, historyinfo.comment);
	}
	if(count != 0)
		printf("+-------+----------+----------------------------------------------------------+\n");
}

void serverSendHistory(int sockfd, MysqlEncap *sql_conn) {
	char path[256];
	memset(path, 0, sizeof(path));
	int ret;
	ret = recv(sockfd, path, sizeof(path), 0);
	if(-1 == ret) {
		perror("server send history");
	} else if(0 == ret) {
		printf("client quit!\n");
		return;
	}

	char sql[512];
	memset(sql, 0, sizeof(sql));
	//version desc query
	sprintf(sql, "select version, user, comment from testdb.history where path = '%s' order by version desc;", path);
	sql_conn->ExecuteQuery(sql);

	int count = sql_conn->GetQueryResultCount();
	//send the result num
	ret = send(sockfd, &count, sizeof(count), 0);
	if(-1 == ret) {
		perror("send history count");
	} else if(0 == ret) {
		printf("client quit\n");
		return;
	}

	while(sql_conn->FetchRow()) {
		HistroyInfo historyinfo;
		strcpy(historyinfo.versionNumber, sql_conn->GetField("version"));
		strcpy(historyinfo.user, sql_conn->GetField("user"));
		strcpy(historyinfo.comment, sql_conn->GetField("comment"));

		ret = send(sockfd, &historyinfo, sizeof(historyinfo), 0);
		if(-1 == ret) {
			perror("send history info");
		}
	}

}


/*
* ************************************************************************
* 					     do tag cmd
* ************************************************************************
*/


void clientDoTag(int sockfd, char *realpath, char *versionNumber, char *tagName) {
	TagInfo taginfo;
	TaskResult taskresult;


	strcpy(taginfo.filename, realpath);
	strcpy(taginfo.versionNumber, versionNumber);
	strcpy(taginfo.tagName, tagName);

	int ret;
	ret = send(sockfd, &taginfo, sizeof(taginfo), 0);
	if(-1 == ret) {
		perror("send tag info");
	}

	ret = recv(sockfd, &taskresult, sizeof(taskresult), 0);
	if(-1 == ret) {
		perror("recv taskresult info");
	} else if(0 == ret) {
		printf("server quit!\n");
		return;
	}
	if(taskresult.success == 1) {
		printf("OK: ");
	} else if(taskresult.success == 0) {
		printf("ERROR: ");
	} else if(taskresult.success == 2) {
		printf("Attention:");
	}
	printf("%s\n", taskresult.description);

}

void serverDoTag(int sockfd, MysqlEncap *sql_conn) {

	TaskResult taskresult;/*return the task result to client*/

	TagInfo taginfo;
	int ret;
	ret = recv(sockfd, &taginfo, sizeof(taginfo), 0);
	if(-1 == ret) {
		perror("recv tag info");
	} else if(0 == ret) {
		printf("client quit!\n");
		return;
	}
	char sql[512];

	/*
	* tagName + path ==>> unique version
	*/
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select version from testdb.tag where tagName = '%s' and path = '%s';", taginfo.tagName, taginfo.filename);
	sql_conn->ExecuteQuery(sql);
	if(sql_conn->FetchRow()) { /*the item with this version have a tag already */
		if(strcmp(taginfo.versionNumber, sql_conn->GetField("version")) == 0) {
			taskresult.success = 2;
			sprintf(taskresult.description, "a same tag '%s' already exist on item '%s' with version %s !", taginfo.tagName, \
				taginfo.filename, taginfo.versionNumber); 
		} else {
			taskresult.success = 0;
			sprintf(taskresult.description, "tag '%s' exist on item '%s' with different version [%s] already!\n ", \
				taginfo.tagName, taginfo.filename, sql_conn->GetField("version"));
		}
	} else {
		//query the item table, get item id and latest verion[ >= taginfo.versionNumber]
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select itemId, version from testdb.item where path = '%s';", taginfo.filename);
		sql_conn->ExecuteQuery(sql);
		if(sql_conn->FetchRow()) {
			if(strcmp(taginfo.versionNumber, "latest") == 0) {
				memset(taginfo.versionNumber, 0, sizeof(taginfo.versionNumber));
				strcpy(taginfo.versionNumber, sql_conn->GetField("version"));
#ifdef _DEBUG_
				printf("version:%s-\n", taginfo.versionNumber);
#endif
			}
			if(atoi(sql_conn->GetField("version")) < atoi(taginfo.versionNumber)) {/*tag version > latest version*/
				taskresult.success = 0;
				strcpy(taskresult.description, "you try to tag a version which is larger than the latest version!");
			} else { /*insert into tag table*/
				memset(sql, 0, sizeof(sql));
				sprintf(sql, "insert into testdb.tag values(NULL, '%s', %s, '%s', %s);", taginfo.tagName, \
					sql_conn->GetField("itemId"), taginfo.filename, taginfo.versionNumber);
				sql_conn->Execute(sql);
				taskresult.success = 1;
				strcpy(taskresult.description, "do tag successfully!");
			}
		} else {
			taskresult.success = 2;
			sprintf(taskresult.description, "item '%s' doesn't exist on server!", taginfo.filename);
		}

	}

	ret = send(sockfd, &taskresult, sizeof(taskresult), 0);
	if(-1 == ret) {
		perror("send taskresult info");
	}

}

/*
* ************************************************************************
* 					     show tag cmd
* ************************************************************************
*/

void clientGetTag(int sockfd, char *path) {
	int ret;
	ret = send(sockfd, path, 256, 0);// important: sizeof(path) = 4
	if(-1 == ret){
		perror("client get tag");
	}

	int count;
	ret = recv(sockfd, &count, sizeof(count), 0);
	if(-1 == ret) {
		perror("recv tag count");
	} else if(0 == ret) {
		printf("server quit!\n");
		return;
	}
	if(count != 0) {
		printf("+--------------------+------------------------------------------------+-------+\n");
		printf("| tagName            | item name                                      |version|\n");
		printf("+--------------------+------------------------------------------------+-------+\n");
	}
	int i;
	for(i=0; i<count; i++) {
		TagInfo taginfo;
		ret = recv(sockfd, &taginfo, sizeof(taginfo), 0);
		if(-1 == ret) {
			perror("recv tag info");
		} else if(0 == ret) {
			printf("server quit!\n");
			return;
		}
		printf("|%-20s|%-48s|%-7s|\n", taginfo.tagName, taginfo.filename, taginfo.versionNumber);
	}
	if(count != 0)
		printf("+--------------------+------------------------------------------------+-------+\n");

}

void serverSendTag(int sockfd, MysqlEncap *sql_conn) {
	char path[256];
	memset(path, 0, sizeof(path));
	int ret;
	ret = recv(sockfd, path, sizeof(path), 0);
	if(-1 == ret) {
		perror("server send tag");
	} else if(0 == ret) {
		printf("client quit!\n");
		return;
	}

	char sql[512];
	memset(sql, 0, sizeof(sql));
	//version desc query
	sprintf(sql, "select tagName, path, version from testdb.tag where path = '%s' order by tagId desc;", path);
	sql_conn->ExecuteQuery(sql);

	int count = sql_conn->GetQueryResultCount();
	//send the result num
	ret = send(sockfd, &count, sizeof(count), 0);
	if(-1 == ret) {
		perror("send tag count");
	} else if(0 == ret) {
		printf("client quit\n");
		return;
	}

	while(sql_conn->FetchRow()) {
		TagInfo taginfo;
		strcpy(taginfo.versionNumber, sql_conn->GetField("version"));
		strcpy(taginfo.tagName, sql_conn->GetField("tagName"));
		strcpy(taginfo.filename, sql_conn->GetField("path"));

		ret = send(sockfd, &taginfo, sizeof(taginfo), 0);
		if(-1 == ret) {
			perror("send tag info");
		}
	}
}


/*
* get item version by label name
*/

int clientGetVersionByTagName(int sockfd, char *tagname, char *path) {
	int ret;
	ret = send(sockfd, tagname, 32, 0);
	if(-1 == ret){
		perror("client tag name");
	}
	ret = send(sockfd, path, 256, 0);// important: sizeof(path) = 4
	if(-1 == ret){
		perror("client path name");
	}

	int version;
	ret = recv(sockfd, &version, sizeof(version), 0);
	if(-1 == ret) {
		perror("recv tag version");
	} else if(0 == ret) {
		printf("server quit!\n");
		return -1;
	}
	return version;
}

void serverSendVersionByTagName(int sockfd, MysqlEncap *sql_conn) {
	char tagname[32];
	char path[256];
	memset(tagname, 0, sizeof(tagname));
	memset(path, 0, sizeof(path));
	int ret;
	ret = recv(sockfd, tagname, sizeof(tagname), 0);
	if(-1 == ret) {
		perror("server recv tag name");
	} else if(0 == ret) {
		printf("client quit!\n");
		return;
	}
	ret = recv(sockfd, path, sizeof(path), 0);
	if(-1 == ret) {
		perror("server recv path name");
	} else if(0 == ret) {
		printf("client quit!\n");
		return;
	}

	char sql[512];
	memset(sql, 0, sizeof(sql));
	//version desc query
	sprintf(sql, "select version from testdb.tag where tagName = '%s' and path = '%s';", tagname, path);
	sql_conn->ExecuteQuery(sql);
	int version = -1;
	if(sql_conn->FetchRow()) {
		version = atoi(sql_conn->GetField("version"));
	}
	
	ret = send(sockfd, &version, sizeof(version), 0);
	if(-1 == ret) {
		perror("send tag version");
	} else if(0 == ret) {
		printf("client quit\n");
		return;
	}
}


#endif
