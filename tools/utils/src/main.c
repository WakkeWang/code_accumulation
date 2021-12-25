#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "list.h"
#include "log.h"
#include "cmd.h"

#include "mii.h"
#include "eeprom.h"
#include "serial.h"

#define VERSION		"1.0"

struct cmd_list{
	struct list_head list;
	struct cmd cmd;
};

struct cmd_list cmd_list;

int register_cmds(struct cmd r_cmds[], int count)
{
	int i;
	struct cmd_list *p = NULL;

	for(i = 0; i < count; i++)
	{
		p = (struct cmd_list*)malloc(sizeof(struct cmd_list));
		p->cmd = r_cmds[i];
		list_add(&p->list, &cmd_list.list);	
	}
	
	return 0;
}

const void *help = 
	"help:\n\n"
	"LogLevel:\n"
	"  -l                                               log level from 0-5,default is 3\n"
	"Mii:\n"
	"  --miiread=<interface,register_addr>              read phy register\n"
	"  --miiwrite=<interface,register_addr,value>       write phy register\n"
	"Serial:\n"
	"  --looptest=<device1,device2,baud>                loop test at 8n1\n"
	"  --speedtest=<device1,device2,highest_baud>       speed from 1200 -> highest_baud -> 921600\n"
	"eeprom:\n"
	"  --burneeff=<i2c_dev,dev_addr,off,file>           burn eeprom from file\n"
	"  --readeetf=<i2c_dev,dev_addr,off,len>            read eeprom to file: ./eeprom\n"
	"  --readee=<i2c_dev,dev_addr,type,off>             hexdump eeprom value at off\n"
	"                                                   type=[b,w,l], which represents 1/2/4 bytes\n"
	"  --writee=<i2c_dev,dev_addr,type,off,value>       write value to eeprom at off\n"
	"                                                   type=[b,w,l], which represents 1/2/4 bytes\n"
	"\n";

static const struct option long_options[] = {
	{"version",		0,					NULL, 'v'},
	{"help",		0,					NULL, 'h'},
	{"loglevel",	required_argument,	NULL, 'l'},
	{"miiread",		required_argument,	NULL, 0x1},
	{"miiwrite",	required_argument,	NULL, 0x2},
	{"looptest",	required_argument,	NULL, 0x3},
	{"speedtest",	required_argument,	NULL, 0x4},
	{"burneeff",	required_argument,	NULL, 0x5},
	{"readeetf",	required_argument,	NULL, 0x6},
	{"readee",		required_argument,	NULL, 0x7},
	{"writee",		required_argument,	NULL, 0x8},
	{ NULL,			0, 					0,    0},
};

static char cmd_argv[256];
static int flags = -1;
int log_level = 3;

static void parse_args(int argc, char *argv[])
{
	int c, option_index;

	if(argc == 1){
		printf(help);
		exit(0);
	}

	do {
		c = getopt_long(argc, argv, "l:hv", long_options, &option_index);

		if (c == -1)
			break;

		switch (c) {
		case 1:
			flags = CMD_MII_READ;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 2:
			flags = CMD_MII_WRITE;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 3:
			flags = CMD_SERIAL_LOOPTEST;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 4:
			flags = CMD_SERIAL_SPEEDTEST;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 5:
			flags = CMD_BURNEEFF;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 6:
			flags = CMD_READEETF;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 7:
			flags = CMD_READEE;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 8:
			flags = CMD_WRITEE;
			if (optarg)
				strcpy(cmd_argv, optarg);
			break;
		case 'l':
			log_level = atoi(optarg);
			LOG_DEBUG("Loglevel = %d", log_level);
			break;
		case 'v':
			printf("Version %s\n", VERSION);
			exit(0);
			break;
		case 'h':
			printf(help);
			exit(0);
			break;
		default:
			printf(help);
			exit(1);
			break;
		}
	} while (1);
}



int main( int argc, const char **argv)
{
	struct list_head *pos = NULL;

	INIT_LIST_HEAD(&cmd_list.list);

	parse_args(argc, (char **)argv);

	mii_cmds_register();
	serial_cmds_register();
	eeprom_cmds_register();

	list_for_each(pos, &cmd_list.list) {
		if(flags == ((struct cmd_list*)pos)->cmd.cmd_code){
			LOG_DEBUG("cmd->code=%d", ((struct cmd_list*)pos)->cmd.cmd_code);
			((struct cmd_list*)pos)->cmd.cmd_func(cmd_argv);			
			break;
		}
	}

	if(flags == -1){
		LOG_DEBUG("no such cmd, not registed,flags=%d",flags);
	}

	return 0;
}
