#ifndef __USART_H
#define __USART_H

#include "board.h"
#include "fifo.h"

/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

extern __IO bool rxFrameFlag;
extern __IO uint8_t rxCmd[FIFO_SIZE];
extern __IO uint8_t rxCount;

void usart_SendCmd(__IO uint8_t *cmd, uint8_t len);
void usart_SendByte(uint16_t data);
void usart_SendCmd3(__IO uint8_t *cmd, uint8_t len);
void usart_SendByte3(uint16_t data);

//// 单轴数据最大缓存长度
//#define RV_MAX_LEN 10

//// 外部全局变量声明
//extern volatile uint8_t rv_data_ready;
//extern int16_t target_x;
//extern int16_t target_y;
//extern int16_t target_z;

//// 函数声明
//void Uart5_en_Init(void);// UART5串口初始化（标准库）
//void Uart5_ProcessData(void);// 解析串口接收的三轴数据
//int16_t Uart5_getdata(uint8_t choosexy);// 获取对应轴目标数据

#endif
