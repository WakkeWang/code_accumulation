#ifndef __CMD_H
#define __CMD_H

typedef unsigned int uint32_t;

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))

struct cmd{
	int cmd_code;
	int (*cmd_func)(char *argv);
};

enum {
	CMD_MII_READ,
	CMD_MII_WRITE,
	CMD_SERIAL_LOOPTEST,
	CMD_SERIAL_SPEEDTEST,
	CMD_BURNEEFF,
	CMD_READEETF,
	CMD_READEE,
	CMD_WRITEE,
};

int register_cmds(struct cmd cmds[], int count);

#endif
