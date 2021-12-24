#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#define	RET_ERROR			 -1 
#define	RET_FAIL			  0 
#define	RET_SUCCESS			  1 

#define       NIC1_NIC_IND		  1 
#define       NIC2_NIC_IND		  2 

int g_eth1_index = 0;
int g_eth2_index = 0;

const char  ETH_NIC_DEV1[16]        = "eth0";
const char  ETH_NIC_DEV2[16]        = "eth1";

/*
函数名称：inline int active_dev( const int sockFd, const char * dev)
功能说明：激活网口
输入参数：
    sockFd:套接字描述符
    dev：对应的网口
输出参数：无
返回值：
    int 成功返回1，失败返回0
关系函数：socket（）
线程安全：否
*/
static int active_dev(const int sockFd, const char* dev)
{
    int     ret;
    struct  ifreq req;

    bzero(&req, sizeof(struct ifreq));

    strncpy(req.ifr_name, dev, sizeof(req.ifr_name));

    ret = ioctl(sockFd, SIOCGIFFLAGS, &req);
    if (ret == -1)
    {
        return(RET_FAIL);
    }
    req.ifr_flags |= IFF_UP;

    ret = ioctl(sockFd, SIOCSIFFLAGS, &req);
    if (ret == -1)
    {
        return(RET_FAIL);
    }

    return(RET_SUCCESS);
}

/*
    函数名称：inline int open_socket( int & sockFd)
    功能说明：新建套接字。
    输入参数：sockFd:套接字描述符
    输出参数：无
    返回值：
        int 成功返回新建的套接字描述符，失败返回0
    关系函数：无
    线程安全：否
*/
int open_socket(void)
{
    struct ifreq ifr;
    int s;
    int ret;
    int sockFd = 0;

    sockFd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockFd < 0)
    {
        exit(0);
    }

    ret = active_dev(sockFd, ETH_NIC_DEV1);
    if (ret == RET_FAIL)
    {
        close(sockFd);
        exit(0);
    }

    strncpy(ifr.ifr_name, ETH_NIC_DEV1, sizeof(ifr.ifr_name));
    s = ioctl(sockFd, SIOCGIFFLAGS, &ifr);
    if (s < 0)
    {
        close(sockFd);
        exit(0);
    }

    ifr.ifr_flags |= IFF_PROMISC;
    ifr.ifr_flags |= IFF_UP;

    s = ioctl(sockFd, SIOCSIFFLAGS, &ifr);
    if (s < 0)
    {
    }

    ioctl(sockFd, SIOCGIFINDEX, &ifr);
    //sl.sll_ifindex = ifr.ifr_ifindex;
    g_eth1_index = ifr.ifr_ifindex;

    ret = active_dev(sockFd, ETH_NIC_DEV2);
    if (ret == RET_FAIL)
    {
        close(sockFd);
        exit(0);
    }

    strncpy(ifr.ifr_name, ETH_NIC_DEV2, sizeof(ifr.ifr_name));
    s = ioctl(sockFd, SIOCGIFFLAGS, &ifr);
    if (s < 0)
    {
        close(sockFd);
        exit(0);
    }

    ifr.ifr_flags |= IFF_PROMISC;
    ifr.ifr_flags |= IFF_UP;

    s = ioctl(sockFd, SIOCSIFFLAGS, &ifr);
    if (s < 0)
    {
    }

    ioctl(sockFd, SIOCGIFINDEX, &ifr);
    //sl.sll_ifindex = ifr.ifr_ifindex;
    g_eth2_index = ifr.ifr_ifindex;
    return(sockFd);
}

/*
    函数名称：inline int set_sockopt( const int sockFd)
    功能说明：对新建的套接字进行设置。
    输入参数：
        sockFd:套接字描述符
    输出参数：无
    返回值：
        int 成功返回0，失败返回0
    关系函数：无
    线程安全：否
*/
int set_sockopt(const int sockFd)
{
    int optval = 0;
    int optlen = 0;
    int ret;

    ret = getsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &optval, (socklen_t*)&optlen);
    if (ret < 0)
    {
        return(RET_FAIL);
    }

    optval = 1024 * 128;// * 16;
    optlen = sizeof(optval);
    ret = setsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &optval, (socklen_t)optlen);
    if (ret < 0)
    {
        return(RET_FAIL);
    }
    ret = getsockopt(sockFd, SOL_SOCKET, SO_RCVBUF, &optval, (socklen_t*)&optlen);
    if (ret < 0)
    {
        return(RET_FAIL);
    }
    optval = 1024 * 512;// * 8;
    optlen = sizeof(optval);
    ret = setsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &optval, (socklen_t)optlen);
    if (ret < 0)
    {
        return(RET_FAIL);
    }
    ret = getsockopt(sockFd, SOL_SOCKET, SO_SNDBUF, &optval, (socklen_t*)&optlen);
    if (ret < 0)
    {
        return(RET_FAIL);
    }
    return(RET_SUCCESS);
}

/*
    函数名称：int recv_via_dev( const int sockFd, char & devInd,char * data, const
int len)
    功能说明：封装的接收函数
    输入参数：
        sockFd:套接字描述符
        devInd:网口标识
        data:接收的数据
        Len：接收数据的长度
    输出参数：无
    返回值：int 成功返回1，失败返回0
    关系函数：无
    线程安全：是
*/
int recv_via_dev(const int sockFd, int* devInd, char* data, int len)
{
    int ret = 0;
    struct sockaddr_ll sa;
    socklen_t sz = sizeof(sa);

    ret = recvfrom(sockFd, data, len, 0, (struct sockaddr*)&sa, &sz);   //function defined in system
    if (ret == -1)
    {
        return -1;
    }

    if (sa.sll_ifindex == g_eth1_index)
    {
        *devInd = NIC1_NIC_IND;
    }
    else if (sa.sll_ifindex == g_eth2_index)
    {
        *devInd = NIC2_NIC_IND;
    }
    else
    {
        return -1;
    }

    return(ret);
}


/*
    函数名称：int send_via_dev( const int sockFd, const char devInd, char * & data,
int & len, unsigned short vlanid=0)
    功能说明：封装的发送函数
    输入参数：
            sockFd:套接字描述符
            devInd:网口标识
            data:发送的数据
            Len：发送数据的长度
            vlanid:vlan标识
    输出参数：无
    返回值：int 成功返回1，失败返回0
    关系函数：无
    线程安全：是
*/
int send_via_dev(const int sockFd, int devInd, char* data, int len)
{
    int ret = 0;
    struct sockaddr_ll sa;

    sa.sll_family = AF_INET;
    memcpy(sa.sll_addr, data, 6);
    sa.sll_halen = 6;
    if (devInd == NIC1_NIC_IND)
    {
        sa.sll_ifindex = g_eth1_index;
    }
    else if (devInd == NIC2_NIC_IND)
    {
        sa.sll_ifindex = g_eth2_index;
    }
    else
    {
        return -1;
    }

    ret = sendto(sockFd, data, len, 0, (struct sockaddr*)&sa, sizeof(sa));
    if (ret < 0)
    {
    }

    return(ret);
}

int main(void)
{
    char data[1500] = {0};
    int sockFd = open_socket();
    int devInd = 0;
    int recvLen = 0;
    struct ethhdr *     eth;

    set_sockopt(sockFd);
    while(1)
    {
        recvLen = recv_via_dev(sockFd, &devInd, data, 1500);
        if (recvLen >= 0)
        {
            //eth = (struct ethhdr*)data;
            //printf("port eth%d recv proto %x\n", devInd, ntohs(eth->h_proto));
            if (NIC1_NIC_IND == devInd)
            {
                devInd = NIC2_NIC_IND;
            }
            else if (NIC2_NIC_IND == devInd)
            {
                devInd = NIC1_NIC_IND;
            }
            send_via_dev(sockFd, devInd, data, recvLen);
        }
        //fflush(stdout);
        //fflush(stderr);
    }
    return 0;
}

