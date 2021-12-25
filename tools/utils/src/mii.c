#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/mii.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>
#include <netinet/in.h>

#include "cmd.h"
#include "log.h"

#define reteck(ret)     \
        if(ret < 0){    \
            LOG_ERROR("%m!");   \
            goto out;   \
        }

static int cmd_mii_read(char *cmd_argv)
{
	int ret;
	char ifname[64];
	uint32_t register_addr;

	struct mii_ioctl_data *mii = NULL;
	struct ifreq ifr;
	int sockfd;

	ret = sscanf(cmd_argv, "%[^,],%x", ifname, &register_addr);
	if(ret != 2)
	{
		LOG_ERROR("Invalid parmeters, please input again");	
		return -1;
	}
	LOG_DEBUG("cmd_argv=[%s], ifname=%s, register_addr=0x%x",cmd_argv, ifname, register_addr);
	
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
	reteck(sockfd);

	//get phy address in smi bus
	ret = ioctl(sockfd, SIOCGMIIPHY, &ifr);
	reteck(ret);

	mii = (struct mii_ioctl_data*)&ifr.ifr_data;
	mii->reg_num    = (uint16_t)register_addr;
	ret = ioctl(sockfd, SIOCGMIIREG, &ifr);
	reteck(ret);

	printf("mii-read: phy addr: 0x%x  reg: 0x%x   value : 0x%x\n", mii->phy_id, mii->reg_num, mii->val_out);


out:
	close(sockfd);
	return 0;
}

static int cmd_mii_write(char *cmd_argv)
{
	int ret;
	char ifname[64];
	uint32_t register_addr, value;

	struct mii_ioctl_data *mii = NULL;
	struct ifreq ifr;
	int sockfd;

	ret = sscanf(cmd_argv, "%[^,],%x,%x", ifname, &register_addr, &value);
	if(ret != 3)
	{
		LOG_ERROR("Invalid parmeters, please input again");	
		return -1;
	}

	LOG_DEBUG("cmd_argv=[%s], ifname=%s, register_addr=0x%x, val=0x%x",cmd_argv, ifname, register_addr, value);
    
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
	sockfd = socket(PF_LOCAL, SOCK_DGRAM, 0);
	reteck(sockfd);

	//get phy address in smi bus
	ret = ioctl(sockfd, SIOCGMIIPHY, &ifr);
	reteck(ret);

	mii = (struct mii_ioctl_data*)&ifr.ifr_data;
	mii->reg_num    = (uint16_t)register_addr;
	mii->val_in     = (uint16_t)value;

	ret = ioctl(sockfd, SIOCSMIIREG, &ifr);
	reteck(ret);
 
	printf("mii-write: phy addr: 0x%x  reg: 0x%x  value : 0x%x\n", mii->phy_id, mii->reg_num, mii->val_in);

out:
	close(sockfd);
	return 0;
}


static struct cmd mii_cmds[] = {
	{CMD_MII_READ, cmd_mii_read},
	{CMD_MII_WRITE, cmd_mii_write},
};

int mii_cmds_register(void)
{
	register_cmds(mii_cmds, ARRAY_SIZE(mii_cmds));

	return 0;
}
