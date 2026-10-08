#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f4xx.h"
#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "queue.h"//消息队列头文件
/**********************************************************
***	Emm_V5.0步进闭环控制例程
***	编写作者：ZHANGDATOU
***	技术支持：张大头闭环伺服
***	淘宝店铺：https://zhangdatou.taobao.com
***	CSDN博客：http s://blog.csdn.net/zhangdatou666
***	qq交流群：262438510
**********************************************************/

// 电机选择枚举定义
typedef enum
{
    MOTOR_NONE = 0,   // 0：电机1
    MOTOR_1    = 1,   // 1：电机2
    MOTOR_2    = 2,   // 2：电机3
    MOTOR_3    = 3,   // 3：电机4
    MOTOR_FINISH= 4   // 4：动作全部完成
}MotorSel_t;

// 上层电机还是下层电机
typedef enum
{
    MOTOR_SEL_DOWN = 0,   // 0：电机1
    MOTOR_SEL_UP   = 1,   // 1：电机2
}MotorRegSel_t;

// 小车运动模式枚举
typedef enum
{
    CAR_STOP = 0,        // 停止
    CAR_FORWARD,         // 前进
    CAR_BACKWARD,        // 后退
    CAR_MOVE_LEFT,       // 左平移
    CAR_MOVE_RIGHT,      // 右平移
		CAR_MOVE_LEFT_DOWN,  // 左斜下平移
    CAR_MOVE_RIGHT_DOWN, // 右斜下平移
		CAR_KEEP             //保持原有状态
}CarMoveMode_t;

typedef struct{
	int16_t Clkleft1; //机械臂(上下) 
	int16_t Clkright1;//机械臂(左右)
	int16_t Clkleft2;//机械臂(前后)
	//int16_t Clkright2;//右后轮
}MotorClk_t;

typedef struct{
	uint16_t interval_ms; // 每次动作间隔tick
	uint8_t mode_flag;//电机任务控制标志位 1：底板电机驱动 2：电机脉冲读取 3：将步进电机现在已经保存的输入脉冲置零
	MotorRegSel_t ctrl_reg;//选择上层电机还是下层电机
	MotorSel_t motor_sel; //控制哪一个电机 0-3
	CarMoveMode_t move_mode; // 新增：小车运动模式
	MotorClk_t motor_clk;//位置模式：输入脉冲
	int16_t stepleft1; //左前轮
	int16_t stepright1;//右前轮
	int16_t stepleft2; //左后轮
	int16_t stepright2;//右后轮
}MotorMsg_t;

void nvic_init(void);
void clock_init(void);
void usart_init(void);
void board_init(void);
void Mecanum_SetMoveMode(MotorMsg_t* msg, CarMoveMode_t mode, int16_t speed);
void step_motor(MotorMsg_t* Motor_Speed);
void Read_Motor_ClC(MotorMsg_t* Motor_Speed);
void Motor_Pulse_Clear(MotorMsg_t* Motor_Speed,float motor_speed);
#endif
