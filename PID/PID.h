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
    /** 积分累加器: 累加历史误差 */
    float integral;
    /** 上次误差: 原代码用于计算微分项，现已弃用，但为了兼容性保留，或可直接删除。由于需要存储上次实际值来防止微分突变，增加下述变量： */
    float prevError;
    /** 上次实际值: 改进后的微分项计算需要 */
    float prevActual;
    /** PID输出: 位置环=目标速度, 速度环=duty */
    float output;
    /** 积分饱和限幅: 防止积分饱和 */
    float integralLimit;
    /** 输出限幅: 位置环限制最大速度, 速度环限制最大duty */
    float outputLimit;
} PIDController;

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
} DriveController;

extern DriveController g_driveController;

/**
 * @brief 初始化双环PID控制器
 * 设置电机参数和PID系数
 */
void DriveController_Init(void);

/**
 * @brief 执行一次双环PID控制
 * 每个控制周期(10ms)调用一次
 */
void double_pid(void);

#endif