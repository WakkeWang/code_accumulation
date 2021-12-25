#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
//#include <syslog.h>

#include "cmd.h"
#include "log.h"

struct BaudRate{
	int speed;
	int bitmap;
};

struct BaudRate baudlist[] ={
{ 1200, B1200 },
{ 1800, B1800 },
{ 2400, B2400 },
{ 4800, B4800 },
{ 9600, B9600 },
{ 19200, B19200 },
{ 38400, B38400 },
{ 57600, B57600 },
{ 115200, B115200 },
{ 230400, B230400 },
{ 460800, B460800 },
{ 500000, B500000 },
{ 576000, B576000 },
{ 921600, B921600 },
};

int comDatabits[] =
{ 5, 6, 7, 8 };

int comStopbits[] =
{ 1, 2 };

int comParity[] =
{ 'n', 'o', 'e' };

void LOG(const char* ms, ... )
{
	char wzLog[1024] = {0};
	char buffer[1024] = {0};
	va_list args;
	va_start(args, ms);
	vsprintf( wzLog ,ms,args);
	va_end(args);

	time_t now;
	time(&now);
	struct tm *local;
	local = localtime(&now);
	sprintf(buffer,"%04d-%02d-%02d %02d:%02d:%02d %s\n", local->tm_year+1900, local->tm_mon,
				local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
				wzLog);
	LOG_ERROR("%s",buffer);
	FILE* file = fopen("testResut.log","a+");
	fwrite(buffer,1,strlen(buffer),file);
	fclose(file);

	return;
}


int set_com(int fd,int speed,int databits,int stopbits,int parity)
{
	int i;
	struct termios opt;

	if( tcgetattr(fd ,&opt) != 0){
		LOG_ERROR("get attr failed!\n");
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(baudlist); i++){
		struct BaudRate *rate = &baudlist[i];
		if (speed == rate->speed){
			cfsetispeed(&opt, rate->bitmap);
			cfsetospeed(&opt, rate->bitmap);
			break;
		}
	}

	opt.c_cflag &= ~CSIZE;
	switch (databits){
	case 5:
		opt.c_cflag |= CS5;
		break;
	case 6:
		opt.c_cflag |= CS7;
		break;
	case 7:
		opt.c_cflag |= CS7;
		break;
	case 8:
		opt.c_cflag |= CS8;
		break;
	default:
		LOG_ERROR("Unsupported data size\n");
		return -1;
	}

	switch(parity){
		case 'n':
		case 'N':
			opt.c_cflag &= ~PARENB;
			opt.c_iflag &= ~INPCK;
			break;
		case 'o':
		case 'O':
			opt.c_cflag |= (PARODD|PARENB);
			opt.c_iflag |= INPCK;
			break;
		case 'e':
		case 'E':
			opt.c_cflag |= PARENB;
			opt.c_cflag &= ~PARODD;
			opt.c_iflag |= INPCK;
			break;
		default:
			LOG_ERROR("Unsupported parity\n");
			return -1;
	}

	switch(stopbits){
		case 1:
			opt.c_cflag &= ~CSTOPB;
			break;
		case 2:
			opt.c_cflag |=  CSTOPB;
			break;
		default:
			LOG_ERROR("Unsupported stop bits\n");
			return -1;
	}

	opt.c_iflag &= ~(IXON | IXOFF | IXANY | BRKINT | ICRNL | INPCK | ISTRIP);
	opt.c_lflag &=  ~(ICANON | ECHO | ECHOE | IEXTEN | ISIG);
	opt.c_oflag &= ~OPOST;
	opt.c_cc[VTIME] = 100;
	opt.c_cc[VMIN] = 0;

	tcflush(fd, TCIOFLUSH);
	if (tcsetattr(fd, TCSANOW, &opt) != 0){
		LOG_ERROR("set attr failed!\n");
		return -1;
	}

	return 0;
}

int OpenDev(char* Dev)
{
	int fd = open(Dev,O_RDWR | O_NOCTTY );
	if( -1 == fd){
		LOG_ERROR("open failed! dev=%s\n",Dev);
		return -1;
	}
	else
		return fd;
}

int openDevice(const char* Dev,int speed,int databits,int stopbits,int parity)
{
	int fd;

	fd = open(Dev, O_RDWR | O_NOCTTY);
	if (-1 == fd){
		LOG_ERROR("open failed!\n");
		return -1;
	}

	if (set_com(fd, speed, databits, stopbits, parity) != 0){
		LOG_ERROR("Set Com Error\n");
		return -1;
	}

	return fd;
}

int pairTest(char * coma,char * comb,int speed,int databits,int stopbits,int parity)
{
	char MSG[] = {1,2,3,4,5,6,7,8,9,0};
	int ret;
	char buf[256];
	char result[256];
	int fd1,fd2;
	int idx;
	ssize_t wsize;

	fd1 = openDevice(coma, speed, databits, stopbits, parity);
	if (fd1 <= 0){
		LOG_ERROR("open device %s failed!\n", coma);
		exit(1);
	}

	fd2 = openDevice(comb, speed, databits, stopbits, parity);
	if (fd2 <= 0){
		LOG_ERROR("open device %s failed!\n", comb);
		exit(1);
	}

	wsize = write(fd1,MSG,sizeof(MSG));
//	usleep(1000000/speed*(4+databits)*sizeof(MSG)*2);

	memset(buf, 0, sizeof(buf));
	idx = 0;
	while ( idx < wsize ){
		ret = read(fd2, buf + idx, wsize - idx );
		if ( ret <= 0 ){
			break;
		}
		idx += ret;
	}
	if(memcmp(MSG,buf,sizeof(MSG)) != 0){
		sprintf(result," fail");
		LOG_ERROR("test %s to %s %d %d %d %c %s",coma,comb, speed, databits, stopbits, parity,result);
	}else{
		sprintf(result," OK");
		printf("test %s to %s %d %d %d %c %s\n",coma,comb, speed, databits, stopbits, parity,result);
	}

	close(fd1);
	close(fd2);

	return 0;
}

int speedTest(char * coma, char * comb, int highest_baud)
{
	int speedIdx, wordIdx, stopIdx, parityIdx;
	time_t old, new;

	old = time(NULL);
	for (speedIdx = 0; speedIdx < ARRAY_SIZE(baudlist); speedIdx++){
		for (wordIdx = 0; wordIdx < ARRAY_SIZE(comDatabits); wordIdx++){
			for (stopIdx = 0; stopIdx < ARRAY_SIZE(comStopbits); stopIdx++){
				for (parityIdx = 0; parityIdx < ARRAY_SIZE(comParity); parityIdx++){
						struct BaudRate *rate = &baudlist[speedIdx];
						if(rate->speed > highest_baud)
								break;
						pairTest(coma, comb, rate->speed, comDatabits[wordIdx], comStopbits[stopIdx], comParity[parityIdx]);
						pairTest(comb, coma, rate->speed, comDatabits[wordIdx], comStopbits[stopIdx], comParity[parityIdx]);
				}
			}
		}
	}
	new = time(NULL);
	LOG_INFO("speedtest %s and %s cost %5ld seconds",coma,comb, new - old);

	return 0;
}

static int cmd_serial_looptest(char *cmd_argv)
{
	int baud;
	int ret;
	char coma[64], comb[64];

	ret = sscanf(cmd_argv, "%[^,],%[^,],%d", coma, comb, &baud);
	LOG_DEBUG("cmd_argv=[%s], coma=%s, comb=%s, baud=%d",cmd_argv, coma, comb, baud);
	if(ret != 3){
		LOG_ERROR("Invalid parmeters, please input again");
		return -1;
	}
	while(1){
		pairTest(coma, comb, baud, 8, 1, 'n');
		pairTest(comb, coma, baud, 8, 1, 'n');
	}

	return 0;
}


static int cmd_serial_speedtest(char *cmd_argv)
{
	int highest_baud;
	int ret;
	char coma[64], comb[64];

	ret = sscanf(cmd_argv, "%[^,],%[^,],%d", coma, comb, &highest_baud);
	LOG_DEBUG("cmd_argv=[%s], coma=%s, comb=%s, highest_baud=%d",cmd_argv, coma, comb, highest_baud);
	if(ret != 3){
		LOG_ERROR("Invalid parmeters, please input again");
		return -1;
	}

	speedTest(coma, comb, highest_baud);

	return 0;
}

static struct cmd serial_cmds[] = {
	{CMD_SERIAL_LOOPTEST, cmd_serial_looptest},
	{CMD_SERIAL_SPEEDTEST, cmd_serial_speedtest},
};

int serial_cmds_register(void)
{
	register_cmds(serial_cmds, ARRAY_SIZE(serial_cmds));

	return 0;
}

