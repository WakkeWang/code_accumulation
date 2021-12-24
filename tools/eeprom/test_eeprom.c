#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define CONFIG_EEPROM_DEV "/dev/i2c-0"
#define CONFIG_EEPROM_PAGE_SIZE  8
#define EEPROM_ADDR 0x54
#define NVM_OFFSET  0x00
#define ENVSIZE (1024*8)

int read_eeprom(unsigned char slaveaddr, int offset, void *buf, int size)
{
	int rc;
	int fd;
	unsigned char addr[2];
	struct i2c_rdwr_ioctl_data ioctl_data;
	struct i2c_msg msgs[2] = {
		{
			.addr = slaveaddr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		}, {
			.addr  = slaveaddr,
			.flags = I2C_M_RD,
			.len = size,
			.buf = buf,
		},
	};

	fd = open(CONFIG_EEPROM_DEV, O_RDWR);
	if (fd < 0) {
		printf("cannot open device, %s\n", strerror(errno));
		return errno;
	}

	addr[0] = (offset & 0xff00) >> 8;
	addr[1] = offset & 0xff;
	ioctl_data.nmsgs = 2;
	ioctl_data.msgs = msgs;

	rc = ioctl(fd, I2C_RDWR, &ioctl_data);
	if (rc != 2) {
		printf("cannot read device, [%d] %s\n", rc, strerror(errno));
		rc = rc == -1 ? errno : EFAULT;
		goto err;
	}

	rc = 0;
err:
	close(fd);

	return rc;
}

int write_eeprom(int fd, unsigned char slaveaddr, int offset, void *buf, int size)
{
	int rc = -1, len;
	unsigned char temp[2 + CONFIG_EEPROM_PAGE_SIZE];
	struct i2c_rdwr_ioctl_data ioctl_data;
	struct i2c_msg msgs[1] = {
		{
			.addr = slaveaddr,
			.flags = 0,
			.len = 2,
			.buf = temp,
		},
	};

	if (!size)
		return -EINVAL;

	ioctl_data.nmsgs = 1;
	ioctl_data.msgs = msgs;

	if(size > CONFIG_EEPROM_PAGE_SIZE)
		size = CONFIG_EEPROM_PAGE_SIZE;

	len = CONFIG_EEPROM_PAGE_SIZE - offset % CONFIG_EEPROM_PAGE_SIZE;
	len = (len < size) ? len : size;
	ioctl_data.msgs->len = 2 + len;

	temp[0] = (offset & 0xff00) >> 8;
	temp[1] = offset & 0xff;
	memcpy(temp + 2, buf, len);
	rc = ioctl(fd, I2C_RDWR, &ioctl_data);
	if (rc != 1) {
		rc = -EFAULT;
		goto err;
	}
	usleep(3000);
	rc = len;

err:
	return rc;
}

int update_eeprom(unsigned char slaveaddr, int offset, void *buf, int size)
{
	int fd;
	int rc, i;
	int m;
	int buf_w[CONFIG_EEPROM_PAGE_SIZE];

	fd = open(CONFIG_EEPROM_DEV, O_RDWR);
	if (fd < 0)
		return errno;

	for (i = 0; i < size;) {
		rc = write_eeprom(fd, slaveaddr, offset + i, (char*)buf + i, size - i);
		if (rc > 0){
			i += rc;
#if 0
                        memset(buf_w, 0, sizeof(buf_w));
			usleep(3000);
                        read_eeprom(slaveaddr, offset + i, buf_w, rc);
                        if(!memcmp(buf_w, (char*)buf + i, rc)){
                                printf("compare %d bytes -----OK\n", rc);
                        }
                        else{
				printf("compare %d bytes -----failed.\n",rc);
				for(m = 0; m < rc; m++)
					printf("the %d bytes write=[0x%x] read=[0x%x]\n", m + 1, *((char *)buf + i + m), *(buf_w + m));
                        }
#endif
		}
		else
			goto err;
	}
	rc = 0;

err:
	close(fd);

	return rc;
}

int write_from_file(const char *filename, int offset)
{
	int fd;
	int rc, size;
	char *buf;
	struct stat filestat;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return errno;

	rc = fstat(fd, &filestat);
	if (rc < 0) {
		rc = errno;
		goto err;
	}

	size = filestat.st_size;
	buf = malloc(size);

	rc = read(fd, buf, size);
	if (rc != size) {
		rc = EFAULT;
		goto err_free;
	}

	printf("file size = %d \n", size);
	rc = update_eeprom(EEPROM_ADDR, offset, buf, size);

err_free:
	free(buf);
err:
	close(fd);

	return rc;
}

void print_buf( char *buf, int size)
{
	int i;

	printf("====start print buf===\n");
	for(i = 0; i < size; i++){
		printf("%c", *(buf+i));
	}
	printf("\n====stop print buf===\n");

}

int compare_buf( char *buf, int offset)
{
	int fd, ret;
	char buf_r[1024*10];
loop:
	memset(buf_r, 0, sizeof(buf_r));
	fd =  open("/root/eeprom.bin", O_CREAT | O_RDWR );
	ret = read(fd, buf_r, ENVSIZE);
	close(fd);
#if 0
	if(ret != ENVSIZE){
		printf("read env from file faile, read again , ret=%d\n",ret);
		goto loop;	
	}
#endif	
	printf("read env from file success, %d bytes\n",ret);

	return !memcmp(buf, buf_r + offset, ret);

}

int main(int argc, const char *argv[])
{
	int offset, kbytes;
	char buf_r[1024*64];
	char buf_w[1024*64];

	if(argc != 4){
		printf("usage:%s r offset 1/2/4/8 (kBytes each read)\n",argv[0]);
		printf("usage:%s w offset file\n",argv[0]);
		exit(-1);
	}	
	
	printf("dev=%s addr=0x%x\n",CONFIG_EEPROM_DEV,EEPROM_ADDR);

	if(!strncmp(argv[1], "r", 1)){
		sscanf(argv[2], "%x", &offset);
		sscanf(argv[3], "%x", &kbytes);
		printf("will read from eeprom , at offset=0x%x , %d kbytes each read\n",offset, kbytes);

		int s = 1024 * kbytes;
 		for(int i = 0; i < ENVSIZE/s; i++)
 		{
			read_eeprom(EEPROM_ADDR, offset + s * i, buf_r + s * i, s);
		}

		if(compare_buf(buf_r, offset)){
			printf("\n\n read OK\n\n");
		}else{
			printf("\n\n buf compare ERR , CRC ERR\n\n");
		}
		
	}

	if(!strncmp(argv[1], "w", 1)){
		sscanf(argv[2], "%x", &offset);
		printf("will write to eeprom , at offset=0x%x , file=%s\n",offset, argv[3]);

		write_from_file(argv[3], offset);
	}

	return 0;
}


