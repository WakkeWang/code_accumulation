#include <stdio.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

static void *thread_fun(void *param)
{
	printf("thread pid:%d, pthread_self:%lu\n", getpid(),pthread_self());
	while(1);
}

int main(int argc, char *argv[])
{
	int n = atoi(argv[1]);
	pthread_t tid[n];
	int i;

	printf("main pid:%d  tid:pthread_self:%lu\n", getpid(),pthread_self());
	
	for(i = 0; i < n; i++)
		pthread_create(&tid[i], NULL, thread_fun, NULL);

	for(i = 0; i < n; i++)
		pthread_join(tid[i], NULL);

	return 0;
}
