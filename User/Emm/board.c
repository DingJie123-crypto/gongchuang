#include "board.h"

/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

/**
	* @brief   配置NVIC控制器
	* @param   无
	* @retval  无
	*/
void nvic_init(void)
{	
	// 4bit抢占优先级位
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_Init(&NVIC_InitStructure);
}

/**
	*	@brief		外设时钟初始化
	*	@param		无
	*	@retval		无
	*/
void clock_init(void)
{
	// 使能GPIOC外设时钟（UART4引脚PC10 PC11）
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	// 使能UART4外设时钟（UART4挂载APB1）
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}
/**
	* @brief   初始化UART4
	* @param   无
	* @retval  无
	*/
void usart_init(void)
{
/**********************************************************
***	初始化UART2引脚 PA2(TX) PA3(RX)
**********************************************************/
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// 不使用GPIO_PinAFConfig，直接写AFRH寄存器，PC10/PC11复用AF8(UART4)
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

/**********************************************************
***	初始化UART2 参数与原USART1保持一致：256000波特率
**********************************************************/
	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 921600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &USART_InitStructure);

/**********************************************************
***	清除USART2空闲/接收中断标志
**********************************************************/
	USART2->SR; USART2->DR;
	USART_ClearITPendingBit(USART2, USART_IT_RXNE);

/**********************************************************
***	使能USART2接收中断 + 空闲帧中断
**********************************************************/	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);

/**********************************************************
***	使能USART2外设
**********************************************************/
	USART_Cmd(USART2, ENABLE);
}

/**
	*	@brief		板载初始化
	*	@param		无
	*	@retval		无
	*/
void board_init(void)
{
	nvic_init();
	clock_init();
	usart_init();
}

extern QueueHandle_t Motor_Queue;

/**
 * @brief 电机脉冲清零函数
 * 将模式标记置3，选择电机为MOTOR_NONE，非阻塞入队消息
 */
void Motor_Pulse_Clear(MotorMsg_t* Motor_Speed,float motor_speed)//调用前需要延时2ms
{
	Motor_Speed->mode_flag = 3;
	Motor_Speed->motor_sel = MOTOR_NONE;
	// 0：非阻塞发送，队列满直接返回，不会卡住任务
	xQueueSend(Motor_Queue, Motor_Speed, 0);

}


/* 读取指定地址电机的实时脉冲数(S_CLKC)，应答存于rxCmd[]，成功返回对应数值 */
void Read_Motor_ClC(MotorMsg_t* Motor_Speed)
{
		//__disable_irq();
    rxFIFO.ptrRead = 0; rxFIFO.ptrWrite = 0;
    { volatile uint8_t d = USART2->SR; d = USART2->DR;/* (void)d; */}
    rxFrameFlag = false;
		//__enable_irq();
		
    if(Motor_Speed->stepleft1 != 0)
		{
			Motor_Speed->motor_sel = MOTOR_NONE;
			Motor_Speed->mode_flag = 2;
			xQueueSend( Motor_Queue, Motor_Speed, 0 );
		}
		else	if(Motor_Speed->stepright1 != 0)
		{
			Motor_Speed->motor_sel = MOTOR_1;
			Motor_Speed->mode_flag = 2;
			xQueueSend( Motor_Queue, Motor_Speed, 0 );
		}
		else
		{
			Motor_Speed->motor_sel = MOTOR_NONE;
			Motor_Speed->mode_flag = 2;
			xQueueSend( Motor_Queue, Motor_Speed, 0 );
		}
   // return res;
}

/**
 * @brief  设置麦轮小车运动模式
 * @param  msg: MotorMsg_t结构体指针
 * @param  mode: 运动模式 CarMoveMode_t
 * @param  speed: 运动速率(正数，建议200~800)，CAR_KEEP模式下此参数无效
 * @retval 无
 */
void Mecanum_SetMoveMode(MotorMsg_t* msg, CarMoveMode_t mode, int16_t speed)
{
    if(msg == (void *)0) return;
    msg->move_mode = mode;
		
		if(CAR_KEEP == mode)
		{
			// 保持结构体原有四个电机速度，不修改；不使用传入speed
            msg->motor_sel = MOTOR_NONE; // 依然重置轮询起点，继续下发当前速度指令
			return;
		}
    switch(mode)
    {
        case CAR_FORWARD:       // 前进：全部正转
            msg->motor_sel = MOTOR_NONE;
            msg->stepleft1  = speed;
            msg->stepright1 = speed;
            msg->stepleft2  = speed;
            msg->stepright2 = speed;
            break;

        case CAR_BACKWARD:      // 后退：全部反转
            msg->motor_sel = MOTOR_NONE;
            msg->stepleft1  = -speed;
            msg->stepright1 = -speed;
            msg->stepleft2  = -speed;
            msg->stepright2 = -speed;
            break;

        case CAR_MOVE_LEFT:     // 左平移
            msg->motor_sel = MOTOR_NONE;
						msg->stepleft1  = speed;
            msg->stepright1 = -speed;
            msg->stepleft2  = -speed;
            msg->stepright2 = speed;
            break;

        case CAR_MOVE_RIGHT:    // 右平移
            msg->motor_sel = MOTOR_NONE;
            msg->stepleft1  = -speed;
            msg->stepright1 = speed;
            msg->stepleft2  = speed;
            msg->stepright2 = -speed;
            break;
				case CAR_MOVE_LEFT_DOWN:// 左斜下平移
						msg->motor_sel = MOTOR_NONE;
						msg->stepleft1  = 0;
            msg->stepright1 = -speed;
            msg->stepleft2  = -speed;
            msg->stepright2 = 0;
					break;
				case CAR_MOVE_RIGHT_DOWN:// 右斜下平移
						msg->motor_sel = MOTOR_NONE;
            msg->stepleft1  = -speed;
            msg->stepright1 = 0;
            msg->stepleft2  = 0;
            msg->stepright2 = -speed;
					break;
        case CAR_STOP:          // 停止，全部速度置0
        default:
            msg->motor_sel = MOTOR_NONE;
            msg->stepleft1  = 0;
            msg->stepright1 = 0;
            msg->stepleft2  = 0;
            msg->stepright2 = 0;
            break;
    }
}


void step_motor(MotorMsg_t* Motor_Speed)
{
	if(Motor_Speed->motor_sel >= MOTOR_FINISH)
	{
		Emm_V5_MMCL_Stop_Now(0,false);
	}
	
	//Mecanum_SetMoveMode(Motor_Speed);
	switch(Motor_Speed->motor_sel)
	{
			case MOTOR_NONE:  // 0 右前轮
			{
					if(Motor_Speed->ctrl_reg == MOTOR_SEL_DOWN)
					{
						if(Motor_Speed->stepleft1 >= 0)
						{
								Emm_V5_Vel_Control(1, 1, Motor_Speed->stepleft1, 225, 0);
						}
						else
						{
								Emm_V5_Vel_Control(1, 0, -Motor_Speed->stepleft1, 225, 0);
						}
					}
					else
					{
						if(Motor_Speed->motor_clk.Clkleft1 >= 0)//机械臂(上下) 
						{
								Emm_V5_Pos_Control3(5, 1, 4000, 255, Motor_Speed->motor_clk.Clkleft1,true,0);
						}
						else
						{
								Emm_V5_Pos_Control3(5, 0, 4000, 255,-Motor_Speed->motor_clk.Clkleft1,true,0);
						}
					}
					break;
			}
			case MOTOR_1:     // 1 左前轮
			{
					if(Motor_Speed->ctrl_reg == MOTOR_SEL_DOWN)
					{
						if(Motor_Speed->stepright1 >= 0)
						{
								Emm_V5_Vel_Control(2, 0, Motor_Speed->stepright1, 225, 0);
						}
						else
						{
								Emm_V5_Vel_Control(2, 1, -Motor_Speed->stepright1, 225, 0);
						}
					}
					else
					{
						if(Motor_Speed->motor_clk.Clkright1 >= 0)//机械臂(左右)
						{
								Emm_V5_Pos_Control3(6, 0, Motor_Speed->stepright1, 70,Motor_Speed->motor_clk.Clkright1,true, 0);
						}
						else
						{
								Emm_V5_Pos_Control3(6, 1, Motor_Speed->stepright1, 70,-Motor_Speed->motor_clk.Clkright1,true, 0);
						}
					}
					break;
			}
			case MOTOR_2:     // 2 右后轮
			{
				if(Motor_Speed->ctrl_reg == MOTOR_SEL_DOWN)
				{
					if(Motor_Speed->stepleft2 >= 0)
					{
							Emm_V5_Vel_Control(3, 1, Motor_Speed->stepleft2, 225, 0);
					}
					else
					{
							Emm_V5_Vel_Control(3, 0, -Motor_Speed->stepleft2, 225, 0);
					}
				}
				else
				{
					if(Motor_Speed->motor_clk.Clkleft2 >= 0)//机械臂(前后)
					{
							Emm_V5_Pos_Control3(7, 0, Motor_Speed->stepleft2, 225,Motor_Speed->motor_clk.Clkleft2,true, 0);
					}
					else
					{
							Emm_V5_Pos_Control3(7, 1, -Motor_Speed->stepleft2, 225,-Motor_Speed->motor_clk.Clkleft2,true, 0);
					}
				}
					break;
			}
			case MOTOR_3:     // 3 左后轮
			{
				if(Motor_Speed->ctrl_reg == MOTOR_SEL_DOWN)
				{
					if(Motor_Speed->stepright2 >= 0)
					{
							Emm_V5_Vel_Control(4, 0, Motor_Speed->stepright2, 225, 0);
					}
					else
					{
							Emm_V5_Vel_Control(4, 1, -Motor_Speed->stepright2, 225, 0);
					}
				}
				else
				{
					;
				}
					break;
			}
			case MOTOR_FINISH:
			{
					// 全部停止电机
//					Emm_V5_Vel_Control(1, 0, 0, 0, 0);
//					Emm_V5_Vel_Control(2, 0, 0, 0, 0);
//					Emm_V5_Vel_Control(3, 0, 0, 0, 0);
//					Emm_V5_Vel_Control(4, 0, 0, 0, 0);
					break;
			}
			default:
					// 非法电机编号，直接停机保护
//					Emm_V5_Vel_Control(1, 0, 0, 0, 0);
//					Emm_V5_Vel_Control(2, 0, 0, 0, 0);
//					Emm_V5_Vel_Control(3, 0, 0, 0, 0);
//					Emm_V5_Vel_Control(4, 0, 0, 0, 0);
					break;
	}
		if(Motor_Speed->motor_sel < 4)
		{
			Motor_Speed->motor_sel += 1;
			if(Motor_Speed->ctrl_reg == MOTOR_SEL_UP)
			{
				if(Motor_Speed->motor_sel == 3)
					Motor_Speed->motor_sel += 1;
			}
		}
			
}


///**
// * @brief  UART4发送单个字节
// * @param  dat：要发送的字节数据
// * @retval 无
// */
//void UART4_SendByte(uint8_t dat)
//{
//    USART_SendData(UART4, dat);
//    // 等待发送寄存器为空
//    while(USART_GetFlagStatus(UART4, USART_FLAG_TXE) == RESET);
//}

///**
// * @brief  UART4发送字符串
// * @param  str：字符串首地址
// * @retval 无
// */
//void UART4_SendString(char *str)
//{
//    while(*str != '\0')
//    {
//        UART4_SendByte((uint8_t)*str++);
//    }
//}


