/**
 * @file DriveBase.c
 * @brief 底盘驱动实现
 *
 * 承上启下层:
 * - 上层: PID控制器 → 调用这里
 * - 下层: Motor/Encoder ← 从这里调用
 *
 * 每个控制周期:
 * 1. 刷新左轮状态(duty + 编码器)
 * 2. 刷新右轮状态
 */

#include "DriveBase.h"

/**
 * @brief 初始化底盘
 * 绑定左右轮的电机和编码器通道
 */
void DriveBase_Init(DriveBase *driveBase)
{
    if (driveBase == 0) {
        return;
    }

    /** 左轮绑定 */
    driveBase->left.name = "left";
    driveBase->left.motor = MOTOR_LEFT;
    driveBase->left.encoder = ENCODER_LEFT;
    driveBase->left.state.duty = 0;
    driveBase->left.state.encoder_delta = 0;
    driveBase->left.state.encoder_total = 0;

    /** 右轮绑定 */
    driveBase->right.name = "right";
    driveBase->right.motor = MOTOR_RIGHT;
    driveBase->right.encoder = ENCODER_RIGHT;
    driveBase->right.state.duty = 0;
    driveBase->right.state.encoder_delta = 0;
    driveBase->right.state.encoder_total = 0;
}

/**
 * @brief 刷新底盘状态
 * 每个控制周期(10ms)调用一次
 * 从Motor/Encoder底层读取最新状态
 */
void DriveBase_UpdateState(DriveBase *driveBase)
{
    if (driveBase == 0) {
        return;
    }

    /** 左轮 */
    Motor_FillDriveState(driveBase->left.motor, &driveBase->left.state);

    /** 右轮 */
    Motor_FillDriveState(driveBase->right.motor, &driveBase->right.state);
}