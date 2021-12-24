 /**********************************************************************
 * 文件名称： can_msg.h
 * 功能描述： CAN报文结构体定义
 * 文件目的： 此文件是定义CAN接收和发送报文的结构体，
 *            从STM32的代码中copy过来，用于应用层的发送和接收报文的定义。
 * 修改日期             版本号        修改人           修改内容
 * -----------------------------------------------
 * 2020/05/13         V1.0             bert            创建
 ***********************************************************************/
 #ifndef _CAN_MSG_H
 #define _CAN_MSG_H
 
 
 /**
 * 数据类型宏定义
 */
 typedef unsigned           char uint8_t;
 typedef unsigned short     int uint16_t;
 typedef unsigned           int uint32_t;
 
 /**
 * CAN报文ID类型
 */
 #define CAN_Id_Standard             ((uint32_t)0x00000000)  /* 标准帧Standard Id */
 #define CAN_Id_Extended             ((uint32_t)0x00000004)  /* 扩展帧Extended Id */
 
 #define CAN_ID_STD                  CAN_Id_Standard           
 #define CAN_ID_EXT                  CAN_Id_Extended
 
 /**
 * CAN报文类型
 */
 #define CAN_RTR_Data                ((uint32_t)0x00000000)  /* 数据帧 Data frame */
 #define CAN_RTR_Remote              ((uint32_t)0x00000002)  /* 远程帧 Remote frame */
 
 #define CAN_RTR_DATA     CAN_RTR_Data         
 #define CAN_RTR_REMOTE   CAN_RTR_Remote
 
 /** 
 * @brief  CAN Tx message structure definition  
 * CAN发送报文结构定义
 */
 typedef struct
 {
   uint32_t StdId;  /* CAN标准帧ID，占11bit，范围：0~0x7FF */ 
   uint32_t ExtId;  /* CAN扩展帧ID，占29bit，范围：0~0x1FFFFFFF */ 
   uint8_t IDE;     /* CAN报文ID类型，CAN_ID_STD 或 CAN_ID_EXT */
   uint8_t RTR;     /* CAN报文类型，CAN_RTR_DATA 或 CAN_RTR_REMOTE */ 
   uint8_t DLC;     /* CAN报文数据长度， 范围：0~8  */ 
   uint8_t Data[8]; /* CAN报文数据内容，每个字节范围：0~0xFF*/
 } CanTxMsg;
 
 /** 
 * @brief  CAN Rx message structure definition  
 * CAN接收报文结构定义
 */
 typedef struct
 {
   uint32_t StdId;  /* CAN标准帧ID，范围：0~0x7FF */ 
   uint32_t ExtId;  /* CAN扩展帧ID，范围：0~0x1FFFFFFF */ 
   uint8_t IDE;     /* CAN报文ID类型，CAN_ID_STD 或 CAN_ID_EXT */
   uint8_t RTR;     /* CAN报文类型，CAN_RTR_DATA 或 CAN_RTR_REMOTE */ 
   uint8_t DLC;     /* CAN报文数据长度， 范围：0~8  */ 
   uint8_t Data[8]; /* CAN报文数据内容，每个字节范围：0~0xFF*/
   uint8_t FMI;     /* 过滤模式，总共有14中，定义有宏，其值依次为0x1,0x2,0x4,0x8,0x10,0x20,0x40……,此处可以不考虑，忽略*/
 } CanRxMsg;
 
 
 #endif
 
