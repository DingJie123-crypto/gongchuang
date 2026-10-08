#include "stm32f4xx.h"                   // Device header
#include "increpid.h"
//#include "Encoder.h"
#include "OLED.h"  
#include <stdio.h>
#include <stdlib.h>
//#include "Moter.h"
#include "math.h"
#include "Emm_V5.h"
#include "FreeRTOS.h"
#include "queue.h"//消息队列头文件

You_PID Right_pid = {0, 0, 0, 0, 0, 0, 0};   // 电机2
Zuo_PID Left_pid = {0, 0, 0, 0, 0, 0, 0};    // 电机1
Zuo_PID1 Left_pid1 = {0, 0, 0, 0, 0, 0, 0};  // 电机3
You_PID1 Right_pid1 = {0, 0, 0, 0, 0, 0, 0}; // 电机4
extern volatile int Left_Speed, Right_Speed;

extern float step_speed1;
extern float step_speed2;
extern float step_speed3;
extern float step_speed4;
float kp = 1;;
float ki = 0.01;
float kd = 0.02;

float kp1 = 1;
float ki1 = 0.01;
float kd1 = 0.02; 

float kp3 = 1;
float ki3 = 0.01;
float kd3 = 0.02; 

float kp4 = 1;
float ki4 = 0.01;
float kd4 = 0.02; 

extern QueueHandle_t Motor_Queue;
/*
Lspeed   左目标速度
Rspeed   右目-标速度
lastlastError   上上次误差
LastError   上次误差
Error    误差
result   结果
*/
//void increment_PID(float Lspeed1, float Rspeed1,float Lspeed2, float Rspeed2)
//{
//	Left_pid._Error1 = (Lspeed1 - step_speed1);
//	Left_pid._p1 = (Left_pid._Error1 - Left_pid._LastError1);
//	Left_pid._i1 = Left_pid._Error1;
//	Left_pid._d1 = Left_pid._Error1 - 2 * Left_pid._LastError1 + Left_pid.lastlastError1;
//	Left_pid.result1 += Left_pid._p1 * kp + Left_pid._i1 * ki + Left_pid._d1 * kd;
//	Left_pid.lastlastError1 = Left_pid._LastError1;
//	Left_pid._LastError1 = Left_pid._Error1;
//	if(Left_pid.result1>320) Left_pid.result1=320;
//	if(Left_pid.result1<-320) Left_pid.result1=-320;
//	
//	
//	Right_pid._Error2 = (Rspeed1 - step_speed2);
//	Right_pid._p2 = (Right_pid._Error2 - Right_pid._LastError2);
//	Right_pid._i2 = Right_pid._Error2;
//	Right_pid._d2 = Right_pid._Error2 - 2 * Right_pid._LastError2 + Right_pid.lastlastError2; // 等于这次误差减去两倍上次误差加上上上次误差
//	Right_pid.result2 += Right_pid._p2 * kp1 + Right_pid._i2 * ki1 + Right_pid._d2 * kd1;
//	Right_pid.lastlastError2 = Right_pid._LastError2;
//	Right_pid._LastError2 = Right_pid._Error2;
//	if(Right_pid.result2>320) Right_pid.result2=320;
//	if(Right_pid.result2<-320) Right_pid.result2=-320;
//	
//	
//	Left_pid1._Error3 = (Lspeed2 - step_speed3);
//	Left_pid1._p3 = (Left_pid1._Error3 - Left_pid1._LastError3);
//	Left_pid1._i3 = Left_pid1._Error3;
//	Left_pid1._d3 = Left_pid1._Error3 - 2 * Left_pid1._LastError3 + Left_pid1.lastlastError3;
//	Left_pid1.result3 += Left_pid1._p3 * kp3 + Left_pid1._i3 * ki3 + Left_pid1._d3 * kd3;
//	Left_pid1.lastlastError3 = Left_pid1._LastError3;
//	Left_pid1._LastError3 = Left_pid1._Error3;
//	if(Left_pid1.result3>320) Left_pid1.result3=320;
//	if(Left_pid1.result3<-320) Left_pid1.result3=-320;
//	
//	
//	Right_pid1._Error4 = (Rspeed2 - step_speed4);
//	Right_pid1._p4 = (Right_pid1._Error4 - Right_pid1._LastError4);
//	Right_pid1._i4 = Right_pid1._Error4;
//	Right_pid1._d4 = Right_pid1._Error4 - 2 * Right_pid1._LastError4 + Right_pid1.lastlastError4;
//	Right_pid1.result4 += Right_pid1._p4 * kp4 + Right_pid1._i4 * ki4 + Right_pid1._d4 * kd4;
//	Right_pid1.lastlastError4 = Right_pid1._LastError4;
//	Right_pid1._LastError4 = Right_pid1._Error4;
//	if(Right_pid1.result4>320) Right_pid1.result4=320;
//	if(Right_pid1.result4<-320) Right_pid1.result4=-320;



//	step_motor(Left_pid.result1, Right_pid.result2, Left_pid1.result3, Right_pid1.result4);
//}

extern volatile float g_yaw;
extern volatile float g_x;
extern volatile float g_y;

//#define angle_p 0.495f
//#define angle_d 0.02f  

#define angle_p 2.4f	 //1.8f		2.0f	
#define angle_i 0.024f  //0.01f  0.15f
#define angle_d 0.07f  //0.07f  0.15f

//#define angle_p 1.5f	 //1.8f		2.0f	
//#define angle_i 0.01f  //0.01f  0.15f
//#define angle_d 0.07f  //0.07f  0.15f

float step_speed1;

//// 姿态角度PID（直行姿态保持）
//void angle_PID(float Lspeed1, float Rspeed1, float Lspeed2, float Rspeed2, int angle)
//{
//    static float angleError;
//    static float angleLastError;
//    static long angle_bias;

//    angleError = angle - g_yaw;  //目标角度 - 当前yaw
//    //角度绕回处理 -180~180
//    if(angleError > 180)
//    {
//        angleError = angleError - 360;
//    }
//    if(angleError < -180)
//    {
//        angleError = angleError + 360;
//    }

//    angle_bias += angleError;
//    //积分限幅
//    if(angle_bias > 45.0f)
//    {
//        angle_bias = 45;
//    }
//    else if(angle_bias < -45.0f)
//    {
//        angle_bias = -45;
//    }

//    float result = angleError * angle_p + angle_bias * angle_i + angle_d * (angleError - angleLastError);
//    angleLastError = angleError;

//    //构造电机消息结构体
//    MotorMsg_t msg;
//    msg.motor_sel = MOTOR_NONE;
//    msg.stepleft1  = Lspeed1 + 0.7f * result;
//    msg.stepright1 = Rspeed1 - 0.7f * result;
//    msg.stepleft2  = Lspeed2 + 0.7f * result;
//    msg.stepright2 = Rspeed2 - 0.7f * result;
//		
//		xQueueSend( Motor_Queue, /* 消息队列的句柄 */
//                &msg,/* 发送的消息内容 */
//                0 );        /* 等待时间 0 */
//    //step_motor(&msg);
//}


//// 姿态角度PID（直行姿态保持），形参：MotorMsg_t*基础速度 + 目标角度
//void angle_PID(MotorMsg_t *base_speed_msg, int target_angle)
//{
//    static float angleError;
//    static float angleLastError;
//    static float angle_bias;
//    angleError = target_angle - g_yaw;  //目标角度 - 当前yaw
//    //角度绕回处理 -180~180
//    if(angleError > 180)
//    {
//        angleError = angleError - 360;
//    }
//    if(angleError < -180)
//    {
//        angleError = angleError + 360;
//    }
//    angle_bias += angleError;
//    //积分限幅
//    if(angle_bias > 45.0f)
//    {
//        angle_bias = 45;
//    }
//    else if(angle_bias < -45.0f)
//    {
//        angle_bias = -45;
//    }
//    float result = angleError * angle_p + angle_bias * angle_i + angle_d * (angleError - angleLastError);
//    angleLastError = angleError;

//		Mecanum_SetMoveMode(base_speed_msg,base_speed_msg->move_mode,base_speed_msg->stepleft1);
//    // 构造最终发送给队列的消息，复制基础速度+叠加PID修正量
//    MotorMsg_t msg = *base_speed_msg; //拷贝基础速度结构体
//		
//    msg.stepleft1  = (base_speed_msg->stepleft1 	!= 0) ? (base_speed_msg->stepleft1 	+ 0.7f * result) : 0;
//    msg.stepright1 = (base_speed_msg->stepright1 	!= 0) ? (base_speed_msg->stepright1 - 0.7f * result) : 0;
//    msg.stepleft2  = (base_speed_msg->stepleft2 	!= 0) ? (base_speed_msg->stepleft2 	+ 0.7f * result) : 0;
//    msg.stepright2 = (base_speed_msg->stepright2 	!= 0) ? (base_speed_msg->stepright2 - 0.7f * result) : 0;
//		
//		xQueueSend( Motor_Queue, &msg, 0 );
//}

// increpid.c 顶部加参数
float pos_p = 0.5f;    // 先只给P，稳定后再加I/D
float lat_p = 0.5f;
float target_x = 0.0f; // 目标点，先写死，后续可改按键/上位机设定
float target_y = 0.0f;

void angle_PID(MotorMsg_t *base_speed_msg, int target_angle)
{
    static float angleError;
    static float angleLastError;
    static float angle_bias;

    static float ef_last = 0, el_last = 0;   // 位置环上次误差

    angleError = target_angle - g_yaw;
    if(angleError > 180)  angleError -= 360;
    if(angleError < -180) angleError += 360;
    angle_bias += angleError;
    if(angle_bias > 45.0f)      angle_bias = 45;
    else if(angle_bias < -45.0f) angle_bias = -45;
    float result = angleError * angle_p + angle_bias * angle_i
                 + angle_d * (angleError - angleLastError);
    angleLastError = angleError;

    /* ---- 新增：位置修正 ---- */
    float ex = target_x - g_x;          // 世界系误差
    float ey = target_y - g_y;

    float rad = g_yaw * 3.14159f / 180.0f;
    float s = sinf(rad), c = cosf(rad);
    float ef =  ex * c + ey * s;        // 车体前向误差(没走到位)
    float el = -ex * s + ey * c;        // 车体横向误差(左右偏了)

    float u_pos = pos_p * ef;           // 前向修正：四轮同向
    float u_lat = lat_p * el;           // 横向修正：麦轮平移，左右反号
    /* ------------------------ */

    Mecanum_SetMoveMode(base_speed_msg, base_speed_msg->move_mode, base_speed_msg->stepleft1);
    MotorMsg_t msg = *base_speed_msg;

    msg.stepleft1  = (base_speed_msg->stepleft1  != 0) ? (base_speed_msg->stepleft1  + 0.7f * result + u_pos + u_lat) : 0;
    msg.stepright1 = (base_speed_msg->stepright1 != 0) ? (base_speed_msg->stepright1 - 0.7f * result + u_pos - u_lat) : 0;
    msg.stepleft2  = (base_speed_msg->stepleft2  != 0) ? (base_speed_msg->stepleft2  + 0.7f * result + u_pos - u_lat) : 0;
    msg.stepright2 = (base_speed_msg->stepright2 != 0) ? (base_speed_msg->stepright2 - 0.7f * result + u_pos + u_lat) : 0;

    xQueueSend( Motor_Queue, &msg, 0 );
}

//左转角度PID（带距离停止判断）
void left_angle_PID(float Lspeed1, float Rspeed1, float Lspeed2, float Rspeed2, int angle,float distance)
{
    static float angleError;
    static float angleLastError;
    static long angle_bias;
    static uint8_t count=0;
    static uint8_t stop = 0;

    if(stop == 0)
    {
        angleError = angle - g_yaw;
        if(angleError > 180)
        {
            angleError = angleError - 360;
        }
        if(angleError < -180)
        {
            angleError = angleError + 360;
        }

        angle_bias += angleError;
        if(angle_bias > 45.0f)
        {
            angle_bias = 45;
        }
        else if(angle_bias < -45.0f)
        {
            angle_bias = -45;
        }

        float result = angleError * angle_p + angle_bias * angle_i + angle_d * (angleError - angleLastError);
        angleLastError = angleError;

        MotorMsg_t msg;
        msg.motor_sel = MOTOR_NONE;
        msg.stepleft1  = Lspeed1 + 0.7f * result;
        msg.stepright1 = -(Rspeed1 - 0.7f * result);
        msg.stepleft2  = -(Lspeed2 - 0.7f * result);
        msg.stepright2 = Rspeed2 + 0.7f * result;

        //step_motor(&msg);
				xQueueSend( Motor_Queue, /* 消息队列的句柄 */
                &msg,/* 发送的消息内容 */
                0 );        /* 等待时间 0 */

        if(step_speed1 >= distance)
        {
            stop = 1;
        }
    }
    if(stop == 1)
    {
        Emm_V5_Reset_CurPos_To_Zero(1);
        count++;
        if(count >= 7)
        {
            stop = 0;
            count = 0;
        }
    }
}

//右转角度PID
void right_angle_PID(float Lspeed1, float Rspeed1, float Lspeed2, float Rspeed2, int angle,float distance)
{
    static float angleError;
    static float angleLastError;
    static long angle_bias;

    angleError = angle - g_yaw;
    if(angleError > 180)
    {
        angleError = angleError - 360;
    }
    if(angleError < -180)
    {
        angleError = angleError + 360;
    }

    angle_bias += angleError;
    if(angle_bias > 45.0f)
    {
        angle_bias = 45;
    }
    else if(angle_bias < -45.0f)
    {
        angle_bias = -45;
    }

    float result = angleError * angle_p + angle_bias * angle_i + angle_d * (angleError - angleLastError);
    angleLastError = angleError;

    MotorMsg_t msg;
    msg.motor_sel = MOTOR_NONE;
    msg.stepleft1  = -(Lspeed1 + 0.7f * result);
    msg.stepright1 = Rspeed1 - 0.7f * result;
    msg.stepleft2  = Lspeed2 - 0.7f * result;
    msg.stepright2 = -(Rspeed2 + 0.7f * result);

    step_motor(&msg);
}




