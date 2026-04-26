/**
 * @file PID.h
 * @brief 双环PID控制器接口
 *
 * 减速电机 MG513XP26 双环PID控制:
 * - 外环(位置环): 输入当前位置(编码器累计)，输出目标速度(RPS)
 * - 内环(速度环): 输入当前速度(RPS)，输出电机占空比(duty)
 *
 * 硬件参数:
 * - MG513XP26: 11脉冲/电机轴转, 26:1减速比
 * - 车轮转一圈 = 11 * 26 = 286 脉冲
 * - 控制周期: 10ms (0.01s)
 */

#ifndef _PID_H_
#define _PID_H_

#include "app/DriveBase.h"
#include <stdint.h>

typedef struct {
    /** 比例系数: 误差增大时增强响应 */
    float kp;
    /** 积分系数: 消除稳态误差 */
    float ki;
    /** 微分系数: 抑制超调振荡 */
    float kd;
    /** 目标值: 位置环=目标编码器数, 速度环=目标转速(RPS) */
    float target;
    /** 实际值: 当前位置或当前速度 */
    float actual;
    /** PID输出: 位置环=目标速度, 速度环=duty */
    float output;
    float prevError;
    float prevActual;      /** 用于微分踢除: 记录上一次的实际值 */
    float integral;
    float integralLimit;
    float outputLimit;
} PIDController;

/**
 * @brief 姿态数据结构体
 */
typedef struct {
    float yaw;
    float pitch;
    float roll;
    float gyroX;           /** 角速度 (用于平衡环微分) */
    float gyroY;
    float gyroZ;
} ImuData;

/**
 * @brief 直立平衡环控制器
 */
typedef struct {
    PIDController pid;         /** 直立环 PID (通常只需要 PD) */
    PIDController velocityPid; /** 速度外环 PID (通常只需要 PI) */
    float mechanicalMedian;    /** 机械中值 (车体垂直时的角度) */
    ImuData imu;               /** 当前 IMU 数据状态 */
} BalanceController;

typedef struct {
    /** 外环位置PID: target=目标位置(计数), output=目标速度(RPS) */
    PIDController position;
    /** 内环速度PID: target=目标速度(RPS), output=duty(%) */
    PIDController speed;
    /** 当前车轮转速(输出轴, RPS) */
    float speedRps;
    /** 累计位置(编码器计数) */
    float positionCounts;
    /** 当前输出到电机的占空比 */
    float duty;
} WheelController;

typedef struct {
    /** 左轮双环PID控制器 */
    WheelController left;
    /** 右轮双环PID控制器 */
    WheelController right;
    /** 底层驱动抽象 */
    DriveBase driveBase;
    /** 电机轴每转脉冲数: MG513XP26=11 */
    float countsPerMotorRev;
    /** 减速比: MG513XP26=26 */
    float gearRatio;
    /** 控制周期(秒): 0.01s = 10ms */
    float samplePeriodS;
    /** 左编码器方向修正: 机械安装导致计数方向与实际前进方向不一致时使用 */
    float leftDirectionSign;
    /** 右编码器方向修正 */
    float rightDirectionSign;
    /** 左电机输出方向修正: 电机正转时实际前进/后退方向不一致时使用 */
    float leftMotorOutputSign;
    /** 右电机输出方向修正 */
    float rightMotorOutputSign;
    /** 是否启用位置环(外环): 1=启用双环, 0=仅启用速度环(由外部直接设定speed.target) */
    uint8_t enablePositionLoop;
    /** 直立平衡环控制 */
    BalanceController balance;
    /** 是否启用直立环: 1=平衡模式, 0=普通模式 */
    uint8_t enableBalanceLoop;
} DriveController;

extern DriveController g_driveController;

/**
 * @brief 从外部更新控制器内部的 IMU 数据
 */
void DriveController_UpdateImu(float pitch, float roll, float yaw, float gx, float gy, float gz);

/**
 * @brief 初始化双环PID控制器
 * 设置电机参数和PID系数
 */
void DriveController_Init(void);

/**
 * @brief 执行一次全系统PID控制
 * 每个控制周期(10ms)调用一次
 */
void double_pid(void);

#endif