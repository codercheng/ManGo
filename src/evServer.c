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
void message_cb(struct ev_loop *main_loop, ev_io* io_w, int e);

void *add(void *arg);
void *get(void *arg);
void *showhistory(void *arg);
void *dotag(void *arg) ;
void *showtag(void *arg);
void *getVersionByTag(void *arg);

int main() {

	main_loop = ev_default_loop(0);

 	create_bind_listen(&listen_fd, 9999, 5);
 	
 	int reuse = 1;
 	if(setsockopt(listen_fd,SOL_SOCKET ,SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {  
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

void accept_cb(struct ev_loop *main_loop, ev_io *io_w, int e) {
	int conn_fd;//声明 连接的fd，也就是accept之后的
 	struct sockaddr_in client_addr;
 	socklen_t len = sizeof(client_addr);
 	conn_fd = accept(io_w->fd, (struct sockaddr *)&client_addr, &len);
 	//setnonblocking(conn_fd);           /*后面传送文件，没有必要非阻塞*/

	if(conn_fd > 0){
		 printf("got connection from ip:%s, port:%d\n",inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port)); 
		 
		 ev_io *msg_watcher = (struct ev_io *)malloc(sizeof(struct ev_io));
		 ev_io_init (msg_watcher, message_cb, conn_fd, EV_READ);
		 ev_io_start(main_loop, msg_watcher);
	}
	else if(conn_fd < 0){
		printf("connection error ...\n");
	}
}


void message_cb(struct ev_loop *main_loop, ev_io* io_w, int e) {
	message msg;
	int ret = recv(io_w->fd, &msg, sizeof(msg), 0);
	if(ret == -1){
		perror("recv message error\n");
	} else if(0 == ret) {
		printf("client quit!\n");
		ev_io_stop(main_loop, io_w);//stop watch...when return
		return;
	}
	printf("cmd recv :%d\n", msg.cmd);
	ev_io_stop(main_loop, io_w);
	
	switch(msg.cmd){
		case ADD:
			{
				thread_pool_add_task(add, (void *)io_w->fd);
				break;
			}
		case GET:
			{
				thread_pool_add_task(get, (void *)io_w->fd);
				break;
			}
		case HISTORY:
			{
				thread_pool_add_task(showhistory, (void *)io_w->fd);
				break;
			}
		case TAG:
			{
				thread_pool_add_task(dotag, (void *)io_w->fd);	
				break;
			}
		case SHOW_TAG:
			{
				thread_pool_add_task(showtag, (void *)io_w->fd);	
				break;
			}
		case GET_VERSION_BY_TAG: 
			{
				thread_pool_add_task(getVersionByTag, (void *)io_w->fd);	
				break;
			}
		default:
			break;
	}
	
}
void *getVersionByTag(void *arg) {
	int sockfd = (int)arg;

	MysqlEncap *sql_conn = conn_pool->GetOneConn();

	serverSendVersionByTagName(sockfd, sql_conn);

	conn_pool->ReleaseOneConn(sql_conn);

	return NULL;
}
void *showtag(void *arg) {
	int sockfd = (int)arg;

	MysqlEncap *sql_conn = conn_pool->GetOneConn();

	serverSendTag(sockfd, sql_conn);

	conn_pool->ReleaseOneConn(sql_conn);

	return NULL;
}
void *dotag(void *arg) {
	int sockfd = (int)arg;

	MysqlEncap *sql_conn = conn_pool->GetOneConn();
	sql_conn->StartTransaction();//开启一个事务

	serverDoTag(sockfd, sql_conn);

	sql_conn->EndTransaction();
	conn_pool->ReleaseOneConn(sql_conn);
	return NULL;
}
void *showhistory(void *arg) {
	int sockfd = (int)arg;

	MysqlEncap *sql_conn = conn_pool->GetOneConn();

	serverSendHistory(sockfd, sql_conn);

	conn_pool->ReleaseOneConn(sql_conn);

	return NULL;
}
//client get, that is, server send
void *get(void *arg) {
	int sockfd = (int)arg;

	MysqlEncap *sql_conn = conn_pool->GetOneConn();
	//sql query doesn't need transaction
	//sql_conn->StartTransaction();//开启一个事务

	serverSendFile(sockfd, sql_conn);

	//sql_conn->EndTransaction();
	conn_pool->ReleaseOneConn(sql_conn);

	return NULL;
}
//client add, that is, server get
void *add(void *arg) {
	int sockfd = (int)arg;

	MysqlEncap *sql_conn = conn_pool->GetOneConn();
	sql_conn->StartTransaction();//开启一个事务

	getFile(sockfd, sql_conn);

	sql_conn->EndTransaction();
	conn_pool->ReleaseOneConn(sql_conn);
	return NULL;
}