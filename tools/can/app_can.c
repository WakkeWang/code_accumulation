/**********************************************************************
* 文件名称： app_can.c
* 功能描述： CAN应用层代码，实现功能如下：
*            功能1：周期发送CAN报文；
*            功能2：接收报文，并转发报文
* 文件目的： 掌握CAN报文的基本发送和接收处理
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

#include "app_can.h"
#include "can_controller.h"


/**************宏定义**************************************************/
/* 本例程中测试周期发送的CAN报文ID */
#define TX_CAN_ID           0X201   

/* 本例程中测试接收的CAN报文ID */
#define RX_CAN_ID           0x201   

/* 本例程中接收到测试报文为RX_CAN_ID的报文，将ID改为RX_TO_TX_CAN_ID为ID的报文转发出去 */
#define RX_TO_TX_CAN_ID     0X301   


/**************全局变量定义*********************************************/
/** 
* CAN应用层调用结构体指针变量
*/
static CAN_COMM_STRUCT gCAN_COMM_STRUCT;

/** 
*CAN应用层接收CAN报文后存储的CAN消息
*/
static CanRxMsg  g_CAN1_Rx_Message;

/** 
*CAN应用层接收CAN报文标志，处理完后清零
*/
static unsigned char g_CAN1_Rx_Flag =0; 


/**************函数声明*************************************************/
/** 
*CAN中断中回调函数，在MCU中中断处理函数有特定的函数定义，
*此处将中断函数放在can_controller.c中，将中断中的具体处理内容放到app_can.c应用中，
*此处作为回调是为了与linux socket编程中类比，在linux socket中使用回调传入的是接收线程函数
*/
void CAN_RX_IRQHandler_Callback(void);




/***********************************************************************
****************应用层代码**********************************************
***********************************************************************/

/**********************************************************************
* 函数名称： int register_can_controller(const pCAN_COMM_STRUCT p_can_controller)
* 功能描述： 应用层进行CAN1结构体注册
* 输入参数： p_can_controller，CAN控制器抽象结构体
* 输出参数： 无
* 返 回 值： 无
* 修改日期             版本号        修改人           修改内容
* -----------------------------------------------
* 2020/05/13         V1.0             bert            创建
***********************************************************************/
int register_can_controller(const pCAN_COMM_STRUCT p_can_controller)
{
    /* 判断传入的p_can_controller为非空，目的是确认这个结构体是实体*/
    if( p_can_controller != NULL )
    {
        /* 将传入的参数p_can_controller赋值给应用层结构体gCAN_COMM_STRUCT */
        
        /*端口号，类比socketcan套接口*/
        gCAN_COMM_STRUCT.can_port              = p_can_controller->can_port; 
        /*CAN控制器配置函数*/
        gCAN_COMM_STRUCT.can_set_controller    = p_can_controller->can_set_controller; 
        /*CAN中断配置*/
        gCAN_COMM_STRUCT.can_set_interrput     = p_can_controller->can_set_interrput;
        /*CAN报文读函数*/
        gCAN_COMM_STRUCT.can_read              = p_can_controller->can_read;
        /*CAN报文发送函数*/
        gCAN_COMM_STRUCT.can_write             = p_can_controller->can_write;
         return 1;
     }
 	return 0;
 }
 
 /**********************************************************************
 * 函数名称： void app_can_init(void)
 * 功能描述： CAN应用层初始化
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void app_can_init(int can_num)
 {
     /** 
     * 应用层进行CAN1结构体注册
     */
     CAN_contoller_add(can_num);
     
     /*
     *调用can_set_controller进行CAN控制器配置，
     *返回can_port，类比linux socketcan中的套接口，单片机例程中作为自定义CAN通道 
     */

	 if(can_num == 0)
     	gCAN_COMM_STRUCT.can_port = gCAN_COMM_STRUCT.can_set_controller("can0");
	 else if(can_num == 1)
		 gCAN_COMM_STRUCT.can_port = gCAN_COMM_STRUCT.can_set_controller("can1");

	 
     /** 
     * 调用can_set_interrput配置CAN接收中断，类比socketcan中的接收线程
     */
     gCAN_COMM_STRUCT.can_set_interrput( gCAN_COMM_STRUCT.can_port, CAN_RX_IRQHandler_Callback );
 }
 
 
 /**********************************************************************
 * 函数名称： void app_can_tx_test(void)
 * 功能描述： CAN应用层报文发送函数，用于测试周期发送报文
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void app_can_tx_test(void)
 {
     // 以10ms为基准，运行CAN测试程序
     
     unsigned char i=0;
     
     /* 发送报文定义 */
     CanTxMsg TxMessage;
     
     /* 发送报文中用一个字节来作为计数器 */
     static unsigned char tx_counter = 0;
     
     /* 以10ms为基准，通过timer计数器设置该处理函数后面运行代码的周期为1秒钟*/  
     static unsigned int timer =0;
     if(timer++>100)
     {
         timer = 0;
     }
     else
     {
         return ;
     }
     
     /* 发送报文报文数据填充，此报文周期是1秒 */
     TxMessage.StdId = TX_CAN_ID;	      /* 标准标识符为0x000~0x7FF */
     TxMessage.ExtId = 0x0000;             /* 扩展标识符0x0000 */
     TxMessage.IDE   = CAN_ID_STD;         /* 使用标准标识符 */
     TxMessage.RTR   = CAN_RTR_DATA;       /* 设置为数据帧  */
     TxMessage.DLC   = 8;                  /* 数据长度, can报文规定最大的数据长度为8字节 */
     
     /* 填充数据，此处可以根据实际应用填充 */
     TxMessage.Data[0] = tx_counter++;       /* 用来识别报文发送计数器 */
     for(i=1; i<TxMessage.DLC; i++)
     {
         TxMessage.Data[i] = i;            
     }
     
     /*  调用can_write发送CAN报文 */
     gCAN_COMM_STRUCT.can_write(gCAN_COMM_STRUCT.can_port, TxMessage);
     
 }
 
 
 /**********************************************************************
 * 函数名称： void app_can_rx_test(void)
 * 功能描述： CAN应用层接收报文处理函数，用于处理中断函数中接收的报文
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void app_can_rx_test(void)
 {
     unsigned char i=0;
     
     /* 发送报文定义 */
     CanTxMsg TxMessage;
     
     /* 发送报文中用一个字节来作为计数器 */
     static unsigned char rx_counter = 0;
       
     
     if( g_CAN1_Rx_Flag == 1)
     {
         g_CAN1_Rx_Flag = 0;
  #if 0       
         /* 发送报文报文数据填充，此报文周期是1秒 */
         TxMessage.StdId = RX_TO_TX_CAN_ID;	  /* 标准标识符为0x000~0x7FF */
         TxMessage.ExtId = 0x0000;             /* 扩展标识符0x0000 */
         TxMessage.IDE   = CAN_ID_STD;         /* 使用标准标识符 */
         TxMessage.RTR   = CAN_RTR_DATA;       /* 设置为数据帧  */
         TxMessage.DLC   = 8;                  /* 数据长度, can报文规定最大的数据长度为8字节 */
         
         /* 填充数据，此处可以根据实际应用填充 */
         TxMessage.Data[0] = rx_counter++;      /* 用来识别报文发送计数器 */
         for(i=1; i<TxMessage.DLC; i++)
         {
             TxMessage.Data[i] = g_CAN1_Rx_Message.Data[i];            
         }
         
         /*  调用can_write发送CAN报文 */
         gCAN_COMM_STRUCT.can_write(gCAN_COMM_STRUCT.can_port, TxMessage);
#endif
     }
 }
 
 /**********************************************************************
 * 函数名称： void CAN_RX_IRQHandler_Callback(void)
 * 功能描述： CAN1接收中断函数；在linux中可以类比用线程，或定时器去读CAN数据
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 void CAN_RX_IRQHandler_Callback(void)
 {
     /* 接收报文定义 */
     CanRxMsg RxMessage; 
     
     /* 接收报文清零 */
     memset( &RxMessage, 0, sizeof(CanRxMsg) );
    
     /* 通过can_read接口读取寄存器已经接收到的报文 */
     gCAN_COMM_STRUCT.can_read(gCAN_COMM_STRUCT.can_port, &RxMessage);
 
     /* 将读取到的CAN报文存拷贝到全局报文结构体g_CAN1_Rx_Message */
     memcpy(&g_CAN1_Rx_Message, &RxMessage, sizeof( CanRxMsg ) );
     
     /* 设置当前接收完成标志，判断当前接收报文ID为RX_CAN_ID，则设置g_CAN1_Rx_Flag=1*/
     if( g_CAN1_Rx_Message.StdId == RX_CAN_ID )
     {
         g_CAN1_Rx_Flag = 1;  
     }
 }
 
 /**********************************************************************
 * 函数名称： int main(int argc, char **argv)
 * 功能描述： 主函数
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 int main(int argc, char **argv)
 {
	if(argc != 3)
	{
		printf("usage:%s can0/can1 rx/tx\n",argv[0]);
		return 0;
	}

	
	if(!strncmp(argv[1], "can0", 4))
     	app_can_init(0);
	if(!strncmp(argv[1], "can1", 4))
     	app_can_init(1);


	printf("waiting for 10s...\n");
	sleep(10);

	if(!strncmp(argv[2], "tx", 2))
	{
		while(1)
		{
			/* CAN应用层周期发送报文 */
			app_can_tx_test();
						
			/* 利用linux的延时函数设计10ms的运行基准 */
			usleep(10000);
		}
	}
	 
	if(!strncmp(argv[2], "rx", 2))
	{
		while(1)
		{
			
			/* CAN接收报文周期触发处理 */
			app_can_rx_test();
			
			/* 利用linux的延时函数设计10ms的运行基准 */
			usleep(10000);
		}

	}

 }
 
 
 

