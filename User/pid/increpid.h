#ifndef _INCREPID_H
#define _INCREPID_H
#include <stdio.h>
#include <stdlib.h>
#include "board.h"
// 电机2（右前）
typedef struct
{
	float _p2;
	float _i2;
	float _d2;
	float _Error2;
	float _LastError2;
	float lastlastError2;
	float result2;
}You_PID;

// 电机1（左前）
typedef struct
{
	float _p1;
	float _i1;
	float _d1;
	float _Error1;
	float _LastError1;
	float lastlastError1;
	float result1;
}Zuo_PID;

// 电机3（左后）
typedef struct
{
	float _p3;
	float _i3;
	float _d3;
	float _Error3;
	float _LastError3;
	float lastlastError3;
	float result3;
}Zuo_PID1;

// 电机4（右后）
typedef struct
{
	float _p4;
	float _i4;
	float _d4;
	float _Error4;
	float _LastError4;
	float lastlastError4;
	float result4;
}You_PID1;

void increment_PID(float Lspeed1, float Rspeed1,float Lspeed2, float Rspeed2);
void angle_PID(MotorMsg_t *base_speed_msg, int target_angle);
void right_angle_PID(float Lspeed1, float Rspeed1, float Lspeed2, float Rspeed2, int angle,float distance);
void left_angle_PID(float Lspeed1, float Rspeed1, float Lspeed2, float Rspeed2, int angle,float distance);
//void step_motor(int16_t stepleft1,int16_t stepright1,int16_t stepleft2,int16_t stepright2);
#endif
