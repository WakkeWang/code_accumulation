#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "crc.h"

int main(int argc, const char *argv[])
{
	//unsigned char *buf1 = "abcdefgsdsfdsdfsdf"; sizeof(buf1)=8, pointer size
	
	unsigned char buf1[] = "abcdefgsdsfdsdfsdf";
	unsigned char buf2[] = "abcdefgsdsfdsdfsde";

	uint32_t crc = 0, crc_1 = 0, crc_2 = 0;

	printf("cpu is %s endian\n", __BYTE_ORDER == __LITTLE_ENDIAN ? "Little" : "Big");

	// check on http://www.ip33.com/crc.html 
	// following is right, buffer can not include "\0"
	crc_1 = crc32 ( crc_1, (unsigned char *)buf1, sizeof(buf1) - 1);
	crc_2 = crc32 ( crc_2, (unsigned char *)buf2, sizeof(buf2) - 1);

	printf("crc is %s\n", crc_1 == crc_2 ? "same" : "differ");

	printf("\ncrc1 of (%s) = 0x%x\n", buf1, crc_1);
	printf("crc2 of (%s) = 0x%x\n", buf2, crc_2);

	return 0;
}



