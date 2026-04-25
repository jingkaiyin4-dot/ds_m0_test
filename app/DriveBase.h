/**
 * @file DriveBase.h
 * @brief 底盘驱动抽象层
 *
 * 承上启下层:
 * - 上层: 双环PID控制器
 * - 下层: Motor + Encoder
 * 抽象出"车轮"概念，统一左右轮操作
 */

#ifndef _DRIVE_BASE_H_
#define _DRIVE_BASE_H_

#include "app/AppTypes.h"
#include "encoder/Encoder.h"
#include "motor/Motor.h"

#include <stdint.h>

/** 单个车轮抽象 */
typedef struct {
    const char *name;           /** 名字("left"/"right") */
    MotorChannel motor;         /** 绑定的电机通道 */
    EncoderChannel encoder;   /** 绑定的编码器通道 */
    DriveMotorState state;    /** 当前状态(duty+编码器) */
} DriveWheel;

/** 底盘抽象 */
typedef struct {
    DriveWheel left;   /** 左轮 */
    DriveWheel right;  /** 右轮 */
} DriveBase;

/** 初始化底盘，绑定电机和编码器 */
void DriveBase_Init(DriveBase *driveBase);

/** 刷新左右轮状态(每个控制周期) */
void DriveBase_UpdateState(DriveBase *driveBase);

#endif