/**********************************************************************
* 文件名称： can_controller.c
* 功能描述： CAN抽象结构体框架的具体实现函数
*            主要实现调用硬件驱动的接口封装
* 文件目的： 掌握CAN底层驱动的抽象
* 修改日期             版本号        修改人           修改内容
* -----------------------------------------------
* 2020/05/13         V1.0             bert            创建
***********************************************************************/

/**************头文件**************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include <fcntl.h>
#include <pthread.h>

#include "can_controller.h"

/**************宏定义**************************************************/


//#define CAN_SFF_MASK 0x000007ffU

/**************全局变量定义*********************************************/
/*接收线程中应用层处理的指针函数*/
pCanInterrupt    g_pCanInterrupt = NULL;
/*接收线程线程ID*/
pthread_t ntid;

/**************函数声明*************************************************/
int register_can_controller(const pCAN_COMM_STRUCT p_can_controller);
void *CAN1_RX0_IRQHandler(void *arg);

/***********************************************************************
****************CAN控制器抽象接口实现代码*******************************
***********************************************************************/

/**********************************************************************
* 函数名称： int CAN_Set_Controller( void )
* 功能描述： CAN控制器初始化，包括GPIO(TX,RX），CAN外设时钟，波特率，过滤器等
* 输入参数： 无
* 输出参数： 无
* 返 回 值： int can_port:返回对应CAN控制器的通道号，类比socketcan中的套接口
* 修改日期             版本号        修改人           修改内容
* -----------------------------------------------
* 2020/05/13         V1.0             bert            创建
***********************************************************************/
int CAN_Set_Controller( char *can_ifname )
{    
    /*************************************************************/
    /*定义套接口变量： sock_fd*/
    int sock_fd;
    /**/
    struct sockaddr_can addr;
    /**/
    struct ifreq ifr;

    /*************************************************************/
    /* 通过system调用ip命令设置CAN波特率 */

	char cmd[64];
	memset(cmd, 0 , sizeof(cmd));
	sprintf(cmd, "ifconfig %s down;ip link set %s type can bitrate 500000 loopback off;ifconfig %s up",can_ifname,can_ifname,can_ifname );	
    system(cmd);               
    
    /*************************************************************/
    /* 创建套接口 sock_fd */
    sock_fd = socket(AF_CAN, SOCK_RAW, CAN_RAW);
	if(sock_fd < 0)
	{
		perror("socket create error!\n");
		return -1;
	}
    
    /*************************************************************/
    //将套接字与 can0 绑定
    strcpy(ifr.ifr_name, can_ifname);
	ioctl(sock_fd, SIOCGIFINDEX,&ifr); // can0 device

	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);
	printf("ifr_name:%s \n",ifr.ifr_name);
	printf("can_ifindex:%d \n",ifr.ifr_ifindex);

 	addr.can_family = AF_CAN;
 	addr.can_ifindex = ifr.ifr_ifindex;
 		
 	if( bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 )
 	{
 		perror("bind error!\n");
 		return -1;
 	}
 	
 	/*************************************************************/
 	//定义接收规则，只接收表示符等于 0x201 的报文
 	struct can_filter rfilter[1];
 	rfilter[0].can_id = 0x201;
 	rfilter[0].can_mask = CAN_SFF_MASK;
 	//设置过滤规则
 	setsockopt(sock_fd, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));
 
     /*************************************************************/
     //设置read()和write()函数设置为非堵塞方式
     int flags;
     flags = fcntl(sock_fd, F_GETFL);
     flags |= O_NONBLOCK;
     fcntl(sock_fd, F_SETFL, flags);  
 
     /*************************************************************/
     /*返回套接口*/
     return sock_fd;  
     /*************************************************************/
 }
 
 
 /**********************************************************************
 * 函数名称： void CAN_Set_Interrupt(int can_port,  pCanInterrupt callback)
 * 功能描述： 创建CAN接收线程，并传入应用的的回调函数，回调函数主要处理应用层的功能
 * 输入参数： can_port,端口号
 *            callback： 中断具体处理应用功能的回调函数
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void CAN_Set_Interrupt(int can_port,  pCanInterrupt callback)
 {
     int err;
     
     if ( NULL != callback ) 
     {
         g_pCanInterrupt = callback;
     }
     
     err = pthread_create(&ntid, NULL,CAN1_RX0_IRQHandler, NULL );
     if( err !=0 )
     {
         printf("create thread fail! \n");
         return ;
     }
     printf("create thread success!\n");
     
 
     return ;
 }
 
 
 
 /**********************************************************************
 * 函数名称： void CAN_Read(int can_port, CanRxMsg* recv_msg)
 * 功能描述： CAN读取接收寄存器，取出接收到的报文
 * 输入参数： can_port,端口号     
 * 输出参数： recv_msg：接收报文
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void CAN_Read(int can_port, CanRxMsg* recv_msg)
 { 
     unsigned char i;
     static unsigned int rxcounter =0;
     
     int nbytes;
     struct can_frame rxframe;
     
    nbytes = read(can_port, &rxframe, sizeof(struct can_frame));
 	if(nbytes>0)
 	{
 	    //printf("nbytes = %d \n",nbytes );
 	    
 	    recv_msg->StdId = rxframe.can_id;
 	    recv_msg->DLC = rxframe.can_dlc;
 	    memcpy( recv_msg->Data, &rxframe.data[0], rxframe.can_dlc);
 	    
 		rxcounter++;

		char buf[256],cmd[256];
		memset(buf, 0, sizeof(buf));
		memset(cmd, 0, sizeof(cmd));
		
 		sprintf(buf, "rxcounter=%d, ID=%03X, DLC=%d, data=%02X %02X %02X %02X %02X %02X %02X %02X",  \
 			rxcounter,
 			rxframe.can_id, rxframe.can_dlc,  \
 			rxframe.data[0],\
 			rxframe.data[1],\
 			rxframe.data[2],\
 			rxframe.data[3],\
 			rxframe.data[4],\
 			rxframe.data[5],\
 			rxframe.data[6],\
 			rxframe.data[7] );

		printf("%s\n",buf);

		sprintf(cmd, "echo \"%s\" >> /home/root/can_rx.log",buf);
		system(cmd);
			
 	}
 
     return ;
 }
  
 /**********************************************************************
 * 函数名称： void CAN_Write(int can_port, CanTxMsg send_msg)
 * 功能描述： CAN报文发送接口，调用发送寄存器发送报文
 * 输入参数： can_port,端口号     
 * 输出参数： send_msg：发送报文
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void CAN_Write(int can_port, CanTxMsg send_msg)
 {
     unsigned char i;
     static unsigned int txcounter=0;
     int nbytes;
     
     struct can_frame txframe;
     
     txframe.can_id = send_msg.StdId;
     txframe.can_dlc = send_msg.DLC;
     memcpy(&txframe.data[0], &send_msg.Data[0], txframe.can_dlc);
 
     nbytes = write(can_port, &txframe, sizeof(struct can_frame)); //发送 frame[0]
 	
 	if(nbytes == sizeof(txframe))
 	{
 	    txcounter++;


		char buf[256],cmd[256];
		memset(buf, 0, sizeof(buf));
		memset(cmd, 0, sizeof(cmd));


		
 	    sprintf(buf,"txcounter=%d, ID=%03X, DLC=%d, data=%02X %02X %02X %02X %02X %02X %02X %02X",  \
 			txcounter,
 			txframe.can_id, txframe.can_dlc,  \
 			txframe.data[0],\
 			txframe.data[1],\
 			txframe.data[2],\
 			txframe.data[3],\
 			txframe.data[4],\
 			txframe.data[5],\
 			txframe.data[6],\
 			txframe.data[7] );

		printf("%s\n",buf);
		
		sprintf(cmd, "echo \"%s\" >> /home/root/can_tx.log",buf);
		system(cmd);
			
     }
     else
 	{
 		//printf("Send Error frame[0], nbytes=%d\n!",nbytes);
 	}
 
     return ;
 }
 
 /**********************************************************************
 * 函数名称： void CAN1_RX0_IRQHandler(void)
 * 功能描述： CAN接收中断函数
 * 输入参数： 无  
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void *CAN1_RX0_IRQHandler(void *arg)
 {
     /* 接收报文定义 */
     while( 1 )
     {
     /* 如果回调函数存在，则执行回调函数 */
         if( g_pCanInterrupt != NULL)
         {
             g_pCanInterrupt();
         }
         usleep(10000);
     }
 }
 
 
 
 /**********************************************************************
 * 名称：     can1_controller
 * 功能描述： CAN1结构体初始化
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
  CAN_COMM_STRUCT can0_controller = {
	.name                   = "can0",
	.can_port               = -1,//默认，应用层传入此参数
	.can_set_controller     = CAN_Set_Controller,
	.can_set_interrput      = CAN_Set_Interrupt,
	.can_read               = CAN_Read,
	.can_write              = CAN_Write, 
 };


   CAN_COMM_STRUCT can1_controller = {
	.name                   = "can1",
	.can_port               = -1,//默认，应用层传入此参数
	.can_set_controller     = CAN_Set_Controller,
	.can_set_interrput      = CAN_Set_Interrupt,
	.can_read               = CAN_Read,
	.can_write              = CAN_Write, 
 };
 
 /**********************************************************************
 * 函数名称： void CAN1_contoller_add(void)
 * 功能描述： CAN结构体注册接口，应用层在使用can1_controller前调用
 * 输入参数： 无  
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void CAN_contoller_add(int cannum)
 {
     /*将can1_controller传递给应用层*/
 	if( cannum == 0)
   	  register_can_controller( &can0_controller );
	if( cannum == 1)
   	  register_can_controller( &can1_controller );
 }
 
 
 /***********************************************************************
 ****************End Of File*********************************************
 ***********************************************************************/
 
