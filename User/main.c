/**
  *********************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   FreeRTOS V9.0.0  + STM32 任务管理
  *********************************************************************
  * @attention
  *
  * 实验平台:野火 STM32 全系列开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  **********************************************************************
  */ 
 
/*
*************************************************************************
*                             包含的头文件
*************************************************************************
*/ 
/* FreeRTOS头文件 */
#include "FreeRTOS.h"
#include "task.h"
/* 开发板硬件bsp头文件 */
#include "bsp_led.h"
#include "bsp_debug_usart.h"
#include "bsp_key.h"
#include "OLED.h"
#include "board.h"
#include "wit_c_sdk.h"
#include "queue.h"//消息队列头文件
#include "increpid.h"
#include "host_parse.h"
#include "Moter.h"

volatile float g_yaw = 0.0f;
volatile float g_x = 0.0f;
volatile float g_y = 0.0f;
volatile float yaw_offset = 0.0f;     // 开机基准角度，首帧抓取，以开机朝向为0°
volatile uint8_t yaw_inited = 0;      // 首帧初始化标志

volatile uint8_t number = 0;
volatile uint8_t main_mode = 0;

volatile uint8_t GM_num_flag = 0;
#define IS_DIGIT    15     //扫比赛码改成15
volatile uint8_t GM_num = 0;
volatile char oled_buf[32]; //全局，用来保存要持续显示的字符串
/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */
static TaskHandle_t AppTaskCreate_Handle = NULL;/* 创建任务句柄 */
//static TaskHandle_t LED_Task_Handle = NULL;/* LED任务句柄 */
static TaskHandle_t MOTOR_Task_Handle = NULL;/* LED任务句柄 */
static TaskHandle_t OLED_Task_Handle = NULL;/* LED任务句柄 */
static TaskHandle_t KEY_Task_Handle = NULL;/* KEY任务句柄 */
static TaskHandle_t Main_Task_Handle = NULL;/* 主任务句柄 */



/********************************** 内核对象句柄 *********************************/
/*
 * 信号量，消息队列，事件标志组，软件定时器这些都属于内核的对象，要想使用这些内核
 * 对象，必须先创建，创建成功之后会返回一个相应的句柄。实际上就是一个指针，后续我
 * 们就可以通过这个句柄操作这些内核对象。
 *
 * 内核对象说白了就是一种全局的数据结构，通过这些数据结构我们可以实现任务间的通信，
 * 任务间的事件同步等各种功能。至于这些功能的实现我们是通过调用这些内核对象的函数
 * 来完成的
 * 
 */


QueueHandle_t Motor_Queue;	//消息队列句柄

#define QUEUE_LEN		4		//队列长度
#define QUEUE_SIZE  sizeof(MotorMsg_t)		//队列大小

/******************************* 全局变量声明 ************************************/
/*
 * 当我们在写应用程序的时候，可能需要用到一些全局变量。
 */

#define ABS(x) ((x) >= 0 ? (x) : -(x))
#define MOTOR_CM(base) ((base) - 0.5f)//距离
/*
*************************************************************************
*                             函数声明
*************************************************************************
*/
static void AppTaskCreate(void);/* 用于创建任务 */

//static void LED_Task(void* pvParameters);/* LED_Task任务实现 */
static void KEY_Task(void* pvParameters);/* KEY_Task任务实现 */
static void OLED_Task(void* pvParameters);
static void MOTOR_Task(void* pvParameters);
static void Main_Task(void* pvParameters);
static void BSP_Init(void);/* 用于初始化板载相关资源 */
void SetSysClockTo168M(void);

static void Delay_ms(volatile uint16_t ms);
static void Delay_us(volatile uint16_t us);
/*****************************************************************
  * @brief  主函数
  * @param  无
  * @retval 无
  * @note   第一步：开发板硬件初始化 
            第二步：创建APP应用任务
            第三步：启动FreeRTOS，开始多任务调度
  ****************************************************************/
int main(void)
{	
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
  /* 开发板硬件初始化 */
  BSP_Init();
  
   /* 创建AppTaskCreate任务 */
  xReturn = xTaskCreate((TaskFunction_t )AppTaskCreate,  /* 任务入口函数 */
                        (const char*    )"AppTaskCreate",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )1, /* 任务的优先级 */
                        (TaskHandle_t*  )&AppTaskCreate_Handle);/* 任务控制块指针 */ 
  /* 启动任务调度 */           
  if(pdPASS == xReturn)
    vTaskStartScheduler();   /* 启动任务，开启调度 */
  else
    return -1;  
  
  while(1);   /* 正常不会执行到这里 */    
}


/***********************************************************************
  * @ 函数名  ： AppTaskCreate
  * @ 功能说明： 为了方便管理，所有的任务创建函数都放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  **********************************************************************/
static void AppTaskCreate(void)
{
  BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
  
  taskENTER_CRITICAL();           //进入临界区
  
	Motor_Queue = xQueueCreate(QUEUE_LEN,QUEUE_SIZE); //创建消息队列
	
	
  xReturn = xTaskCreate((TaskFunction_t )OLED_Task,  /* 任务入口函数 */
                        (const char*    )"OLED_Task",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )2, /* 任务的优先级 */
                        (TaskHandle_t*  )&OLED_Task_Handle);/* 任务控制块指针 */ 
						
  xReturn = xTaskCreate((TaskFunction_t )KEY_Task,  /* 任务入口函数 */
                        (const char*    )"KEY_Task",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )3, /* 任务的优先级 */
                        (TaskHandle_t*  )&KEY_Task_Handle);/* 任务控制块指针 */ 
	xReturn = xTaskCreate((TaskFunction_t )Main_Task,  /* 任务入口函数 */
                        (const char*    )"Main_Task",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )4, /* 任务的优先级 */
                        (TaskHandle_t*  )&Main_Task_Handle);/* 任务控制块指针 */ 
	xReturn = xTaskCreate((TaskFunction_t )MOTOR_Task,  /* 任务入口函数 */
                        (const char*    )"MOTOR_Task",/* 任务名字 */
                        (uint16_t       )512,  /* 任务栈大小 */
                        (void*          )NULL,/* 任务入口函数参数 */
                        (UBaseType_t    )5, /* 任务的优先级 */
                        (TaskHandle_t*  )&MOTOR_Task_Handle);/* 任务控制块指针 */ 
  if(pdPASS == xReturn);
  vTaskDelete(AppTaskCreate_Handle); //删除AppTaskCreate任务
  
  taskEXIT_CRITICAL();            //退出临界区
}

volatile uint16_t num = 0;
volatile char ch[40];
volatile float motor_speed = 0;//速度
/**********************************************************************
  * @ 函数名  ： LED_Task
  * @ 功能说明： LED_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void OLED_Task(void* parameter)
{	
	uint8_t OLED_flag = 0;
	uint8_t Delay_500 = 0;
	
	static TickType_t pxPreviousWakeTime;
	const TickType_t xTimeIncrement = pdMS_TO_TICKS(100);
	pxPreviousWakeTime = xTaskGetTickCount();
	while (1)
	{
		OLED_flag += 1;
		if(OLED_flag >= 5)
		{
			OLED_flag = 0;
			if(Delay_500 == 0)
			{
				Delay_500 = 1;
				OLED_ShowString(0,0,"OK  ",OLED_8X16);
			}
			else
			{
				Delay_500 = 0;
				OLED_ShowString(0,0,"OFF  ",OLED_8X16);
			}
		}
		OLED_ShowNum(0,16,number,3,OLED_8X16);
		OLED_ShowNum(0,32,main_mode,3,OLED_8X16);
		sprintf(ch, "sp:%.2f      \r\n",motor_speed);
		OLED_ShowString(48,0,(uint8_t *)ch,OLED_8X16);
		sprintf(ch, "Yaw:%.2f      \r\n",g_yaw);
		OLED_ShowString(48, 16, (uint8_t *)ch, OLED_8X16); // 16*16点阵字体
		sprintf(ch, "x:%.2f      \r\n",g_x);
		OLED_ShowString(48, 32, (uint8_t *)ch, OLED_8X16); // 16*16点阵字体
		sprintf(ch, "y:%.2f      \r\n",g_y);
		OLED_ShowString(48, 48, (uint8_t *)ch, OLED_8X16); // 16*16点阵字体
		OLED_Update();
		vTaskDelayUntil(&pxPreviousWakeTime,xTimeIncrement);
	}
}

static void MOTOR_Task(void* parameter)
{	
	BaseType_t xReturn = pdTRUE;/* 定义一个创建信息返回值，默认为pdTRUE */
	uint8_t RxData_flag = 0;//接收消息标志位
	MotorMsg_t Motor_Speed;//接收电机设置速度
	//uint32_t r_queue;	/* 定义一个接收消息的变量 */
	int8_t add = 0;
	int32_t res = 0;
	uint8_t rx2_flag = 0;
	uint8_t rx3_flag = 0;
	//绝对延时变量
	static TickType_t pxPreviousWakeTime1;
	const TickType_t xTimeIncrement1 = pdMS_TO_TICKS(2);
  while (1)
  {
		if(RxData_flag == 0)
		{
			//接收消息
			 xReturn = xQueueReceive( Motor_Queue,    /* 消息队列的句柄 */
                             &Motor_Speed,      /* 发送的消息内容 */
                             portMAX_DELAY); /* 等待时间 一直等 */
			if(xReturn == pdTRUE)
			{
				RxData_flag = Motor_Speed.mode_flag;
				//OLED_ShowString(64,48,"OK  ",OLED_8X16);
				pxPreviousWakeTime1 = xTaskGetTickCount();
				//xTimeIncrement = 
			}
		}
		if(RxData_flag == 1)
		{
			step_motor(&Motor_Speed);
			if(Motor_Speed.motor_sel >= MOTOR_FINISH)
			{
				Motor_Speed.motor_sel = MOTOR_NONE;
				RxData_flag = 0;
			}
		}
		else	if(RxData_flag == 2)
		{
			if(rx2_flag == 0)
			{
				Emm_V5_Read_Sys_Params(Motor_Speed.motor_sel+1, S_CLKC);
			}
			rx2_flag ++;
			if(rx2_flag >= 2)
			{
				while(rxFrameFlag == false);
			}
			if(rxFrameFlag && rx2_flag >= 2)
			{
					rxFrameFlag = false;

					if(rxCmd[0] == Motor_Speed.motor_sel+1 && rxCmd[1] == 0x30 && rxCount == 8)
					{
							/* 4字节脉冲数，大端 */
							res = (int32_t)((uint32_t)rxCmd[3] << 24 |
															(uint32_t)rxCmd[4] << 16 |
															(uint32_t)rxCmd[5] << 8  |
															(uint32_t)rxCmd[6]);
							/* 符号位 */
							if(rxCmd[2]) { res = -res; }
					}
					motor_speed = -res/101.859f;
					rx2_flag = 0;
					Motor_Speed.motor_sel = MOTOR_NONE;
					RxData_flag = 0;
			}
			
		}
		else	if(RxData_flag == 3)
		{
//			rx3_flag++;
//			if(rx3_flag <= 1)
				Emm_V5_Reset_CurPos_To_Zero(Motor_Speed.motor_sel);
//			else
//			{
				//rx3_flag = 0;
				RxData_flag = 0;
			//}
		}
//		sprintf(ch,"Time:%d    ",xTaskGetTickCount());
//		OLED_ShowString(48,32,ch,OLED_8X16);
		//printf("Time:%d\r\n",xTaskGetTickCount());
		//vTaskDelay(8); 
		vTaskDelayUntil(&pxPreviousWakeTime1,xTimeIncrement1);
  }
}


static void Main_Task(void* parameter)
{	
	float raw = 0.00f;
	float x = 0.00f;
	float x_offset = 0.00f;
	float y = 0.00f;
	float y_offset = 0.00f;
	//发送电机设置速度
	MotorMsg_t Motor_Speed = {
	.interval_ms = 7,
	.motor_sel = MOTOR_NONE,
	.move_mode = CAR_FORWARD,
	.stepleft1 = 40,
	.stepright1 = 40,
	.stepleft2 = 40,
	.stepright2 = 40};
  while (1)
  {
		const host_feedback_t *fb = host_get_feedback();
		raw = fb->z; 
		x = fb->x;
		y = fb->y;
		//raw = Wit_GetYaw();
		if(yaw_inited == 0 && raw != 0)
		{
				yaw_offset = raw;   /* 首帧数据到达，以当前朝向为0°基准 */
				x_offset   = x;
				y_offset   = y;
				yaw_inited = 1;
		}
		g_yaw = raw - yaw_offset;            /* 相对角度：开机朝向为0 */
		g_x = x - x_offset;
		g_y = y - y_offset;
		if(g_yaw > 180.0f)  g_yaw -= 360.0f;
		if(g_yaw < -180.0f) g_yaw += 360.0f;
		

		switch(number)
		{
			case 0:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 0;
				Motor_Speed.motor_clk.Clkleft2 = 0;
				Motor_Speed.motor_clk.Clkright1 = 0;
				break;
			case 1:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 3200;
				Motor_Speed.motor_clk.Clkleft2 = 0;
				Motor_Speed.motor_clk.Clkright1 = 0;
				
					break;
			case 2:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 3200;
				Motor_Speed.motor_clk.Clkleft2 = 1300;
				Motor_Speed.motor_clk.Clkright1 = 0;
				Motor_Speed.move_mode = CAR_BACKWARD;
				Motor_speed(1,50);
					break;
			case 3:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
//				Motor_Speed.motor_clk.Clkleft1 = 1000;
//				Motor_Speed.motor_clk.Clkleft2 = 1000;
//				Motor_Speed.motor_clk.Clkright1 = -1600;
					Motor_speed(1,110);
					break;
			case 4:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 7000;
				Motor_Speed.motor_clk.Clkleft2 = 600;
				Motor_Speed.motor_clk.Clkright1 = 0;
				Motor_Speed.move_mode = CAR_BACKWARD;
					break;
			case 5:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 7000;
				Motor_Speed.motor_clk.Clkleft2 = 250;
				Motor_Speed.motor_clk.Clkright1 = -370;
				Motor_Speed.move_mode = CAR_BACKWARD;
					break;
			case 6:
				Motor_speed(1,80);
					break;
			case 7:
				Motor_speed(2,2);
				break;
			case 8:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 6000;
				Motor_Speed.motor_clk.Clkleft2 = 600;
				Motor_Speed.motor_clk.Clkright1 = -380;
					break;
			case 9:
				Motor_speed(1,123);
				break;
			case 10:
				Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.ctrl_reg = MOTOR_SEL_UP;
				Motor_Speed.motor_clk.Clkleft1 = 6000;
				Motor_Speed.motor_clk.Clkleft2 = 1100;
				Motor_Speed.motor_clk.Clkright1 = 0;
				Motor_Speed.move_mode = CAR_BACKWARD;
				break;
			case 11:
				Motor_speed(1,100);
				break;
			default:
					// 默认保持原有速度不变
			Motor_Speed.motor_sel = MOTOR_NONE;
				Motor_Speed.mode_flag = 1;
				Motor_Speed.stepleft1  = 40;
				Motor_Speed.stepright1 = 40;
				Motor_Speed.stepleft2  = 40;
				Motor_Speed.stepright2 = 40;
					Motor_Speed.move_mode = CAR_FORWARD;
					break;
		}
		
		xQueueSend( Motor_Queue, &Motor_Speed, 0 );
//		switch(number)
//		{
//			case 0:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//				Motor_Speed.move_mode = CAR_STOP;
//				break;
//			case 1:
//				Motor_Speed.move_mode = CAR_FORWARD;
//				//step_motor(MotorMsg_t* Motor_Speed);
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					break;
//			case 2:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					Motor_Speed.move_mode = CAR_BACKWARD;
//					break;
//			case 3:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					Motor_Speed.move_mode = CAR_MOVE_LEFT;
//					break;
//			case 4:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					Motor_Speed.move_mode = CAR_MOVE_RIGHT;
//					break;
//			case 5:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					Motor_Speed.move_mode = CAR_MOVE_LEFT_DOWN;
//					break;
//			case 6:
//				Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					Motor_Speed.move_mode = CAR_MOVE_RIGHT_DOWN;
//				break;
//			default:
//					// 默认保持原有速度不变
//			Motor_Speed.motor_sel = MOTOR_NONE;
//				Motor_Speed.mode_flag = 1;
//				Motor_Speed.stepleft1  = 40;
//				Motor_Speed.stepright1 = 40;
//				Motor_Speed.stepleft2  = 40;
//				Motor_Speed.stepright2 = 40;
//					Motor_Speed.move_mode = CAR_FORWARD;
//					break;
//		}
//		angle_PID(&Motor_Speed,0);
//		
//		Read_Motor_ClC(&Motor_Speed);
//		
//		if(motor_speed >= MOTOR_CM(210.0f))
//		{
//			Motor_Speed.motor_sel = MOTOR_NONE;
//			Motor_Speed.mode_flag = 1;
//			Motor_Speed.stepleft1  = 40;
//			Motor_Speed.stepright1 = 40;
//			Motor_Speed.stepleft2  = 40;
//			Motor_Speed.stepright2 = 40;
//			Motor_Speed.move_mode = CAR_STOP;
//			angle_PID(&Motor_Speed,0);
//			number = 0;
//			Delay_ms(2);
//			Motor_Pulse_Clear(&Motor_Speed,motor_speed);
//		}
//		if(number == 1)
//		{
//			angle_PID(&Motor_Speed,90);
//		}
    vTaskDelay(20);   /* 延时500个tick */
  }
}

/**********************************************************************
  * @ 函数名  ： LED_Task
  * @ 功能说明： LED_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
//static void LED_Task(void* parameter)
//{	
//  while (1)
//  {
//    LED1_ON;
//    printf("LED_Task Running,LED1_ON\r\n");
//    vTaskDelay(500);   /* 延时500个tick */
//    
//    LED1_OFF;     
//    printf("LED_Task Running,LED1_OFF\r\n");
//    vTaskDelay(500);   /* 延时500个tick */
//  }
//}



///**********************************************************************
//  * @ 函数名  ： LED_Task
//  * @ 功能说明： LE	D_Task任务主体
//  * @ 参数    ：   
//  * @ 返回值  ： 无
//  ********************************************************************/
static void KEY_Task(void* parameter)
{	
	BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */
	
	//发送电机设置速度
	MotorMsg_t Motor_Speed = {
	.interval_ms = 7,
	.move_mode = CAR_FORWARD,
	.motor_sel = MOTOR_NONE,
	.stepleft1 = 40,
	.stepright1 = 40,
	.stepleft2 = 40,
	.stepright2 = 40};

	
  static TickType_t pxPreviousWakeTime;
  const TickType_t xTimeIncrement = pdMS_TO_TICKS(20);
  pxPreviousWakeTime = xTaskGetTickCount();
	uint8_t Key_Num;
//	uint8_t i = 0;
  while (1)
  {
    Key_Tick();
//		i++;
//		if(i >= 20)
//		{
//			i = 0;
			Key_Num = Key_GetNum();
			if(Key_Num == 1)
			{
				number++;
				
				
				if(number > 10)
				{
					number = 0;
				}
				
			}
			else	if(Key_Num == 2)
			{
				main_mode = number;
				
				
				xReturn = xQueueSend( Motor_Queue, /* 消息队列的句柄 */
                            &Motor_Speed,/* 发送的消息内容 */
                            0 );        /* 等待时间 0 */
				if(pdPASS == xReturn)
				{
					OLED_ShowString(0,48,"OK  ",OLED_8X16);
					//printf("消息send_data1发送成功!\n\n");
//					sprintf((char *)ch,"Send_data1:OK");
//					OLED_ShowString(0,32,(char *)ch,OLED_8X16);
				}
			}
//		}
    vTaskDelayUntil(&pxPreviousWakeTime,xTimeIncrement);
  }
}




/***********************************************************************
  * @ 函数名  ： BSP_Init
  * @ 功能说明： 板级外设初始化，所有板子上的初始化均可放在这个函数里面
  * @ 参数    ：   
  * @ 返回值  ： 无
  *********************************************************************/
static void BSP_Init(void)
{
	/*
	 * STM32中断优先级分组为4，即4bit都用来表示抢占优先级，范围为：0~15
	 * 优先级分组只需要分组一次即可，以后如果有其他的任务需要用到中断，
	 * 都统一用这个优先级分组，千万不要再分组，切忌。
	 */
	SetSysClockTo168M();
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	
	Delay_ms(100);
	
	OLED_Init();
	
	/* LED 初始化 */
	//LED_GPIO_Config();

	/* 串口初始化	*/
	Debug_USART_Config();//扫码模块初始化
  
	/* 按键初始化	*/
	Key_GPIO_Config();
	//Wit_Usart6_Start();
	comm_init();
	
	Motor_PWM_Init();
	board_init();
	uart3_init();
	
	Motor_speed(1,100);
	Motor_speed(2,1);
	//NVIC_SystemReset();//复位函数
//	Motor_speed(3,160);
//	Motor_speed(4,160);
//	volatile uint16_t i = 5000;
//	while(i--);
//	Emm_V5_En_Control(0,true,false);
////	Emm_V5_Vel_Control(1, 1, 40, 225, 0);

//Emm_V5_Vel_Control(0, 1, 80, 225, 0);
}

static void Delay_ms(volatile uint16_t ms)
{
	while(ms--)
	{
		Delay_us(1000);
	}
}

static void Delay_us(volatile uint16_t us)
{
	volatile uint16_t i = 0;
	while(us--)
	{
		i = 168;
		while(i--);
	}
}

// 中断里面只存数据，不处理显示
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
			if(GM_num_flag == 0)
			{
				oled_buf[GM_num] = USART_ReceiveData(USART1);
        GM_num++;
				if(GM_num >= IS_DIGIT)
				{
					GM_num = 0;
					GM_num_flag = 1;
					oled_buf[IS_DIGIT] = '\0';
					
					
				}
			}
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

void SetSysClockTo168M(void)
{
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    while(RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET);

    // HSE=8M, M=8 N=336 P=2 Q=7 → 168M
    RCC_PLLConfig(RCC_PLLSource_HSE, 8, 336, 2, 7);
    RCC_PLLCmd(ENABLE);
    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);

    // ==========FLASH部分，F4正确写法==========
    FLASH_SetLatency(FLASH_Latency_3);
    FLASH->ACR |= FLASH_ACR_PRFTEN;   //预取缓冲
    FLASH->ACR |= FLASH_ACR_ICEN;     //指令cache
    FLASH->ACR |= FLASH_ACR_DCEN;     //数据cache
    // ========================================

    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div4);
    RCC_PCLK2Config(RCC_HCLK_Div2);

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while(RCC_GetSYSCLKSource() != 0x08);
}


/********************************END OF FILE****************************/
