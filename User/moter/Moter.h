#ifndef  __MOTER_H__
#define  __MOTER_H__

#include "stm32f4xx.h"                  // Device header

void Motor_Init(void);
void Motor_PWM_Init(void);
void Motor_speed(uint8_t Num,int16_t M_Yaw);
void Servo_GoAngle(float angle,uint8_t num);
#endif
