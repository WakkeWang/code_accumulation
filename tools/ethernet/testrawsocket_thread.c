#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#define	RET_ERROR			 -1
#define	RET_FAIL			  0
#define	RET_SUCCESS			  1

#define       NIC0_NIC_IND		  0
#define       NIC1_NIC_IND		  1 
#define       NIC2_NIC_IND		  2 
#define       NIC3_NIC_IND		  3 
#define       NIC4_NIC_IND		  4 
#define       NIC5_NIC_IND		  5 
#define       BUTT_NIC_IND		  6 

int g_eth_index[BUTT_NIC_IND] = {0};
int gSock_fds[BUTT_NIC_IND] = {0};

const char  ETH_NIC_DEV0[16]        = "eth0";
const char  ETH_NIC_DEV1[16]        = "eth1";
const char  ETH_NIC_DEV2[16]        = "eth2";
const char  ETH_NIC_DEV3[16]        = "eth3";
const char  ETH_NIC_DEV4[16]        = "eth4";
const char  ETH_NIC_DEV5[16]        = "eth5";
const char *ETH_NIC_DEVS[BUTT_NIC_IND]        = 
{
    ETH_NIC_DEV0,
    ETH_NIC_DEV1,
    ETH_NIC_DEV2,
    ETH_NIC_DEV3,
    ETH_NIC_DEV4,
    ETH_NIC_DEV5,
};

#define RWOPT2STR(op) (((op) == SO_RCVBUF) ? "SO_RCVBUF" : "SO_SNDBUF")

enum{
    G_NUM_IP = 0,
    G_NUM_IP_FRAG_DROP,
    G_NUM_IPV4,
    G_NUM_IPV4_UNKNOWN,
    G_NUM_IPV4_NOSESS_DROP,
    G_NUM_UDP_POL,
    G_NUM_UDP_NOAUTH,
    G_NUM_UDP_AUTH,
    G_NUM_UDP_DROP,
    G_NUM_TCP_POL,
    G_NUM_TCP_NOAUTH,
    G_NUM_TCP_AUTH,
    G_NUM_TCP_DROP,
    G_NUM_ICMP_POL,
    G_NUM_ICMP_DROP,
    G_NUM_MAX,
};

char *g_str[G_NUM_MAX] = {
    "IP",
    "IP_FRAG_DROP",
    "IPV4",
    "IPV4_UNKNOWN",
    "IPV4_NOSESS_DROP",
    "UDP_POL",
    "UDP_NOAUTH",
    "UDP_AUTH",
    "UDP_DROP",
    "TCP_POL",
    "TCP_NOAUTH",
    "TCP_AUTH",
    "TCP_DROP",
    "ICMP_POL",
    "ICMP_DROP",
};
    
unsigned int g_num[2][G_NUM_MAX] = {{0},{0}};

void printgnum(int beg, int end)
{
    int i = 0;

    printf("forward :");
    for (i = beg; i < end; i++)
    {
        printf("%s=%u ", g_str[i], g_num[0][i]);
    }
    printf("\n");
    printf("backward:");
    for (i = beg; i < end; i++)
    {
        printf("%s=%u ", g_str[i], g_num[1][i]);
    }
    printf("\n");
}

void set_sock_rwopt(const int sockFd, int optname, int value)
{
    int ret = 0;
    int optval = 0;
    int optlen = 0;

    ret = getsockopt(sockFd, SOL_SOCKET, optname, &optval, (socklen_t*)&optlen);
    if (ret < 0)
    {
        perror("IPEAD:set_sockopt():getsockopt() error");
        return;
    }
    printf("IPEAD:set_sockopt():before getsockopt %s=%d\n", RWOPT2STR(optname), optval);

    optval = value;
    optlen = sizeof(optval);
    ret = setsockopt(sockFd, SOL_SOCKET, optname, &optval, (socklen_t)optlen);
    if (ret < 0)
    {
        perror("IPEAD:set_sockopt():setsockopt() error");
        return;
    }
    printf("IPEAD:set_sockopt():setsockopt %s=%d\n", RWOPT2STR(optname), optval);

    ret = getsockopt(sockFd, SOL_SOCKET, optname, &optval, (socklen_t*)&optlen);
    if (ret < 0)
    {
        perror("IPEAD:set_sockopt():getsockopt() error");
        return;
    }

    printf("IPEAD:set_sockopt():after getsockopt %s=%d\n", RWOPT2STR(optname), optval);
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
    set_sock_rwopt(sockFd, SO_RCVBUF, 1024 * 1024 * 32);
    set_sock_rwopt(sockFd, SO_SNDBUF, 1024 * 1024 * 32);
    return RET_SUCCESS;
}

/*
函数名称：inline int active_dev( const int sockFd, const char * dev, int *index)
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
static int active_dev(const int sockFd, const char* dev, int *index)
{
    int     ret;
    struct  ifreq req;
    struct sockaddr_ll sll;

    bzero(&req, sizeof(struct ifreq));
    bzero(&sll, sizeof(struct sockaddr_ll));

    strncpy(req.ifr_name, dev, sizeof(req.ifr_name));

    ret = ioctl(sockFd, SIOCGIFFLAGS, &req);
    if (ret == -1)
    {
        perror("ioctl:SIOCGIFFLAGS");
        return(RET_FAIL);
    }

    req.ifr_flags |= IFF_UP;
    req.ifr_flags |= IFF_PROMISC;

    ret = ioctl(sockFd, SIOCSIFFLAGS, &req);
    if (ret == -1)
    {
        perror("ioctl:SIOCSIFFLAGS");
        return(RET_FAIL);
    }

    ret = ioctl(sockFd, SIOCGIFINDEX, &req);
    if (ret == -1)
    {
        perror("ioctl:SIOCGIFINDEX");
        return(RET_FAIL);
    }

    printf("****   if name %s, if index %d   *****\n", req.ifr_name, req.ifr_ifindex);
    *index  = req.ifr_ifindex;

    sll.sll_family = PF_PACKET;
    sll.sll_ifindex = *index;
    sll.sll_protocol = htons(ETH_P_ALL);
    if ((ret = bind(sockFd, (struct sockaddr *)&sll, sizeof(sll))) == -1)
    {
        perror("bind: ");
        exit(-1);
    }
    printf("socket %d bind %s success index=%d ret = %d\n", sockFd, dev, *index, ret);

    return(RET_SUCCESS);
}


void open_one_socket(int devInd)
{
    int ret = 0;

    gSock_fds[devInd] = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (gSock_fds[devInd] < 0)
    {
        printf("IPEAD:open_socket():gSock_fds[%d] cant get SOCK_PACKET socket", devInd);
        exit(0);
    }
    ret = active_dev(gSock_fds[devInd], ETH_NIC_DEVS[devInd], &g_eth_index[devInd]);
    if (ret == RET_FAIL)
    {
        close(gSock_fds[devInd]);
        printf("IPEAD:open_socket():can't active device%d:", devInd);
        exit(0);
    }
    printf("open_socket gSock_fds[%d] %d\n", devInd, gSock_fds[devInd]);
    set_sockopt(gSock_fds[devInd]);

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
    open_one_socket(NIC1_NIC_IND);
    open_one_socket(NIC2_NIC_IND);
//    open_one_socket(NIC3_NIC_IND);
//    open_one_socket(NIC4_NIC_IND);
}


/*
    函数名称：int recv_via_dev( const int sockFd, char * data, const
int len)
    功能说明：封装的接收函数
    输入参数：
        sockFd:套接字描述符
        data:接收的数据
        Len：接收数据的长度
    输出参数：无
    返回值：int 成功返回1，失败返回0
    关系函数：无
    线程安全：是
*/
int recv_via_dev(int *devInd, char* data, int len)
{
    int ret = 0;
    int i = 0;
    struct sockaddr_ll sa;
    socklen_t sz = sizeof(sa);

#ifdef DEBUG_IO
    alarm_log(DBG_MSG, MODULE_SOCKET, "IPEAD:recv_via_dev():recvfrom() begin...\n");
    memset(&sa, 0, sizeof(sa));
#endif
    ret = recvfrom(gSock_fds[*devInd], data, len, 0, (struct sockaddr*)&sa, &sz);   //function defined in system
    if (ret == -1)
    {
        return -1;
    }
    //printf("devInd is %d, sa.sll_ifindex is %d\n", *devInd, sa.sll_ifindex);
    if (g_eth_index[*devInd] != sa.sll_ifindex)
    {
        for (i = NIC1_NIC_IND; i < NIC5_NIC_IND; i++)
        {
            if (g_eth_index[i] == sa.sll_ifindex)
            {
                printf("eth%d recv_via_dev recv eth%d pack!!!!\n", *devInd, i);
                *devInd = i;
                break;
            }
        }
        if (i == NIC5_NIC_IND)
        {
            printf("devInd is %d, sa.sll_ifindex is %d not in list, ret = %d\n", *devInd, sa.sll_ifindex, ret);
            return -1;
        }
    }
    return ret;
}


/*
    函数名称：int send_via_dev(int devInd, char * & data,
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
int send_via_dev(int devInd, char* data, int len)
{
    int ret = 0;
    struct sockaddr_ll sa;

    sa.sll_family = AF_INET;
    memcpy(sa.sll_addr, data, 6);
    sa.sll_halen = 6;

    sa.sll_ifindex = g_eth_index[devInd];

    ret = sendto(gSock_fds[devInd], data, len, 0, (struct sockaddr*)&sa, sizeof(sa));
    if (ret < 0)
    {
        perror("send_via_dev");
        if (errno != EINTR)
        {
            printf("IPEAD:send_via_dev(%d, data, %d):sendto() error:\n", devInd, len);
            return(RET_FAIL);
        }
        printf("IPEAD:send_via_dev():sendto() error11111111:\n");
    }

    return ret;
}

void SET_CPU(unsigned int cpu_id)
{
    cpu_set_t cpuset;
    int ret;

    printf("worker: Starting cpu(%d)\n",cpu_id);

    /* Set CPU affinity */
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret)
    {
        fprintf(stderr, "(%d): Fail: pthread_setaffinity_np()\n", cpu_id);
    }

    return;
}


void set_Thread_High_Priority(int value)
{
	/* Start out with a standard, low-priority setup for the sched params.*/
	struct sched_param sp;
	bzero((void*)&sp, sizeof(sp));
	int policy = SCHED_OTHER;

	/* If desired, set up high-priority sched params structure.*/
	if (value) {

		/* FIFO scheduler, ranked above default SCHED_OTHER queue */
		policy = SCHED_FIFO;

		/* The priority only compares us to other SCHED_FIFO threads, so we
	 	* just pick a random priority halfway between min & max.
		*/
		//const int priority = (sched_get_priority_max(policy) + sched_get_priority_min(policy)) / 2;

		const int priority = sched_get_priority_max(policy);
		sp.sched_priority = priority;
	}

	/* Actually set the sched params for the current thread.*/
	if (0 == pthread_setschedparam(pthread_self(), policy, &sp)) {
		printf("IO Thread #%d using high-priority scheduler!\n", pthread_self());
	}
}

static void *Station_Thread(void * arg)
{
    char data[1500] = {0};
    int devInd = 0;
    int recvLen = 0;
    struct ethhdr *     eth;
    char pname[128] = {0};

    printf("station pthread_self is %d, gettid is %d\n", pthread_self(), getpid());
    sprintf(pname, "Station_%ld", getpid());
    prctl(PR_SET_NAME, pname);

	/* bind thread on CPU3, and high priority to avoid preemption */
    SET_CPU(3);
	set_Thread_High_Priority(1);
    
    while (1)
    {
        devInd = NIC1_NIC_IND;
        recvLen = recv_via_dev(&devInd, data, 1500);
        if (recvLen >= 0)
        {
            g_num[0][G_NUM_IP]++;
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
            send_via_dev(devInd, data, recvLen);
        }
    }
    return NULL;
}

static void *Terminal_Thread(void * arg)
{
    char data[1500] = {0};
    int devInd = 0;
    int recvLen = 0;
    struct ethhdr *     eth;
    char pname[128] = {0};

    printf("terminal pthread_self is %d, gettid is %d\n", pthread_self(), getpid());
    sprintf(pname, "Terminal_%ld", getpid());
    prctl(PR_SET_NAME, pname);

	/* bind thread on CPU2, and high priority to avoid preemption */
    SET_CPU(2);
	set_Thread_High_Priority(1);

    while (1)
    {
        devInd = NIC2_NIC_IND;
        recvLen = recv_via_dev(&devInd, data, 1500);
        if (recvLen >= 0)
        {
            g_num[1][G_NUM_IP]++;
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
            send_via_dev(devInd, data, recvLen);
        }
    }
    return NULL;
}

int main(void)
{
    int sockFd = open_socket();
    pthread_t        stationPthread;
    pthread_t        terminalPthread;
    int count = 0;

    pthread_create(&stationPthread, NULL, Station_Thread, NULL);
    pthread_create(&terminalPthread, NULL, Terminal_Thread, NULL);
    while (1)
    {
        if ((++count) % 30 == 0)
        {
            printgnum(G_NUM_IP, G_NUM_IP+1);
            count = 0;
        }
        sleep(1);
        fflush(stdout);
        fflush(stderr);
    }

    return 0;
}

