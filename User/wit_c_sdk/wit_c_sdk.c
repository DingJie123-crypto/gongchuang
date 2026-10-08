#include "wit_c_sdk.h"

/* ============================================================
 * WIT standard protocol SDK — NORMAL(0x55) only.
 * MODBUS / CAN / I2C branches stripped (saves ~1KB flash:
 * CRC16 lookup tables 512B + per-protocol switch cases).
 * Synced from official WITMOTION/WitStandardProtocol_JY901.
 * ============================================================ */

static SerialWrite p_WitSerialWriteFunc = NULL;
static RegUpdateCb p_WitRegUpdateCbFunc = NULL;
static DelaymsCb  p_WitDelaymsFunc = NULL;

static uint8_t __attribute__((unused)) s_ucAddr = 0xff;
static uint8_t  s_ucWitDataBuff[WIT_DATA_BUFF_SIZE];
static uint32_t s_uiWitDataCnt = 0, __attribute__((unused)) s_uiProtoclo = 0, s_uiReadRegIndex = 0;
int16_t sReg[REGSIZE];

static uint8_t __CaliSum(uint8_t *data, uint32_t len)
{
    uint32_t i;
    uint8_t ucCheck = 0;
    for(i=0; i<len; i++) ucCheck += *(data + i);
    return ucCheck;
}

int32_t WitSerialWriteRegister(SerialWrite Write_func)
{
    if(!Write_func)return WIT_HAL_INVAL;
    p_WitSerialWriteFunc = Write_func;
    return WIT_HAL_OK;
}

static void CopeWitData(uint8_t ucIndex, uint16_t *p_data, uint32_t uiLen)
{
    uint32_t uiReg1 = 0, uiReg2 = 0, uiReg1Len = 0, uiReg2Len = 0;
    uint16_t *p_usReg1Val = p_data;
    uint16_t *p_usReg2Val = p_data+3;

    uiReg1Len = 4;
    switch(ucIndex)
    {
        case WIT_ACC:      uiReg1 = AX;         uiReg1Len = 3; uiReg2 = TEMP;    uiReg2Len = 1; break;
        case WIT_ANGLE:    uiReg1 = Roll;       uiReg1Len = 3; uiReg2 = VERSION;  uiReg2Len = 1; break;
        case WIT_TIME:     uiReg1 = YYMM;       break;
        case WIT_GYRO:     uiReg1 = GX;         uiReg1Len = 3; break;
        case WIT_MAGNETIC: uiReg1 = HX;         uiReg1Len = 3; break;
        case WIT_DPORT:    uiReg1 = D0Status;   break;
        case WIT_PRESS:    uiReg1 = PressureL;  break;
        case WIT_GPS:      uiReg1 = LonL;       break;
        case WIT_VELOCITY: uiReg1 = GPSHeight;  break;
        case WIT_QUATER:   uiReg1 = q0;         break;
        case WIT_GSA:      uiReg1 = SVNUM;      break;
        case WIT_REGVALUE: uiReg1 = s_uiReadRegIndex; break;
        default: return;
    }
    if(uiLen == 3)
    {
        uiReg1Len = 3;
        uiReg2Len = 0;
    }
    if(uiReg1Len)
    {
        memcpy(&sReg[uiReg1], p_usReg1Val, uiReg1Len<<1);
        if(p_WitRegUpdateCbFunc) p_WitRegUpdateCbFunc(uiReg1, uiReg1Len);
    }
    if(uiReg2Len)
    {
        memcpy(&sReg[uiReg2], p_usReg2Val, uiReg2Len<<1);
        if(p_WitRegUpdateCbFunc) p_WitRegUpdateCbFunc(uiReg2, uiReg2Len);
    }
}

void WitSerialDataIn(uint8_t ucData)
{
    uint16_t usData[4];
    uint8_t ucSum;
    if(p_WitRegUpdateCbFunc == NULL)return ;
    s_ucWitDataBuff[s_uiWitDataCnt++] = ucData;
    if(s_ucWitDataBuff[0] != 0x55)
    {
        s_uiWitDataCnt--;
        memcpy(s_ucWitDataBuff, &s_ucWitDataBuff[1], s_uiWitDataCnt);
        return ;
    }
    if(s_uiWitDataCnt >= 11)
    {
        ucSum = __CaliSum(s_ucWitDataBuff, 10);
        if(ucSum != s_ucWitDataBuff[10])
        {
            s_uiWitDataCnt--;
            memcpy(s_ucWitDataBuff, &s_ucWitDataBuff[1], s_uiWitDataCnt);
            return ;
        }
        usData[0] = ((uint16_t)s_ucWitDataBuff[3] << 8) | (uint16_t)s_ucWitDataBuff[2];
        usData[1] = ((uint16_t)s_ucWitDataBuff[5] << 8) | (uint16_t)s_ucWitDataBuff[4];
        usData[2] = ((uint16_t)s_ucWitDataBuff[7] << 8) | (uint16_t)s_ucWitDataBuff[6];
        usData[3] = ((uint16_t)s_ucWitDataBuff[9] << 8) | (uint16_t)s_ucWitDataBuff[8];
        CopeWitData(s_ucWitDataBuff[1], usData, 4);
        s_uiWitDataCnt = 0;
    }
    if(s_uiWitDataCnt == WIT_DATA_BUFF_SIZE)s_uiWitDataCnt = 0;
}

int32_t WitRegisterCallBack(RegUpdateCb update_func)
{
    if(!update_func)return WIT_HAL_INVAL;
    p_WitRegUpdateCbFunc = update_func;
    return WIT_HAL_OK;
}

int32_t WitWriteReg(uint32_t uiReg, uint16_t usData)
{
    uint8_t ucBuff[8];
    if(uiReg >= REGSIZE)return WIT_HAL_INVAL;
    if(p_WitSerialWriteFunc == NULL)return WIT_HAL_EMPTY;
    ucBuff[0] = 0xFF;
    ucBuff[1] = 0xAA;
    ucBuff[2] = uiReg & 0xFF;
    ucBuff[3] = usData & 0xff;
    ucBuff[4] = usData >> 8;
    p_WitSerialWriteFunc(ucBuff, 5);
    return WIT_HAL_OK;
}

int32_t WitReadReg(uint32_t uiReg, uint32_t uiReadNum)
{
    uint8_t ucBuff[8];
    if((uiReg + uiReadNum) >= REGSIZE)return WIT_HAL_INVAL;
    if(uiReadNum > 4)return WIT_HAL_INVAL;
    if(p_WitSerialWriteFunc == NULL)return WIT_HAL_EMPTY;
    ucBuff[0] = 0xFF;
    ucBuff[1] = 0xAA;
    ucBuff[2] = 0x27;
    ucBuff[3] = uiReg & 0xff;
    ucBuff[4] = uiReg >> 8;
    p_WitSerialWriteFunc(ucBuff, 5);
    s_uiReadRegIndex = uiReg;
    return WIT_HAL_OK;
}

int32_t WitInit(uint32_t uiProtocol, uint8_t ucAddr)
{
    if(uiProtocol != WIT_PROTOCOL_NORMAL)return WIT_HAL_INVAL;
    s_uiProtoclo = uiProtocol;
    s_ucAddr = ucAddr;
    s_uiWitDataCnt = 0;
    return WIT_HAL_OK;
}

int32_t WitDelayMsRegister(DelaymsCb delayms_func)
{
    if(!delayms_func)return WIT_HAL_INVAL;
    p_WitDelaymsFunc = delayms_func;
    return WIT_HAL_OK;
}

char CheckRange(short sTemp,short sMin,short sMax)
{
    if ((sTemp>=sMin)&&(sTemp<=sMax)) return 1;
    else return 0;
}

/* Acceleration calibration */
int32_t WitStartAccCali(void)
{
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(CALSW, CALGYROACC) != WIT_HAL_OK)      return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}
int32_t WitStopAccCali(void)
{
    if(WitWriteReg(CALSW, NORMAL) != WIT_HAL_OK)          return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(SAVE, SAVE_PARAM) != WIT_HAL_OK)       return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}

/* Magnetic field calibration */
int32_t WitStartMagCali(void)
{
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(CALSW, CALMAGMM) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}
int32_t WitStopMagCali(void)
{
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(CALSW, NORMAL) != WIT_HAL_OK)          return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}

/* UART baud rate */
int32_t WitSetUartBaud(int32_t uiBaudIndex)
{
    if(!CheckRange(uiBaudIndex, WIT_BAUD_4800, WIT_BAUD_230400)) return WIT_HAL_INVAL;
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(BAUD, uiBaudIndex) != WIT_HAL_OK)      return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}
/* Bandwidth */
int32_t WitSetBandwidth(int32_t uiBaudWidth)
{
    if(!CheckRange(uiBaudWidth, BANDWIDTH_256HZ, BANDWIDTH_5HZ)) return WIT_HAL_INVAL;
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(BANDWIDTH, uiBaudWidth) != WIT_HAL_OK) return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}
/* Output rate */
int32_t WitSetOutputRate(int32_t uiRate)
{
    if(!CheckRange(uiRate, RRATE_02HZ, RRATE_NONE)) return WIT_HAL_INVAL;
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(RRATE, uiRate) != WIT_HAL_OK)          return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}
/* Output content */
int32_t WitSetContent(int32_t uiRsw)
{
    if(!CheckRange(uiRsw, RSW_TIME, RSW_MASK)) return WIT_HAL_INVAL;
    if(WitWriteReg(KEY, KEY_UNLOCK) != WIT_HAL_OK)        return WIT_HAL_ERROR;
    if(p_WitDelaymsFunc) p_WitDelaymsFunc(1);
    if(WitWriteReg(RSW, uiRsw) != WIT_HAL_OK)             return WIT_HAL_ERROR;
    return WIT_HAL_OK;
}

/* ============================================================
 * USART6 port driver for STM32F4  (WIT module on PC6=TX / PC7=RX)
 * 9600 8N1, RXNE interrupt -> WitSerialDataIn().
 * ============================================================ */

static void Wit_Usart6_SendData(uint8_t *p_data, uint32_t uiLen)
{
    uint32_t i;
    for(i = 0; i < uiLen; i++)
    {
        while(USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET);
        USART_SendData(USART6, p_data[i]);
    }
    while(USART_GetFlagStatus(USART6, USART_FLAG_TC) == RESET);
}

/* Default empty update callback so WitSerialDataIn() works out of the box.
 * sReg[] is still filled by CopeWitData() before this is called, so reading
 * sReg[Yaw] / sReg[Roll] / ... works without a user callback. */
static void Wit_DefaultUpdateCb(uint32_t uiReg, uint32_t uiRegNum)
{
    (void)uiReg;
    (void)uiRegNum;
}

void Wit_Usart6_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef  NVIC_InitStructure;

    /* PC6 / PC7 on AHB1, USART6 on APB2 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    /* PC6 = USART6_TX, PC7 = USART6_RX, AF8 */
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_USART6);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_USART6);

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd   = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* 9600, 8N1, no flow control, RX/TX */
    USART_InitStructure.USART_BaudRate            = 230400;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl= USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART6, &USART_InitStructure);

    /* RXNE interrupt -> WitSerialDataIn() */
    USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel                   = USART6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 6;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART6, ENABLE);
}

void Wit_Usart6_Start(void)
{
    WitInit(WIT_PROTOCOL_NORMAL, 0x50);
    WitSerialWriteRegister(Wit_Usart6_SendData);
    WitRegisterCallBack(Wit_DefaultUpdateCb);
    Wit_Usart6_Init();
}

/* RX interrupt: feed every received byte to the WIT parser */
//void USART6_IRQHandler(void)
//{
//    uint8_t ch;
//    if(USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
//    {
//        ch = (uint8_t)USART_ReceiveData(USART6);
//        WitSerialDataIn(ch);
//        USART_ClearITPendingBit(USART6, USART_IT_RXNE);
//    }
//    if(USART_GetITStatus(USART6, USART_IT_ORE) != RESET)
//    {
//        (void)USART6->SR;
//        (void)USART6->DR;
//    }
//}

/* ============================================================
 * Convenience getters — read physical quantities from sReg[].
 * Scale assumptions (factory defaults):
 *   acc  : +/-16g        -> /32768 * 16        [g]
 *   gyro : +/-2000dps    -> /32768 * 2000      [deg/s]
 *   angle: +/-180deg     -> /32768 * 180       [deg]
 * If you reconfigure the sensor range, update the constants below.
 * ============================================================ */
#define WIT_ACC_SCALE     16.0f
#define WIT_GYRO_SCALE    2000.0f
#define WIT_ANGLE_SCALE   180.0f

float Wit_GetYaw(void)    { return (float)sReg[Yaw]   / 32768.0f * WIT_ANGLE_SCALE; }
float Wit_GetRoll(void)   { return (float)sReg[Roll]  / 32768.0f * WIT_ANGLE_SCALE; }
float Wit_GetPitch(void)  { return (float)sReg[Pitch] / 32768.0f * WIT_ANGLE_SCALE; }

float Wit_GetAccX(void)   { return (float)sReg[AX]    / 32768.0f * WIT_ACC_SCALE;   }
float Wit_GetAccY(void)   { return (float)sReg[AY]    / 32768.0f * WIT_ACC_SCALE;   }
float Wit_GetAccZ(void)   { return (float)sReg[AZ]    / 32768.0f * WIT_ACC_SCALE;   }

float Wit_GetGyroX(void)  { return (float)sReg[GX]    / 32768.0f * WIT_GYRO_SCALE;  }
float Wit_GetGyroY(void)  { return (float)sReg[GY]    / 32768.0f * WIT_GYRO_SCALE;  }
float Wit_GetGyroZ(void)  { return (float)sReg[GZ]    / 32768.0f * WIT_GYRO_SCALE;  }

int16_t Wit_GetMagX_Raw(void)  { return sReg[HX]; }
int16_t Wit_GetMagY_Raw(void)  { return sReg[HY]; }
int16_t Wit_GetMagZ_Raw(void)  { return sReg[HZ]; }
int16_t Wit_GetTempRaw(void)   { return sReg[TEMP]; }
