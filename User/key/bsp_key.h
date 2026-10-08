#ifndef __KEY_H
#define	__KEY_H

#include "stm32f4xx.h"

//引脚定义
/*******************************************************/
#define KEY1_GPIO_CLK			RCC_APB2Periph_GPIOE
#define KEY1_GPIO_PORT		GPIOE
#define KEY1_GPIO_PIN			GPIO_Pin_2

#define KEY2_GPIO_CLK			RCC_APB2Periph_GPIOE
#define KEY2_GPIO_PORT		GPIOE
#define KEY2_GPIO_PIN			GPIO_Pin_3

#define LEFT1_GPIO_CLK		RCC_APB2Periph_GPIOE
#define LEFT1_GPIO_PORT		GPIOE
#define LEFT1_GPIO_PIN		GPIO_Pin_5

#define LEFT2_GPIO_CLK		RCC_APB2Periph_GPIOC
#define LEFT2_GPIO_PORT		GPIOC
#define LEFT2_GPIO_PIN		GPIO_Pin_4

#define LEFT3_GPIO_CLK		RCC_APB2Periph_GPIOE
#define LEFT3_GPIO_PORT		GPIOE
#define LEFT3_GPIO_PIN		GPIO_Pin_2

#define LEFT4_GPIO_CLK		RCC_APB2Periph_GPIOE
#define LEFT4_GPIO_PORT		GPIOE
#define LEFT4_GPIO_PIN		GPIO_Pin_3

#define LEFT5_GPIO_CLK		RCC_APB2Periph_GPIOE
#define LEFT5_GPIO_PORT		GPIOE
#define LEFT5_GPIO_PIN		GPIO_Pin_4
/*******************************************************/

 /** 按键按下标置宏
	* 按键按下为高电平，设置 KEY_ON=1， KEY_OFF=0
	* 若按键按下为低电平，把宏设置成KEY_ON=0 ，KEY_OFF=1 即可
	*/
#define KEY_ON	1
#define KEY_OFF	0

void Key_GPIO_Config(void);
void Key_Tick(void);
uint8_t Key_GetNum(void);

#endif /* __LED_H */

