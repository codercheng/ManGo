/*
*测试连接池类ConnPool:
*g++ -g -Wall -lmysqlclient -lpthread mysql_encap.cpp mysql_encap.h conn_pool.cpp conn_pool.h testConnPool.cpp -o testConnPool
*/

#include "conn_pool.h"
#include <stdio.h>
#include "mysql_encap.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
using namespace std;

ConnPool* conn_pool = ConnPool::GetInstance();

void *run(void *arg){
	while(true){
		MysqlEncap *sql_conn = conn_pool->GetOneConn();
//		sql_conn->ExecuteQuery("select * from testdb.student;");
//	
//		while(sql_conn->FetchRow()){
//			printf("%s,%s\n",sql_conn->GetField("sid"),sql_conn->GetField("name"));
//		}
	
		conn_pool->ShowStatus();
		sleep(rand()%3);
		conn_pool->ReleaseOneConn(sql_conn);
	
		conn_pool->ShowStatus();
	}
	return NULL;
}

int main(){
	srand(time(0));
	freopen("out.txt","w",stdout);
	pthread_t tid1;
	pthread_t tid2;
	pthread_t tid3;
	pthread_t tid4;
	int ret;
	ret = pthread_create(&tid1, NULL, run, (void *)1);
	ret = pthread_create(&tid2, NULL, run, (void *)2);
	ret = pthread_create(&tid3, NULL, run, (void *)3);
	ret = pthread_create(&tid4, NULL, run, (void *)4);
	
	
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);

	return 0;
}
