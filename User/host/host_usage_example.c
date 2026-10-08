/* ============ 对端单片机使用示例 (STM32F407 标准外设库 StdPeriph 版) ============
 * 以 USART3 为例, 256000 8N1, 接 OPS9 板 PA2(TX)/PA3(RX)
 * 接线: 对端TX -> OPS9 PA3(RX), 对端RX -> OPS9 PA2(TX), 共地
 * 需在工程中使能: RCC_APB1Periph_USART3, RCC_AHB1Periph_GPIOB/C(按引脚)
 * ============================================================================ */

#include "host_parse.h"
#include "stm32f4xx.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "misc.h"

/* 文件级接收缓冲: 中断服务与重新挂载必须用同一个变量 */
static uint8_t comm_rx_byte;

/* ---- 1. 串口初始化 (USART3: TX=PB10, RX=PB11, 256000 8N1) ---- */

void comm_init(void)
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
    USART_InitStructure.USART_BaudRate            = 256000;
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
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART6, ENABLE);
}

/* ---- 2. 串口中断服务: 逐字节喂解析器 ---- */
void USART6_IRQHandler(void)
{
    if (USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
    {
        comm_rx_byte = (uint8_t)USART_ReceiveData(USART6);
        host_parse_byte(comm_rx_byte);
        /* RXNE 读数据即清标志, 无需手动清除 */
    }
    /* 溢出错误保护: 防止 ORE 卡死接收 */
    if (USART_GetFlagStatus(USART6, USART_FLAG_ORE) != RESET)
    {
        USART_ReceiveData(USART6);
    }
}

/* ---- 3. 主循环里读取解析结果 ---- */
//float comm_task(void)
//{
//    const host_feedback_t *fb = host_get_feedback();
//    if (fb->ok)
//    {
//        /* fb->x, fb->y: 世界系位置(m); fb->z: yaw(度) */
//        float x = fb->x, y = fb->y, yaw_deg = fb->z;
//        /*(void)x; (void)y; */return yaw_deg;
//    }
//}

/* ---- 4. 发命令给 OPS9 板 (阻塞发送) ---- */
static void uart6_send_byte(uint8_t b)
{
    USART_SendData(USART6, b);
    while (USART_GetFlagStatus(USART6, USART_FLAG_TXE) == RESET)
        ;
}

void comm_send_cmd(uint8_t cmd)
{
    uint8_t frame[HOST_FRAME_RX_LEN];
    host_make_cmd(cmd, frame, uart6_send_byte);
    /* 也可直接: uart3_send_byte(0xC5); uart3_send_byte(cmd); */
}
