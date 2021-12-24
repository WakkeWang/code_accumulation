#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_SIZE 1024*8

int main()
{
	int fd = 0, i = 0;
	char buf[MAX_SIZE] = { 0 };

	fd = open( "ee.bin", O_CREAT | O_WRONLY);
	printf("fd=%d\n",fd);

	for (i = 0; i < MAX_SIZE; i++)
	{
		buf[i] = 0xa5;
	}

	write(fd, buf, MAX_SIZE);

	close(fd);
}

