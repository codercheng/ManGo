#include <stdio.h>
#include "lock.h"
#include <pthread.h>
#include <unistd.h>

using namespace std;

MutexLock mutex;
int cnt = 2000;

void *f(void *arg){
	int t_num = (int)arg;
	while(true){
		MutexGuard lock(mutex);
		if(cnt>0){
			usleep(1);
			printf("%d: %d\n", t_num, cnt--);
		}else{
			break;
		}
		
	}
	return NULL;
}

int main(){
	pthread_t tid;
	pthread_t tid1;
	pthread_t tid2;
	pthread_t tid3;
	int ret = pthread_create(&tid, NULL, f,(void*)1);
	if(ret == -1){
		perror("create error\n");
	}
	
	ret = pthread_create(&tid1, NULL, f, (void*)2);
	if(ret == -1){
		perror("create error\n");
	}
	
	ret = pthread_create(&tid2, NULL, f, (void*)3);
	if(ret == -1){
		perror("create error\n");
	}
	
	ret = pthread_create(&tid3, NULL, f, (void*)4);
	if(ret == -1){
		perror("create error\n");
	}
	
	pthread_join(tid, NULL);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);
	return 0;
}
