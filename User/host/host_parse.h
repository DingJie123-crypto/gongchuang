#ifndef _HOST_PARSE_H
#define _HOST_PARSE_H

#include <stdint.h>

/* ================= 与 OPS9 底盘板 USART2 协议对接 =================
 * 波特率 256000, 8N1
 *
 * 底盘->本机 反馈帧 (14字节, 1ms一帧, 小端):
 *   [0]  帧头 0x5C
 *   [1..4]  x : float, 世界系位置 (m)
 *   [5..8]  y : float, 世界系位置 (m)
 *   [9..12] z : float, yaw 角度 (deg)
 *   [13] CRC8 (多项式 x8+x5+x4+1, 初值 0xFF, 覆盖前13字节)
 *
 * 本机->底盘 命令帧 (2字节):
 *   [0] 帧头 0xC5
 *   [1] 命令字节 data
 * ================================================================ */

#define HOST_FRAME_TX_LEN   14u    /* 底盘反馈帧长 */
#define HOST_FRAME_RX_LEN   2u     /* 发往底盘的命令帧长 */
#define HOST_FRAME_HEADER   0x5Cu
#define HOST_FRAME_CMD_HDR  0xC5u

typedef struct
{
    float x;        /* 世界系 X 位置 (m) */
    float y;        /* 世界系 Y 位置 (m) */
    float z;        /* yaw 角度 (deg)    */
    uint8_t ok;     /* 最近一帧是否校验通过 */
} host_feedback_t;

void comm_init(void);
float comm_task(void);
/* 每收到 1 字节调用一次 (放串口接收中断里)
 * 返回: 1 = 刚解析完一帧有效数据(校验通过), 0 = 其他 */
uint8_t host_parse_byte(uint8_t byte);

/* 也可整段缓冲一次喂入, 返回完整解析出的有效帧数 */
uint8_t host_parse_buffer(const uint8_t *buf, uint8_t len);

/* 获取最近一帧解析结果 (指针, 勿拷贝结构体时中断竞争可自行加锁) */
const host_feedback_t *host_get_feedback(void);

/* 组一条命令帧到 out (至少2字节), 并通过 send_fn 逐字节发送
 * send_fn 传入 NULL 则只组帧不发送 */
void host_make_cmd(uint8_t cmd, uint8_t out[HOST_FRAME_RX_LEN],
                   void (*send_fn)(uint8_t byte));

/* 可选: 对端若需要校验底盘反馈 CRC, 单帧校验接口 */
uint8_t host_verify_frame(const uint8_t *frame, uint8_t len);

#endif
