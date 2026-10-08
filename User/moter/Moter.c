#include "Moter.h"

void Motor_Init(void)
{      
    // 定义一个GPIO初始化结构体
    GPIO_InitTypeDef GPIO_InitStructstr;
    // 使能GPIOD端口的时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    // 设置GPIO_InitStructstr结构体中的引脚位，这里选择的是GPIOD端口的4、5、6、7号引脚
    GPIO_InitStructstr.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    // 设置GPIO模式为输出模式
    GPIO_InitStructstr.GPIO_Mode = GPIO_Mode_OUT;
    // 设置GPIO输出类型为推挽输出
    GPIO_InitStructstr.GPIO_OType = GPIO_OType_PP;
    // 设置GPIO上拉/下拉为上拉
    GPIO_InitStructstr.GPIO_PuPd = GPIO_PuPd_UP;
    // 设置GPIO输出速度为100MHz
    GPIO_InitStructstr.GPIO_Speed = GPIO_Speed_100MHz;
    // 使用上面定义的参数初始化GPIOD端口的对应引脚
    GPIO_Init(GPIOD, &GPIO_InitStructstr);
    // 将GPIOD端口的4、5、6、7号引脚复位（即设置为低电平）
    GPIO_ResetBits(GPIOD, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7);
}

void TIM4_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能

    TIM_TimeBaseStructure.TIM_Period = arr;
    TIM_TimeBaseStructure.TIM_Prescaler =psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
    TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
	
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    TIM_Cmd(TIM4, ENABLE);

}


void Motor_PWM_Init(void)
{		 					 
	// 定义GPIO和TIM的初始化结构体变量
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	// ========== F4不需要GPIO_PinRemapConfig！删掉F1的重映射代码 ==========
	// 只需要关闭JTAG释放PA15 PB3（F4写法）
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	// 使能TIM2的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  
	// 使能GPIOA和GPIOB的时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE); 

	// ========== 四个通道引脚复用配置 AF1_TIM2 ==========
	//CH1:PA15
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource15,GPIO_AF_TIM2); 
	//CH2:PB3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource3,GPIO_AF_TIM2); 
	//CH3:PB10
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_TIM2);
	//CH4:PB11
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_TIM2);

	// ========== GPIO初始化 ==========
	//PA15
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;           
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        
	GPIO_Init(GPIOA,&GPIO_InitStructure);             
	
	//PB3 PB10 PB11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_10 | GPIO_Pin_11;           
	GPIO_Init(GPIOB,&GPIO_InitStructure);              
	
	// ===================== 50Hz配置 =====================
	TIM_TimeBaseStructure.TIM_Prescaler=83;   //psc=83
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; 
	TIM_TimeBaseStructure.TIM_Period=19999;   //arr=19999
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseStructure);
	
	// CH1 PA15
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; 
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);  
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);  

	// CH2 PB3
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);  
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);  

	// CH3 PB10
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);
	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);

	// CH4 PB11
	TIM_OC4Init(TIM2, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM2,ENABLE);
	TIM_Cmd(TIM2, ENABLE);
	
	TIM4_Int_Init(99,7199);
}

/********************************************
*                                           *
*               内部变量                     *
*                                           *
********************************************/
//电池电压采集辅组变量
static int Voltage_All = 0;
static uint8_t Voltage_Count = 0;
static uint16_t reach_FilterCount = 0;

static float target_limit_float(float insert,float low,float high);
static uint8_t target_limit_u8(uint8_t insert,uint8_t low,uint8_t high);
static float PI_Compute(float target, float curr);

/********************************************
*                                           *
*               全局变量                     *
*                                           *
********************************************/
float Voltage;//电池电压
float F = 0;//云台当前的角度
float PTZ_TargetAngle = 0; //云台目标角度
uint8_t PTZ_ControlStep = 30;//云台控制步进值
uint8_t select_mode = 0;
uint8_t target_reach_flag = 0;

volatile int target_pwm1 = 0;
volatile int target_pwm2 = 0;
// 每毫秒对应的计数，在 TIM2 10ms 中断里递减
volatile static int run_countdown1 = 0;   // >0 表示正在走角度
volatile static int run_countdown2 = 0;   // >0 表示正在走角度

// 让舵机以固定速度转 angle 度（需实测标定 ms_per_deg）
void Servo_GoAngle(float angle,uint8_t num)
{
	const int speed = 200;                  // 固定转速档（正方向）
	const float ms_per_deg1 = 12.0f;          // 【需标定】：转1度需要的毫秒数
	const float ms_per_deg2 = 13.0f;          // 【需标定】：转1度需要的毫秒数
	if(num == 1)
	{
		run_countdown1 = (int)(angle * ms_per_deg1 / 10.0f); // 10ms中断次数
		target_pwm1 = speed;                     // 先启动
	}
	else
	{
		run_countdown2 = (int)(angle * ms_per_deg2 / 10.0f); // 10ms中断次数
		target_pwm2 = speed;                     // 先启动
	}
   
}

//舵机速度控制
int servo_speed_control(int speed) {
    // 限制输入范围
    if (speed > 400) speed = 400;
    if (speed < -400) speed = -400;
    
	int pwm = 0;
	
	if( speed > 0 )
		pwm = 1600 + speed;  //1500~1600是大致正方向死区的位置
	else if( speed < 0 )
		pwm = 1400 + speed;  //1400~1500是大致反方向死区的位置
    else pwm = 1500;
	
    return pwm;
}

//定时器2更新中断
void TIM4_IRQHandler(void)
{
	
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
	{   
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update); 
		
		// 在 TIM2_IRQHandler 的按键处理之后加：
		if (run_countdown1 > 0) 
		{
			if (--run_countdown1 == 0)
					target_pwm1 = 0;                     // 时间到，停转
		}
		
		if (run_countdown2 > 0) 
		{
			if (--run_countdown2 == 0)
					target_pwm2 = 0;                     // 时间到，停转
		}
		
		if( target_pwm1 > 400 ) target_pwm1 = 400;
		if( target_pwm1 <-400 ) target_pwm1 = -400;
		
		if( target_pwm2 > 400 ) target_pwm2 = 400;
		if( target_pwm2 <-400 ) target_pwm2 = -400;
		// 输出到舵机
		TIM2->CCR3 = servo_speed_control(target_pwm1);
		TIM2->CCR4 = servo_speed_control(-target_pwm2);
		//采集电池电压
		//Voltage_All+=Get_battery_volt();
		//if(++Voltage_Count==100) Voltage=(float)Voltage_All/10000.0f,Voltage_All=0,Voltage_Count=0,LED=!LED;
	}
}


void Motor_speed(uint8_t Num,int16_t M_Yaw)
{
	if(Num == 0)
	{
		TIM_SetCompare1(TIM2,M_Yaw*2000.0f/270.0f+500);
		TIM_SetCompare2(TIM2,M_Yaw*2000.0f/270.0f+500);
		Servo_GoAngle(M_Yaw ,1); //160
		Servo_GoAngle(M_Yaw,2);
	}
	else	if(Num == 1)//爪子 143完全闭合  123抓紧物块
	{
		TIM_SetCompare1(TIM2,M_Yaw*2000.0f/270.0f+500);
	}
	else	if(Num == 2)//转盘 第一个点位 75 后续旋转需要增加120°
	{
		if(M_Yaw == 1)
			M_Yaw = 75;
		else	if(M_Yaw == 2)
			M_Yaw = 75+120;
		else	if(M_Yaw == 3)
			M_Yaw = 75+120*2;
		TIM_SetCompare2(TIM2,M_Yaw*2000.0f/270.0f+500);
	}
	else	if(Num == 3)
	{
		Servo_GoAngle(M_Yaw ,1);
	}
	else	if(Num == 4)
	{
		Servo_GoAngle(M_Yaw,2);
	}
//		TIM_SetCompare3(TIM2,right1*2000.0f/270.0f+500);
//		TIM_SetCompare4(TIM2,right1*2000.0f/270.0f+500);
}




