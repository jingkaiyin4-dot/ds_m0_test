/**
 * @file Encoder.h
 * @brief 编码器接口
 *
 * MG513XP26 编码器:
 * - AB相输出: A相触发中断计数, B相判断方向
 * - 11 PPR: 每转11个脉冲
 * - 增量计数模式: 每次中断更新计数
 */

#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <stdint.h>

/** 编码器通道枚举 */
typedef enum {
    ENCODER_LEFT = 0,   /** 左轮编码器 */
    ENCODER_RIGHT = 1,   /** 右轮编码器 */
} EncoderChannel;

/** GPIO中断入口(编码器A相中断处理) */
void GROUP1_IRQHandler(void);

/**
 * @brief 读取增量计数
 * @return 自上次读取以来的脉冲数(读取后自动清零)
 * 用于计算速度
 */
int32_t Encoder_GetDelta(EncoderChannel channel);

/**
 * @brief 读取累计总计数
 * @return 累计总脉冲数(不清零)
 * 用于计算位置/里程
 */
int32_t Encoder_GetTotal(EncoderChannel channel);

/**
 * @brief 复位所有编码器
 * 同时清零增量计数和累计计数
 */
void Encoder_ResetAll(void);

#endif