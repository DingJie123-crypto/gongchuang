/**
  ******************************************************************************
  * @file    bsp_key.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   按键应用bsp（扫描模式）
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火  STM32 F407 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
  
#include "bsp_key.h" 

/// 不精确的延时
void Key_Delay(__IO u32 nCount)
{
	for(; nCount != 0; nCount--);
} 

/**
  * @brief  配置按键用到的I/O口
  * @param  无
  * @retval 无
  */
void Key_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitTypeDefstruct;
	//Enable GPIOE/GPIOC/GPIOA clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	//Init IO as input
	GPIO_InitTypeDefstruct.GPIO_Mode=GPIO_Mode_IN;

	GPIO_InitTypeDefstruct.GPIO_Pin=GPIO_Pin_4;
	GPIO_InitTypeDefstruct.GPIO_PuPd=GPIO_PuPd_UP;
	GPIO_InitTypeDefstruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOC,&GPIO_InitTypeDefstruct);
//	GPIO_InitTypeDefstruct.GPIO_Pin=GPIO_Pin_9|GPIO_Pin_10;
//	GPIO_InitTypeDefstruct.GPIO_PuPd=GPIO_PuPd_UP;
//	GPIO_InitTypeDefstruct.GPIO_Speed=GPIO_Speed_50MHz;
//	GPIO_Init(GPIOA,&GPIO_InitTypeDefstruct);
	GPIO_InitTypeDefstruct.GPIO_Pin=GPIO_Pin_3|GPIO_Pin_2|GPIO_Pin_5;
	GPIO_InitTypeDefstruct.GPIO_PuPd=GPIO_PuPd_UP;
	GPIO_InitTypeDefstruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOE,&GPIO_InitTypeDefstruct);
}

uint8_t Key_GetState(void)
{
	if (GPIO_ReadInputDataBit(KEY1_GPIO_PORT, KEY1_GPIO_PIN) == 0)
	{
		return 1;
	}
	else	if (GPIO_ReadInputDataBit(KEY2_GPIO_PORT, KEY2_GPIO_PIN) == 0)
	{
		return 2;
	}
	else	if (GPIO_ReadInputDataBit(LEFT1_GPIO_PORT, LEFT1_GPIO_PIN) == 0)
	{
		return 3;
	}
	else	if (GPIO_ReadInputDataBit(LEFT2_GPIO_PORT, LEFT2_GPIO_PIN) == 0)
	{
		return 4;
	}
//	else	if (GPIO_ReadInputDataBit(LEFT3_GPIO_PORT, LEFT3_GPIO_PIN) == 0)
//	{
//		return 5;
//	}
//	else	if (GPIO_ReadInputDataBit(LEFT4_GPIO_PORT, LEFT4_GPIO_PIN) == 0)
//	{
//		return 6;
//	}
	return 0;
}

volatile uint8_t Key_Num = 0;
void Key_Tick(void)
{
	static uint8_t last = 0,now = 0;
//	static uint8_t Count = 0;
//	Count += 1;
//	if(Count >= 20)
//	{
//		Count = 0;
		last = now;
		now = Key_GetState();
		if(last != 0 && now == 0)
		{
			Key_Num = last;
		}
	//}
}

uint8_t Key_GetNum(void)
{
	uint8_t Temp = 0;
	if(Key_Num)
	{
		Temp = Key_Num;
		Key_Num = 0;
		return Temp;
	}
	return 0;
}

