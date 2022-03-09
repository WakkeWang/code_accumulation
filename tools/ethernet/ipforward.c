#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ether.h>
#include <net/if.h>// struct ifreq
#include <sys/ioctl.h> // ioctl、SIOCGIFADDR
#include <sys/socket.h> // socket
#include <netinet/ether.h> // ETH_P_ALL
#include <netpacket/packet.h> // struct sockaddr_ll

#include <stdlib.h>

void *write_fifo(void *info);
void *read_fifo(void *info);

typedef struct packet_info_t{
	struct sockaddr_ll sll;             //原始套接字地址结构  
	struct sockaddr_ll sll_iso;             //原始套接字地址结构  
	//struct ifreq req;                   //网络接口地址  
	uint32_t minlen, maxlen;	
	uint32_t pack_count, data_count;	
	uint32_t run;
	char buffer[4096];
	int isofd;
	int netfd;
	int debug;
	int interval;
	struct timeval prev, current;
}packet_info;

void printHexT(unsigned char *name, unsigned char *c, int n)
{
	int i;
	printf("\n---------------------[%s ,len = %d, start ]----------------------\n",name,n);
	printf("%04d:  ",0);
	for(i=0;i<n;i++)
	{
		printf("%02X",c[i]);
		if(i%6==5)
			printf(" ");
		if(i%18==17){
			printf("\n");
			printf("%04d:  ",i);
		}
	}
	if(i%16!=0)
		printf("\n");

	printf("----------------------[%s       end        ]----------------------\n",name);

}

void set_misc(int sock_raw_fd,int misc,char *ethname)
{
	struct ifreq req;	//网络接口地址
	int ret;

	strncpy(req.ifr_name, ethname, IFNAMSIZ);		//指定网卡名称
	if(-1 == ioctl(sock_raw_fd, SIOCGIFFLAGS, &req))	//获取网络接口
	{
		perror("ioctl");
		close(sock_raw_fd);
		exit(-1);
	}

	if(misc)
		req.ifr_flags |= IFF_PROMISC;
	else
		req.ifr_flags &= ~IFF_PROMISC;
	if(-1 == ioctl(sock_raw_fd, SIOCSIFFLAGS, &req))	//网卡设置混杂模式
	{
		perror("ioctl");
		close(sock_raw_fd);
		exit(-1);
	}
}

packet_info *isoinfo;

void update_info(const char *title, packet_info *info, unsigned int datalen)
{
	if (info->debug) {
		if(datalen >= info->maxlen)
			info->maxlen = datalen;
		if(datalen <= info->minlen)
			info->minlen = datalen;
		info->pack_count++;
		info->data_count += datalen;
		gettimeofday(&info->current, NULL);
		if (info->current.tv_sec - info->prev.tv_sec >= info->interval) {
			printf("%s:\n", title);
			printf("packet count: %d ", info->pack_count);
			printf("minlen: %d, maxlen: %d, average: %.1f, speed: %0.4f Mbps\n",
					info->minlen, info->maxlen, (float)info->data_count / info->pack_count,
					info->data_count * 8.0 / (info->current.tv_sec - info->prev.tv_sec) / 1024 / 1024);
			info->pack_count = 0;
			info->data_count = 0;
			info->maxlen = 0;
			info->minlen = 8192;
			info->prev = info->current;
		}
	}
}

int bind_eth(int skfd, char *ethname)
{
	struct sockaddr_ll fromaddr;
	struct ifreq ifr;

	bzero(&fromaddr,sizeof(fromaddr));
	bzero(&ifr,sizeof(ifr));
	strcpy(ifr.ifr_name, ethname);

	//获取接口索引
	if(-1 == ioctl(skfd,SIOCGIFINDEX,&ifr)){
		perror("get dev index error:");
		exit(1);
	}
	fromaddr.sll_ifindex = ifr.ifr_ifindex;
	fromaddr.sll_family = PF_PACKET;
	fromaddr.sll_protocol=htons(ETH_P_ALL);

	return bind(skfd,(struct sockaddr*)&fromaddr, sizeof(fromaddr));
}


/*
 *int bind_eth(int skfd, char *ethname)
 *{
 *        struct sockaddr_ll fromaddr;
 *        struct ifreq ifr;
 *
 *        bzero(&fromaddr,sizeof(fromaddr));
 *        bzero(&ifr,sizeof(ifr));
 *        strcpy(ifr.ifr_name, ethname);
 *
 *        //获取接口索引
 *        if(-1 == ioctl(skfd,SIOCGIFINDEX,&ifr)){
 *                perror("get dev index error:");
 *                exit(1);
 *        }
 *        printf("interface Index:%d\n",ifr.ifr_ifindex);
 *
 *        fromaddr.sll_ifindex = ifr.ifr_ifindex;
 *        fromaddr.sll_family = PF_PACKET;
 *        fromaddr.sll_protocol=htons(ETH_P_ALL);
 *
 *        return bind(skfd,(struct sockaddr*)&fromaddr,sizeof(struct sockaddr));
 *}
 */


int main(int argc, char *argv[])
{
	int ret;
	pthread_t wpid, rpid;
	socklen_t len = sizeof(struct sockaddr_in);
	int netfd, isofd;
	char *ethname = "eth0";
	char *ethname2 = "eth1";
	packet_info *out_info, *in_info;
	struct sockaddr_ll sll;             //原始套接字地址结构  
	struct ifreq req;                   //网络接口地址  

	if (argc > 2) {
		ethname = argv[1];
		ethname2 = argv[2];
	}

	out_info = calloc(1, sizeof(packet_info));
	if (!out_info)
		return -1;

	in_info = calloc(1, sizeof(packet_info));
	if (!in_info)
		return -1;

	isofd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
	if(isofd < 0){
		printf("socket error\n");
		exit(-1);
	}
	set_misc(isofd, 1, ethname2);

	out_info->isofd = isofd;
	in_info->isofd = isofd;

	netfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
	if(netfd < 0){
		printf("socket error\n");
		exit(-1);
	}
	set_misc(netfd, 1, ethname);

	out_info->netfd = netfd;
	in_info->netfd = netfd;

	strncpy(req.ifr_name, ethname, IFNAMSIZ);     //指定网卡名称  
	if(-1 == ioctl(netfd, SIOCGIFINDEX, &req))    //获取网络接口  
	{  
		perror("ioctl");  
		close(netfd);  
		exit(-1);  
	}

	/*将网络接口赋值给原始套接字地址结构*/  
	bzero(&out_info->sll, sizeof(out_info->sll));  
	out_info->sll.sll_ifindex = req.ifr_ifindex;  

	/*将网络接口赋值给原始套接字地址结构*/  
	bzero(&in_info->sll, sizeof(in_info->sll));  
	in_info->sll.sll_ifindex = req.ifr_ifindex;

	ret = bind_eth(netfd, ethname);
	if(ret){
		perror("1bind eth");
		return -1;
	}

	bzero(&req, sizeof(req));
	strncpy(req.ifr_name, ethname2, IFNAMSIZ);     //指定网卡名称  
	if(-1 == ioctl(netfd, SIOCGIFINDEX, &req))    //获取网络接口  
	{  
		perror("ioctl");  
		close(netfd);  
		exit(-1);  
	}

	/*将网络接口赋值给原始套接字地址结构*/  
	bzero(&out_info->sll_iso, sizeof(out_info->sll_iso));  
	out_info->sll_iso.sll_ifindex = req.ifr_ifindex;  

	/*将网络接口赋值给原始套接字地址结构*/  
	bzero(&in_info->sll_iso, sizeof(in_info->sll_iso));  
	in_info->sll_iso.sll_ifindex = req.ifr_ifindex;

	ret = bind_eth(isofd, ethname2);
	if(ret){
		perror("bind eth");
		return -1;
	}

	printf("start ok\n");

	in_info->run = 1;
	in_info->debug = 0;
	in_info->interval = 10;
	ret = pthread_create(&rpid, NULL, read_fifo, in_info);
	if(ret)
		goto out;

	out_info->run = 1;
	out_info->debug = 0;
	out_info->interval = 10;
	ret = pthread_create(&wpid,NULL, write_fifo, out_info);
	if(ret)
		goto out;
	pthread_join(wpid,NULL);
	pthread_join(rpid,NULL);

out:
	printf("exit ok\n");
	set_misc(isoinfo->netfd, 0, ethname);
	close(isoinfo->netfd);
	close(isoinfo->isofd);
	return 0;
}

void *write_fifo(void *info)
{
	packet_info *p = info;
	int ret;

	gettimeofday(&p->prev, NULL);
	while(p->run) {
		//socklen_t len = sizeof(p->sll);
		//ret = recvfrom(p->netfd, p->buffer, sizeof(p->buffer), 0 , (struct sockaddr *)&p->sll, &len);  
		ret = read(p->netfd, p->buffer, 8192);
		if(ret <= 0) {
			continue;
		}
		update_info(__func__, p, ret);
		//printHexT("get net",p->buffer,ret);
		//ret = write(p->isofd, p->buffer, ret);
		ret = sendto(p->isofd, p->buffer, ret, 0 , (struct sockaddr *)&p->sll_iso, sizeof(p->sll_iso));  
		if (ret <= 0) {
			printf("fifo is full or error lost packet len %d\n",ret);
			continue;
		}
	}
	return NULL;
}

void *read_fifo(void *info)
{
	packet_info *p = info;
	int ret;

	gettimeofday(&p->prev, NULL);
	while(p->run) {
		ret = read(p->isofd, p->buffer, 2048);
		if(ret <= 0) {
			continue;
		}

		update_info(__func__, p, ret);

		//printHexT("get fifo",p->buffer,ret);
		ret = sendto(p->netfd, p->buffer, ret, 0 , (struct sockaddr *)&p->sll, sizeof(p->sll));  
		if (ret == -1) {  
			perror("sendto");  
			continue;
		}
	}
	return NULL;
}
