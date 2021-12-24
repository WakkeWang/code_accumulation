#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char *argv[])
{
	pid_t result, wait_pid;
	int status;

	result = fork();

	if(result == -1){
		perror("can not create new process");
		exit(1);
	}else if (result == 0){
			printf("child process id: %ld\n", (long)getpid());
			pause();
			exit(0);
	}else{	
			printf("parent process id:%d\n", getpid());
#if 1
			/* define 1 to make child process always a zomie */

			while(1);
#endif
			printf("parent process wait for child...\n");
			do {
				wait_pid = waitpid( result, &status, WUNTRACED | WCONTINUED);

				if (WIFEXITED(status))
						printf("child process exites, status=%d\n", WEXITSTATUS(status));
			
				if (WIFSIGNALED(status))
						printf("child process is killed by signal %d\n", WTERMSIG(status));
			
				if (WIFSTOPPED(status))
						printf("child process is stopped by signal %d\n", WSTOPSIG(status));

				if (WIFCONTINUED(status))
						printf("child process resume running...\n");
			} while( !WIFEXITED(status) && !WIFSIGNALED(status));
			
			exit(0);
	}

	return 0;
}
