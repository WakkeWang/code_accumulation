/**********************************************************************
* 文件名称： app_controller.h
* 功能描述： CAN控制器接口实现头文件
* 文件目的： 主要进行函数声明
* 修改日期             版本号        修改人           修改内容
* -----------------------------------------------
* 2020/05/13         V1.0             bert            创建
***********************************************************************/
#ifndef _CAN_CONTROLLER_H
#define _CAN_CONTROLLER_H

/**************头文件**************************************************/
#include "can_msg.h"


/**************宏定义**************************************************/
/* NULL 定义*/
#ifndef NULL 
#define NULL  (void*)0
#endif

/* 指针类型定义 */
typedef void ( *pCanInterrupt ) ( void );

/* CAN端口号定义*/
enum
{
    CAN_PORT_NONE = 0,
    CAN_PORT_CAN1,
    CAN_PORT_CAN2,
    CAN_PORT_MAX
};

/* CAN通信抽象结构体定义*/
typedef struct _CAN_COMM_STRUCT
{
    /* CAN硬件名称 */
    char *name;
	
    /* CAN端口号，裸机里为端口号;linux应用里作为socket套接口 */
    int  can_port;                                
	
    /* CAN控制器配置函数，返回端口号赋值给can_port */
    int  (*can_set_controller)(  char* can_ifname );                
	
    /* CAN接口中断创建，在linux中对应创建接收线程 */
    void (*can_set_interrput)( int can_port , pCanInterrupt callback ); 
	
    /* CAN读取报文接口 */
    void (*can_read)( int can_port , CanRxMsg* recv_msg);   
	
    /* CAN发送报文接口*/
    void (*can_write)( int can_port , CanTxMsg send_msg);   
	
}CAN_COMM_STRUCT, *pCAN_COMM_STRUCT;



/**************函数声明**************************************************/
extern int register_can_controller(const pCAN_COMM_STRUCT p_can_controller);
void CAN_contoller_add(int cannum);



#endif

/***********************************************************************
****************End Of File*********************************************
***********************************************************************/
