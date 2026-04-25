/**
 * @file Motor.h
 * @brief 电机驱动接口
 *
 * MG513XP26 减速电机驱动:
 * - PWM控制占空比: -100% ~ 100%，负值反转
 * - 使用TIMER输出的PWM控制电机转速
 */

#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdint.h>
#include "app/AppTypes.h"

/** 电机通道枚举 */
typedef enum {
    MOTOR_LEFT = 0,   /** 左轮电机 */
    MOTOR_RIGHT = 1,   /** 右轮电机 */
} MotorChannel;

/**
 * @brief 设置电机占空比
 * @param channel 电机通道(MOTOR_LEFT / MOTOR_RIGHT)
 * @param duty 占空比(-100 ~ 100)，负值反转
 *
 * duty数值含义:
 * - 100: 正向最大转速
 * - 0: 停止
 * - -100: 反向最大转速
 */
void Motor_SetDuty(MotorChannel channel, int16_t duty);

/**
 * @brief 停止所有电机
 */
void Motor_StopAll(void);

/**
 * @brief 读取最近一次设置的duty值
 */
int16_t Motor_GetDuty(MotorChannel channel);

/**
 * @brief 填充驱动状态给DriveBase使用
 * @param channel 电机通道
 * @param state 状态结构体指针
 */
void Motor_FillDriveState(MotorChannel channel, DriveMotorState *state);

/**
 * @brief 兼容旧接口
 */
void SetPWM_and_Motor(int Channel, int Duty);

#endif