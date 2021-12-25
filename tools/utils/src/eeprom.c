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

#include "cmd.h"
#include "log.h"

#define EEPROM_PAGE_SIZE  8

static int read_eeprom(char *device, unsigned char slaveaddr, int offset, void *buf, int size)
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

	fd = open(device, O_RDWR);
	if (fd < 0) {
		LOG_ERROR("cannot open device:%s, %s", device, strerror(errno));
		return errno;
	}

	addr[0] = (offset & 0xff00) >> 8;
	addr[1] = offset & 0xff;
	ioctl_data.nmsgs = 2;
	ioctl_data.msgs = msgs;

	rc = ioctl(fd, I2C_RDWR, &ioctl_data);
	if (rc != 2) {
		LOG_ERROR("cannot read device, [%d] %s\n", rc, strerror(errno));
		rc = rc == -1 ? errno : EFAULT;
		goto err;
	}

	rc = 0;
err:
	close(fd);

	return rc;
}

static int write_eeprom(int fd, unsigned char slaveaddr, int offset, void *buf, int size)
{
	int rc = -1, len;
	unsigned char temp[2 + EEPROM_PAGE_SIZE];
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

	if(size > EEPROM_PAGE_SIZE)
		size = EEPROM_PAGE_SIZE;

	len = EEPROM_PAGE_SIZE - offset % EEPROM_PAGE_SIZE;
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

static int update_eeprom(char *device, unsigned char slaveaddr, int offset, void *buf, int size)
{
	int fd;
	int rc, i;
	int m;
	int buf_w[EEPROM_PAGE_SIZE];

	fd = open(device, O_RDWR);
	if (fd < 0) {
		LOG_ERROR("cannot open device:%s, %s", device, strerror(errno));
		return errno;
	}

	for (i = 0; i < size;) {
		rc = write_eeprom(fd, slaveaddr, offset + i, (char*)buf + i, size - i);
		if (rc > 0){
			i += rc;
		}
		else
			goto err;
	}
	rc = 0;

err:
	close(fd);

	return rc;
}

static int write_from_file(char *device, int dev_addr, const char *filename, int offset)
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

	LOG_DEBUG("file size = %d \n", size);
	rc = update_eeprom(device, dev_addr, offset, buf, size);
	
	if(!rc)
		printf("burn success\n");

err_free:
	free(buf);
err:
	close(fd);

	return rc;
}


static int cmd_burneeff(char *cmd_argv)
{

	int ret;
	char eeprom_dev[64];
	char file_to_burn[264];
	int dev_addr, offset;

	LOG_DEBUG("cmd_argv=[%s]",cmd_argv);

	/** i2c_dev, dev_addr, off, file */
	ret = sscanf(cmd_argv, "%[^,],%x,%x,%s", eeprom_dev, &dev_addr, &offset, file_to_burn);
	LOG_DEBUG("eeprom_dev=%s, dev_addr=0x%x, offset=0x%x, file_to_burn=%s",eeprom_dev, dev_addr, offset, file_to_burn);
	if(ret != 4){
		LOG_ERROR("Invalid parmeters, please input again");
		return -1;
	}

	if(access(file_to_burn, R_OK)){
		LOG_ERROR("File not found :%s",file_to_burn);
		return -1;	
	}
	
	write_from_file(eeprom_dev, dev_addr, file_to_burn, offset);

	return 0;
}

static int cmd_readeetf(char *cmd_argv)
{
	char eeprom_dev[64];
	char buf_r[1024*512];
	int dev_addr, offset, len ,ret ,fd;

	LOG_DEBUG("cmd_argv=[%s]",cmd_argv);

	/** i2c_dev, dev_addr, off, len */
	ret = sscanf(cmd_argv, "%[^,],%x,%x,%d", eeprom_dev, &dev_addr, &offset, &len);
	LOG_DEBUG("eeprom_dev=%s, dev_addr=0x%x, offset=0x%x, len=%d", eeprom_dev, dev_addr, offset, len);
	if(ret != 4){
		LOG_ERROR("Invalid parmeters, please input again");
		return -1;
	}

	memset(buf_r, 0, sizeof(buf_r));
	if(read_eeprom(eeprom_dev, dev_addr, offset, buf_r, len)){
		LOG_ERROR("read %s at address 0x%x , offset=0x%x, len=%d", eeprom_dev, dev_addr, offset, len);
		return -1;
	}
	
	fd =  open("eeprom_r.bin", O_CREAT | O_RDWR );
	write(fd, buf_r, len);
	close(fd);

	printf("read eeprom to ./eeprom_r.bin success\n");
	
	return 0;
}

static int cmd_readee(char *cmd_argv)
{
	int ret;
	char eeprom_dev[64];
	int buf_r;
	char type;
	int len;
	int dev_addr, offset;

	LOG_DEBUG("cmd_argv=[%s]",cmd_argv);

	//i2c_dev, dev_addr, type, off
	ret = sscanf(cmd_argv, "%[^,],%x,%c,%x", eeprom_dev, &dev_addr, &type, &offset);
	LOG_DEBUG("eeprom_dev=%s, dev_addr=0x%x, type=%c, offset=0x%x",eeprom_dev, dev_addr, type, offset);
	if(ret != 4 || (type != 'b' && type != 'w' && type != 'l')){
		LOG_ERROR("Invalid parmeters, please input again");
		return -1;
	}

	if(read_eeprom(eeprom_dev, dev_addr, offset, &buf_r, 4)){
		LOG_ERROR("read %s at address 0x%x , offset=0x%x, len=%d", eeprom_dev, dev_addr, offset, len);
		return -1;
	}
	switch (type) {
		case 'b':
				printf("offset=0x%x, value=0x%x\n", offset, buf_r & 0xff);
				break;
		case 'w':
				printf("offset=0x%x, value=0x%x\n", offset, buf_r & 0xffff);
				break;
		case 'l':
				printf("offset=0x%x, value=0x%x\n", offset, buf_r & 0xffffffff);
				break;
	}

	return 0;
}

static int cmd_writee(char *cmd_argv)
{
	int ret;
	char eeprom_dev[64];
	char type;
	int len;
	int dev_addr, offset;
	char value[4];

	LOG_DEBUG("cmd_argv=[%s]",cmd_argv);

	//i2c_dev, dev_addr, type, off, value
	ret = sscanf(cmd_argv, "%[^,],%x,%c,%x,%x", eeprom_dev, &dev_addr, &type, &offset, value);
	LOG_DEBUG("eeprom_dev=%s, dev_addr=0x%x, type=%c, offset=0x%x, value=0x%x",eeprom_dev, dev_addr, type, offset, *((int*)value));
	if(ret != 5 || (type != 'b' && type != 'w' && type != 'l')){
		LOG_ERROR("Invalid parmeters, please input again");
		return -1;
	}

	switch (type) {
		case 'b':
				if(!update_eeprom(eeprom_dev, dev_addr, offset, value, 1))
					printf("offset=0x%x, value=0x%x\n", offset, *((int*)value) & 0xff);
				break;
		case 'w':
				if(!update_eeprom(eeprom_dev, dev_addr, offset, value, 2))
					printf("offset=0x%x, value=0x%x\n", offset, *((int*)value) & 0xffff);
				break;
		case 'l':
				if(!update_eeprom(eeprom_dev, dev_addr, offset, value, 4))
					printf("offset=0x%x, value=0x%x\n", offset, *((int*)value) & 0xffffffff);
				break;
	}
	
	return 0;
}

static struct cmd mii_cmds[] = {
	{CMD_BURNEEFF, cmd_burneeff},
	{CMD_READEETF, cmd_readeetf},
	{CMD_READEE, cmd_readee},
	{CMD_WRITEE, cmd_writee},
};

int eeprom_cmds_register(void)
{
	register_cmds(mii_cmds, ARRAY_SIZE(mii_cmds));

	return 0;
}

