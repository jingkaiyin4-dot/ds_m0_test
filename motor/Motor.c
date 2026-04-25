/**
 * @file Motor.c
 * @brief 电机驱动实现
 *
 * MG513XP26 减速电机驱动:
 * - 使用TIMER输出的PWM控制电机转速
 * - 占空比控制: -100 ~ 100%
 * - IN1/IN2 控制方向
 */

#include "ti_msp_dl_config.h"
#include "Motor.h"
#include "Encoder.h"

#include <stdbool.h>

/** PWM定时器是否已启动 */
static bool g_motorTimerStarted;
/** 软件侧记录的duty值(供OLED显示) */
static int16_t g_motorDuty[2];

/**
 * @brief 设置PWM比较值
 * @param channel 电机通道
 * @param dutyPercent 0-100的占空比
 *
 * TIMER周期是7999，比较值 = 7999 - (7999 * duty% / 100)
 */
static void Motor_SetPwmCompare(MotorChannel channel, uint16_t dutyPercent)
{
    uint32_t compareValue = 7999U - ((7999U * dutyPercent) / 100U);

    if (channel == MOTOR_RIGHT) {
        DL_TimerA_setCaptureCompareValue(Motor_INST, compareValue, GPIO_Motor_C0_IDX);
    } else {
        DL_TimerA_setCaptureCompareValue(Motor_INST, compareValue, GPIO_Motor_C1_IDX);
    }
}

/**
 * @brief 设置电机方向
 * @param channel 电机通道
 * @param forward true=正向, false=反向
 *
 * IN1/IN2逻辑:
 * - 正向: IN1=1, IN2=0
 * - 反向: IN1=0, IN2=1
 * 左右电机接线方向不完全对称，需分别处理
 */
static void Motor_SetDirection(MotorChannel channel, bool forward)
{
    if (channel == MOTOR_RIGHT) {
        if (forward) {
            DL_GPIO_setPins(MOTOR_MOTOR_LA_PORT, MOTOR_MOTOR_LA_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_LB_PORT, MOTOR_MOTOR_LB_PIN);
        } else {
            DL_GPIO_setPins(MOTOR_MOTOR_LB_PORT, MOTOR_MOTOR_LB_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_LA_PORT, MOTOR_MOTOR_LA_PIN);
        }
    } else {
        if (forward) {
            DL_GPIO_setPins(MOTOR_MOTOR_RB_PORT, MOTOR_MOTOR_RB_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_RA_PORT, MOTOR_MOTOR_RA_PIN);
        } else {
            DL_GPIO_setPins(MOTOR_MOTOR_RA_PORT, MOTOR_MOTOR_RA_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_RB_PORT, MOTOR_MOTOR_RB_PIN);
        }
    }
}

/**
 * @brief 设置电机占空比
 * @param channel 电机通道
 * @param duty 占空比(-100 ~ 100)
 *
 * 正数: 正向转动
 * 负数: 反向转动
 * 0: 停止
 */
void Motor_SetDuty(MotorChannel channel, int16_t duty)
{
    bool forward;
    uint16_t magnitude;

    /** 第一次输出时启动PWM定时器 */
    if (!g_motorTimerStarted) {
        DL_Timer_startCounter(Motor_INST);
        g_motorTimerStarted = true;
    }

    /** 软件限幅 */
    if (duty > 100) {
        duty = 100;
    } else if (duty < -100) {
        duty = -100;
    }

    /** 符号转方向，绝对值转PWM值 */
    forward = (duty >= 0);
    magnitude = (uint16_t) (forward ? duty : -duty);

    Motor_SetDirection(channel, forward);
    Motor_SetPwmCompare(channel, magnitude);
    g_motorDuty[channel] = duty;
}

/**
 * @brief 停止所有电机
 */
void Motor_StopAll(void)
{
    Motor_SetDuty(MOTOR_LEFT, 0);
    Motor_SetDuty(MOTOR_RIGHT, 0);
}

/**
 * @brief 读取duty值
 */
int16_t Motor_GetDuty(MotorChannel channel)
{
    return g_motorDuty[channel];
}

/**
 * @brief 填充驱动状态给DriveBase
 * @param channel 电机通道
 * @param state 状态结构体
 *
 * 打包当前电机输出状态+编码器状态，供PID读取
 */
void Motor_FillDriveState(MotorChannel channel, DriveMotorState *state)
{
    if (state == 0) {
        return;
    }

    state->duty = g_motorDuty[channel];
    state->encoder_delta = Encoder_GetDelta((channel == MOTOR_LEFT) ? ENCODER_LEFT : ENCODER_RIGHT);
    state->encoder_total = Encoder_GetTotal((channel == MOTOR_LEFT) ? ENCODER_LEFT : ENCODER_RIGHT);
}

/**
 * @brief 兼容旧接口
 */
void SetPWM_and_Motor(int Channel, int Duty)
{
    Motor_SetDuty((Channel == 0) ? MOTOR_LEFT : MOTOR_RIGHT, (int16_t) Duty);
}