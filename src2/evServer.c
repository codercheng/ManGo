#include <ev.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include "common.h"
#include "sock.h"
#include "conn_pool.h"
#include "mysql_encap.h"
#include "thread_pool.h"
#include "files.h"

ev_io *accept_watcher;
struct ev_loop *main_loop;
int listen_fd;//声明 服务器监听fd
//db连接池
ConnPool* conn_pool;

void setnonblocking(int sock);
void accept_cb(struct ev_loop *main_loop, ev_io * io_w, int e);
void send_cb(struct ev_loop *main_loop, ev_io * io_w, int e);
void message_cb(struct ev_loop *main_loop, ev_io* io_w, int e);

void *taskprocess(void *arg);
void *addfile(void *arg);
void *testadd(void *arg);
void *testget(void *arg);

int main() {

	main_loop = ev_default_loop(0);

 	create_bind_listen(&listen_fd, 9999, 5);
 	
 	int reuse = 1;
 	if(setsockopt(listen_fd,SOL_SOCKET ,SO_REUSEADDR,&reuse,sizeof(reuse)) != 0) {  
        printf("setsockopt error in reuseaddr[%d]\n", listen_fd);  	
    }  
 	setnonblocking(listen_fd);
 	
 	signal(SIGPIPE, SIG_IGN);
 	
 	
 	conn_pool = ConnPool::GetInstance();//bind 成功在初始化线程池
 	thread_pool_init(5);//线程池	
 	
 	accept_watcher = (struct ev_io *)malloc(sizeof(struct ev_io));
 	ev_io_init (accept_watcher, accept_cb, listen_fd, EV_READ);
 	ev_io_start(main_loop,accept_watcher);

    ev_run(main_loop,0);
    
	return 0;
}

void setnonblocking(int sock) {
   int opts;
   opts = fcntl(sock, F_GETFL);
   if(opts < 0) {
      perror("fcntl(sock, GETFL)");
      exit(1);
   }
   opts = opts | O_NONBLOCK;
   if(fcntl(sock, F_SETFL, opts) < 0) {
      perror("fcntl(sock,SETFL,opts)");
      exit(1);
   }
}

void accept_cb(struct ev_loop *main_loop,ev_io *io_w,int e) {
	int conn_fd;//声明 连接的fd，也就是accept之后的
 	struct sockaddr_in client_addr;
 	socklen_t len = sizeof(client_addr);
 	conn_fd = accept(io_w->fd, (struct sockaddr *)&client_addr, &len);
 	//setnonblocking(conn_fd);           /*后面传送文件，没有必要非阻塞*/

	if(conn_fd > 0){
		 printf("got connection from ip:%s, port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port)); 
		 //ev_io *send_watcher = (struct ev_io *)malloc(sizeof(struct ev_io));
		 //ev_io_init (send_watcher, send_cb, conn_fd, EV_WRITE);
		 //ev_io_start(main_loop, send_watcher);
		 
		 ev_io *msg_watcher = (struct ev_io *)malloc(sizeof(struct ev_io));
		 ev_io_init (msg_watcher, message_cb, conn_fd, EV_READ);
		 ev_io_start(main_loop, msg_watcher);
	}
	else if(conn_fd < 0){
		printf("connection error ...\n");
	}
}

void send_cb(struct ev_loop *main_loop, ev_io *io_w, int e) {
	printf("sock can write\n");
	char path[512];
	printf("input the path:");
	scanf("%s",path);
	path[strlen(path)]='\0';
	printf("%s-\n",path);
	//Send(path, io_w->fd);
}
void message_cb(struct ev_loop *main_loop, ev_io* io_w, int e) {
	message msg;
	int ret = recv(io_w->fd, &msg, sizeof(msg), 0);
	if(ret == -1){
		perror("recv message error\n");
	}
	printf("msg recv :%d\n", msg.cmd);
	ev_io_stop(main_loop, io_w);
	
	switch(msg.cmd){
		case GET_FILES:
			{
				thread_pool_add_task(taskprocess,(void *)io_w->fd);
				break;
			}			
		case ADD_FILES:
				printf("add files\n");
				thread_pool_add_task(addfile,(void *)io_w->fd);
				break;
		case SEND_FILES:
			{
				char path[512];
				printf("input the path:");
				scanf("%s",path);
				path[strlen(path)]='\0';
				printf("%s-\n",path);
				//Send(path, io_w->fd);
				break;
			}
		case TEST_ADD:
			{
				printf("-----------test add----------\n");
				thread_pool_add_task(testadd,(void *)io_w->fd);
				break;
			}
		case TEST_GET:
			{
				printf("----------test get------------\n");
				thread_pool_add_task(testget,(void *)io_w->fd);
			}
		default:
			break;
	}
	
}
void *testget(void *arg) {
	int sockfd = (int)arg;
	MysqlEncap *sql_conn = conn_pool->GetOneConn();
	printf("server send file start 1:\n");
	serverSendFile(sockfd, sql_conn);
	conn_pool->ReleaseOneConn(sql_conn);
	return NULL;
}
void *testadd(void *arg) {
	int sockfd = (int)arg;
	MysqlEncap *sql_conn = conn_pool->GetOneConn();
	printf("server get file start 1:\n");
	getFile(sockfd, sql_conn);
	conn_pool->ReleaseOneConn(sql_conn);
	return NULL;
}
void *addfile(void *arg) {
	return NULL;
	
}

void *taskprocess(void *arg) {
	int sockfd = (int)arg;
	MysqlEncap *sql_conn = conn_pool->GetOneConn();
//	sql_conn->ExecuteQuery("select * from testdb.student;");
//	while(sql_conn->FetchRow()){
//		printf("%s,%s\n",sql_conn->GetField("sid"),sql_conn->GetField("name"));
//	}
	sql_conn->ExecuteQuery("select path from testdb.student where sid = 11;");
	if(sql_conn->FetchRow()) {
		Send(sql_conn->GetField("path"), sockfd);		
	}
	conn_pool->ReleaseOneConn(sql_conn);
	return NULL;
}
