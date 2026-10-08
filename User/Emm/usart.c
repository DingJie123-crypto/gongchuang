#include "usart.h"

/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

__IO bool rxFrameFlag = false;
__IO uint8_t rxCmd[FIFO_SIZE] = {0};
__IO uint8_t rxCount = 0;

__IO bool rxFrameFlag3 = false;
__IO uint8_t rxCmd3[FIFO_SIZE] = {0};
__IO uint8_t rxCount3 = 0;
/**
	* @brief   UART4中断函数
	* @param   无
	* @retval  无
	*/
void USART2_IRQHandler(void)
{
	__IO uint16_t i = 0;

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
	{
		fifo_enQueue((uint8_t)USART2->DR);
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
	}

	else if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		USART2->SR; USART2->DR;
		rxCount = fifo_queueLength(); for(i=0; i < rxCount; i++) { rxCmd[i] = fifo_deQueue(); }
		rxFrameFlag = true;
	}
}

/**
	* @brief   USART发送多个字节
	* @param   无
	* @retval  无
	*/
void usart_SendCmd(__IO uint8_t *cmd, uint8_t len)
{
	__IO uint8_t i = 0;
	__IO uint16_t t0 = 0;
	
	for(i=0; i < len; i++) { usart_SendByte(cmd[i]); }
	
	while(!(USART2->SR & USART_FLAG_TC))
	{
		++t0; if(t0 > 8000) { return; }
	}
}

/**
	* @brief   USART发送一个字节
	* @param   无
	* @retval  无
	*/
void usart_SendByte(uint16_t data)
{
	__IO uint16_t t0 = 0;
	
	USART2->DR = (data & (uint16_t)0x01FF);

	while(!(USART2->SR & USART_FLAG_TXE))
	{
		++t0; if(t0 > 8000)	{	return; }
	}
}

void USART3_IRQHandler(void)
{
	__IO uint16_t i = 0;

	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		fifo_enQueue3((uint8_t)USART3->DR);
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}

	else if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		USART3->SR; USART3->DR;
		rxCount3 = fifo_queueLength3(); for(i=0; i < rxCount3; i++) { rxCmd3[i] = fifo_deQueue3(); }
		rxFrameFlag3 = true;
	}
}

void usart_SendCmd3(__IO uint8_t *cmd, uint8_t len)
{
	__IO uint8_t i = 0;
	__IO uint16_t t0 = 0;
	
	for(i=0; i < len; i++) { usart_SendByte3(cmd[i]); }
	
	while(!(USART3->SR & USART_FLAG_TC))
	{
		++t0; if(t0 > 8000) { return; }
	}
}

void usart_SendByte3(uint16_t data)
{
	__IO uint16_t t0 = 0;
	
	USART3->DR = (data & (uint16_t)0x01FF);

	while(!(USART3->SR & USART_FLAG_TXE))
	{
		++t0; if(t0 > 8000)	{	return; }
	}
}

