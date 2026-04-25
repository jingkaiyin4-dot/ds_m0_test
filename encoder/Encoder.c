/**
 * @file Encoder.c
 * @brief 编码器驱动实现
 *
 * MG513XP26 AB相编码器:
 * - A相: 触发中断计数
 * - B相: 判断方向(B相高电平=正向)
 * - 11 PPR: 每转11个脉冲
 *
 * 增量计数原理:
 * - 每次A相边沿触发中断
 * - 读取B相电平: 高=正转, 低=反转
 * - 正转+1, 反转-1
 */

#include "ti_msp_dl_config.h"
#include "Encoder.h"

#include <stdbool.h>

/**
 * 编码器计数:
 * - g_encoderCounts: 增量计数(本周期,用于测速)
 * - g_encoderTotals: 累计计数(用于定位)
 */
static volatile int32_t g_encoderCounts[2];
static volatile int32_t g_encoderTotals[2];

/**
 * 引脚映射(物理 wiring):
 * - 右编码器: PB3=A相, PB5=B相
 * - 左编码器: PB6=A相, PB7=B相
 * SysConfig生成的引脚名仍使用LA/LB/RA/RB
 */
#define ENCODER_RIGHT_A_PIN Encoder_PIN_LA_PIN
#define ENCODER_RIGHT_B_PIN Encoder_PIN_LB_PIN
#define ENCODER_LEFT_A_PIN  Encoder_PIN_RA_PIN
#define ENCODER_LEFT_B_PIN  Encoder_PIN_RB_PIN

/**
 * @brief 更新编码器计数
 * @param channel 编码器通道
 * @param forward true=正转, false=反转
 */
static void Encoder_Update(EncoderChannel channel, bool forward)
{
    if (forward) {
        g_encoderCounts[channel]++;
        g_encoderTotals[channel]++;
    } else {
        g_encoderCounts[channel]--;
        g_encoderTotals[channel]--;
    }
}

/**
 * @brief GPIO中断处理
 * 只对A相开中断，检测到A相边沿后:
 * 1. 读取B相电平判断方向
 * 2. 更新计数
 */
void GROUP1_IRQHandler(void)
{
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(
        Encoder_PORT,
        ENCODER_RIGHT_A_PIN | ENCODER_LEFT_A_PIN);

    /** 右轮: B相高电平=正转 */
    if (pending & ENCODER_RIGHT_A_PIN) {
        bool forward = (DL_GPIO_readPins(Encoder_PORT, ENCODER_RIGHT_B_PIN) != 0U);
        Encoder_Update(ENCODER_RIGHT, forward);
        DL_GPIO_clearInterruptStatus(Encoder_PORT, ENCODER_RIGHT_A_PIN);
    }

    /** 左轮: B相低电平=正转(安装方向与右轮相反) */
    if (pending & ENCODER_LEFT_A_PIN) {
        bool forward = (DL_GPIO_readPins(Encoder_PORT, ENCODER_LEFT_B_PIN) == 0U);
        Encoder_Update(ENCODER_LEFT, forward);
        DL_GPIO_clearInterruptStatus(Encoder_PORT, ENCODER_LEFT_A_PIN);
    }
}

/**
 * @brief 读取增量计数
 * @return 自上次读取后的脉冲增量(读取后清零)
 * 用于计算速度
 */
int32_t Encoder_GetDelta(EncoderChannel channel)
{
    int32_t delta;

    /** 读取时关中断，防止与中断竞争 */
    __disable_irq();
    delta = g_encoderCounts[channel];
    g_encoderCounts[channel] = 0;
    __enable_irq();

    return delta;
}

/**
 * @brief 读取累计计数(不清零)
 * 用于计算位置
 */
int32_t Encoder_GetTotal(EncoderChannel channel)
{
    int32_t total;

    __disable_irq();
    total = g_encoderTotals[channel];
    __enable_irq();

    return total;
}

/**
 * @brief 复位所有编码器
 * 清零增量计数和累计计数
 */
void Encoder_ResetAll(void)
{
    __disable_irq();
    g_encoderCounts[ENCODER_LEFT] = 0;
    g_encoderCounts[ENCODER_RIGHT] = 0;
    g_encoderTotals[ENCODER_LEFT] = 0;
    g_encoderTotals[ENCODER_RIGHT] = 0;
    __enable_irq();
}