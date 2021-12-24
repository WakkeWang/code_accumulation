#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>


#define LAN7500_MAGIC 0x7500

char eeprom[512]={
0xa5,0x02,0x80,0x0f,0x11,0x70,0x02,0x01,0x01,0x7b,0x09,0x04,0x0a,0x23,0x10,0x28, //[0-15]
0x1a,0x30,0x08,0x3d,0x08,0x41,0x12,0x11,0x12,0x1a,0x12,0x11,0x12,0x1a,0x00,0x00, //[16-31]
0x04,0x7c,0x12,0x01,0x00,0x02,0xff,0x00,0xff,0x40,0x24,0x04,0x00,0x75,0x00,0x01, //[32-47]
0x01,0x02,0x03,0x01,0x09,0x02,0x27,0x00,0x01,0x01,0x04,0xa0,0x32,0x09,0x04,0x00, //[48-63]
0x00,0x03,0xff,0x00,0xff,0x05,0x0a,0x03,0x53,0x00,0x4d,0x00,0x53,0x00,0x43,0x00, //[64-79]
0x10,0x03,0x4c,0x00,0x41,0x00,0x4e,0x00,0x37,0x00,0x35,0x00,0x30,0x00,0x30,0x00, //[80-95]
0x1a,0x03,0x30,0x00,0x30,0x00,0x38,0x00,0x30,0x00,0x30,0x00,0x66,0x00,0x31,0x00, //[96-111]
0x31,0x00,0x37,0x00,0x30,0x00,0x30,0x00,0x31,0x00,0x08,0x03,0x43,0x00,0x66,0x00,
0x67,0x00,0x08,0x03,0x69,0x00,0x2f,0x00,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


extern char* optarg;
extern int optind,opterr,optopt;
int getopt(int argc, char * const argv[], const char *optstring);

void help()
{
	/*
	 *  ./mac_burn -m address -o eeprom.bin 
	 *		-m   mac address to burn.
	 *		-o   to give a name of eeprom.bin to be generated.
	 *		-f   eeprom.bin file to burn
	 */

	printf("\n Usage:\n");
	printf("./mac_burn -I eth0 -m addr -b                  : to burn eeprom according to mac_address.\n");
	printf("./mac_burn -I eth0 -i eeprom.bin -b            : burn eeprom from the bin file.\n");
	printf("./mac_burn -m mac_addr -o eeprom.bin   : to generated a bin file according to mac_address.\n");
	printf("\n\n");
}


#define HAVE_BURN   0x01 
#define HAVE_MAC    0x02 
#define HAVE_FILE   0x04
#define HAVE_IFNAME 0x08



//mode= 0x0b  burn from mac_address
//mode= 0x0d  burn from bin_file 
//mode= 0x06  generate a bin file
char mode = 0;

char ifname[12];
char eeprom_file[32];
char mac_address[32];

char ee_filename[32]=".eeprom.bin";

void generate_bin_file()
{
	int i;

	if( mode & HAVE_MAC )  // replace mac_address 
	{
		int m=1;

#if 0	
		const char *s = ":";
		char * token;
		unsigned char mac;


		token = strtok(mac_address, s);

		while( NULL != token )
		{
			sscanf(token, "%x%x", &mac);

			eeprom[m++] = mac;
			printf("token_char = %s\n",token);

			printf("mac=0x%x\n",mac);
			
			token = strtok(NULL, s);
		}
#else
		int i;
		unsigned char mac[6]={0};
		sscanf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",&mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
		//printf("Mac is %s,mac is %02x:%02x:%02x:%02x:%02x:%02x\n",mac_address,mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	
		for(m=0;m<6;m++)
		{
			eeprom[m+1] = mac[m];
		}
		
#endif 
	
	}


	if( mode & HAVE_FILE )  // replace file name 
		strcpy(ee_filename, eeprom_file);


	FILE *pfile = fopen(ee_filename, "wb") ;
	if(pfile == NULL)
	{
		printf("open failed\n");
		return;
	}

	for(i=0;i<512;i++)
	{
		fwrite(eeprom+i, 1, 1, pfile);
	}

	fclose(pfile);
}

void burn_from_mac()
{
	char cmd[128];
	generate_bin_file();
	int ret=0;

	if( access(ee_filename, F_OK) == 0 )
	{
		sprintf(cmd,"ethtool -E %s magic 0x%x offset 0x00 length 512 < %s",ifname, LAN7500_MAGIC, ee_filename);
		ret = system(cmd);
		if(ret)
			printf("burn failed!\n");
		else
			printf("burn success!\n");
	}

#if 0
	if( !(mode & HAVE_FILE))
	{
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd,"rm %s",ee_filename);
		system(cmd);
	}
#endif
}

void burn_from_file()
{
	int ret = 0;
	char cmd[128];
	if( access(ee_filename, F_OK) == 0 )
	{
		sprintf(cmd,"ethtool -E %s magic 0x%x offset 0x00 length 512 < %s",ifname, LAN7500_MAGIC, ee_filename);
		ret = system(cmd);
		if(ret)
			printf("burn failed!\n");
		else
			printf("burn success!\n");
	}
}


int main(int argc, const char *argv[])
{

	if(argc != 5 && argc !=6 )
	{
		help();
		return -1;
	}

	int ch;

	while( (ch = getopt(argc, argv, "bm:o:i:I:")) != -1)
	{
	//	printf("optind:%d  ch=%c\n",optind,ch);
		switch (ch)
		{
			case 'b':
				mode |= HAVE_BURN;
				break;
			case 'm':
				mode |= HAVE_MAC;
				memset(mac_address, 0, sizeof(mac_address));
				strcpy(mac_address,optarg);
				if(strlen(mac_address) != 17)
				{
					printf("mac_address is invalid\n");
					return -1;
				}
				break;
			case 'i':
			case 'o':
				mode |= HAVE_FILE;
				memset(eeprom_file, 0, sizeof(eeprom_file));
				strcpy(eeprom_file,optarg);
				break;
			case 'I':
				mode |= HAVE_IFNAME;
				memset(ifname, 0, sizeof(ifname));
				strcpy(ifname,optarg);
				break;
			default :
				printf("error option:[%c=%d]\n",ch,ch);
				return -1;
		}
	}

	if(mode == 0x0b)
	{
		printf("\nburn from mac_address...\n");
		printf("mac_address= %s \n", mac_address);
		printf("ifname = %s\n", ifname);

		burn_from_mac();
	}
	else if(mode == 0x0d)
	{
		printf("\nburn from bin_file\n ");
		printf("binfile= %s  \n", eeprom_file);
		printf("ifname = %s\n", ifname);

		burn_from_file();
	}
	else if(mode == 0x06)
	{
		printf("\ngenerate a bin file according to mac_address\n");
		printf("binfile= %s  \n", eeprom_file);
		printf("mac_address= %s  \n", mac_address);

		generate_bin_file();
	}
	else{
		help();
	}

	return 0;

}
