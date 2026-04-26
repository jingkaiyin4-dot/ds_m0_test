/**
 * @file PID.c
 * @brief 双环PID控制器实现
 *
 * 减速电机 MG513XP26 双环PID控制:
 * - 外环(位置环): 目标位置(编码器计数) → 目标速度(RPS)
 * - 内环(速度环): 目标速度(RPS) → 电机占空比(duty)
 *
 * 控制流程(每10ms执行一次):
 * 1. 读取编码器增量，计算当前速度
 * 2. 位置环PID: 当前位置 → 目标速度
 * 3. 速度环PID: 当前速度 → duty
 * 4. 输出duty到电机
 */

#include "PID.h"
#include "../Drivers/MPU6050/mpu6050.h"
#include "Encoder.h"
#include "Motor.h"
#include "bno08x_uart_rvc.h"
#include "ti_msp_dl_config.h"

#include <math.h>

/**
 * MG513XP26 减速电机参数:
 * - 11 PPR: 每转11个脉冲(电机轴)
 * - 26:1 减速比: 电机转26圈，车轮转1圈
 * - 车轮转一圈 = 11 * 26 = 286 脉冲
 */
#define MG513XP26_ENCODER_COUNTS_PER_MOTOR_REV (11.0f) /** 电机轴每转脉冲数 */
#define MG513XP26_GEAR_RATIO (26.0f)                   /** 减速比(电机:车轮) */
#define CONTROL_SAMPLE_PERIOD_S (0.01f)                /** 控制周期10ms */

void DriveController_UpdateImu(float pitch, float roll, float yaw, float gx,
                               float gy, float gz) {
  g_driveController.balance.imu.pitch = pitch;
  g_driveController.balance.imu.roll = roll;
  g_driveController.balance.imu.yaw = yaw;
  g_driveController.balance.imu.gyroX = gx;
  g_driveController.balance.imu.gyroY = gy;
  g_driveController.balance.imu.gyroZ = gz;
}

DriveController g_driveController;

/**
 * @brief 单环PID计算
 * @param pid PID控制器结构体(包含参数和状态)
 * @param actual 当前实际值(位置或速度)
 * @return PID输出(位置环=目标速度, 速度环=duty)
 *
 * 离散PID公式:
 * output = kp * error + ki * integral + kd * derivative
 * error = target - actual
 * integral = Σerror * dt (本函数中 dt=1，由 samplePeriodS 在外部换算)
 * derivative = (error - prevError) / dt
 */
static float PIDController_Update(PIDController *pid, float actual) {
  float error = pid->target - actual;
  /** 修改为实际值的负增量，防止目标值改变导致的微分突变(Derivative Kick) */
  float derivative = -(actual - pid->prevActual);

  pid->actual = actual;
  pid->integral += error;

  /** 积分限幅，防止积分饱和 */
  if (pid->integral > pid->integralLimit) {
    pid->integral = pid->integralLimit;
  } else if (pid->integral < -pid->integralLimit) {
    pid->integral = -pid->integralLimit;
  }

  pid->output =
      pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

  /** 输出限幅 */
  if (pid->output > pid->outputLimit) {
    pid->output = pid->outputLimit;
  } else if (pid->output < -pid->outputLimit) {
    pid->output = -pid->outputLimit;
  }

  pid->prevError = error;
  pid->prevActual = actual; /** 记录上次的实际值 */
  return pid->output;
}

/**
 * @brief 单车轮双环PID控制更新
 * @param wheel 车轮控制器(包含位置环和速度环PID)
 * @param driveWheel 底层驱动状态(电机+编码器)
 * @param directionSign 编码器方向修正(前进时计数为正)
 * @param motorOutputSign 电机输出方向修正(正duty对应前进)
 * @param drive 驱动控制器全局参数
 *
 * 控制流程:
 * 1. 读取编码器增量，换算车轮转速
 * 2. 位置环PID: 当前位置 → 目标速度
 * 3. 速度环PID: 当前速度 → duty
 * 4. 输出duty到电机PWM
 */
static void WheelController_Update(WheelController *wheel,
                                   DriveWheel *driveWheel, float directionSign,
                                   float motorOutputSign,
                                   const DriveController *drive) {
  /** 编码器增量 × 方向修正 = 实际位移 */
  float delta = (float)driveWheel->state.encoder_delta * directionSign;

  /** 电机轴每秒转速 = 增量 / (每转脉冲数 × 控制周期)
   * 车轮转速 = 电机轴转速 / 减速比 */
  float motorRps =
      ((float)delta) / drive->countsPerMotorRev / drive->samplePeriodS;
  float rawSpeedRps = motorRps / drive->gearRatio;

  /** 增加一阶低通滤波: 消除低分辨率编码器带来的量化噪声，防止低速抖动 */
  wheel->speedRps = 0.3f * rawSpeedRps + 0.7f * wheel->speedRps;

  wheel->positionCounts += delta;

  /** 如果启用了位置环(双环控制) */
  if (drive->enablePositionLoop) {
    /** 外环位置PID: 输入当前位置，输出目标速度(RPS) */
    PIDController_Update(&wheel->position, wheel->positionCounts);
    wheel->speed.target = wheel->position.output;
  }
  /** 如果未启用位置环，此时处于纯速度控制模式(单环)，
   *  wheel->speed.target 会由外界(比如循迹逻辑)直接设定，不受位置环覆盖。
   */

  if (drive->enableBalanceLoop) {
    /** [关键修复]
     * 当调试直立环时，必须强制关闭底层速度环，否则速度环(目标为0)会产生反向阻力(滞后感)！
     */
    wheel->duty = 0.0f;
  } else {
    /** 内环速度PID: 输入当前速度，输出电机占空比(%) */
    wheel->duty = PIDController_Update(&wheel->speed, wheel->speedRps);
  }

  /** duty × 输出方向修正 → 实际PWM */
  Motor_SetDuty(driveWheel->motor,
                (int16_t)lroundf(wheel->duty * motorOutputSign));
}

/**
 * @brief 初始化驱动控制器
 * 设置电机参数和PID系数
 */
void DriveController_Init(void) {
  DriveBase_Init(&g_driveController.driveBase);
  g_driveController.countsPerMotorRev = MG513XP26_ENCODER_COUNTS_PER_MOTOR_REV;
  g_driveController.gearRatio = MG513XP26_GEAR_RATIO;
  g_driveController.samplePeriodS = CONTROL_SAMPLE_PERIOD_S;
  g_driveController.leftDirectionSign = 1.0f;
  g_driveController.rightDirectionSign = 1.0f;
  g_driveController.leftMotorOutputSign = 1.0f;
  g_driveController.rightMotorOutputSign = 1.0f;

  /** 默认开启位置环(双环控制) */
  g_driveController.enablePositionLoop = 1;

  /**
   * 速度环参数(内环):
   * - target: 目标转速(RPS车轮转速)
   * - output: 电机占空比(%)
   * - kp=1.8: 比例响应
   * - ki=0.11: 消除稳态误差
   * - kd=0.015: 抑制振荡
   * - outputLimit=100: 最大100%占空比
   */
  g_driveController.left.speed = (PIDController){
      .kp = 1.8f,
      .ki = 0.11f,
      .kd = 0.015f,
      .integralLimit = 1000.0f,
      .outputLimit = 100.0f,
  };
  g_driveController.right.speed = g_driveController.left.speed;

  /**
   * 位置环参数(外环):
   * - target: 目标累计计数(由main.c设置)
   * - output: 目标速度(RPS)
   * - kp=0.006: 位置增益
   * - ki=0.0005: 积分消除误差
   * - kd=0.165: 微分抑制超调
   * - outputLimit=10: 最大10RPS(防止速度冲太高)
   */
  g_driveController.left.position = (PIDController){
      .kp = 0.006f,
      .ki = 0.0f, /** 将位置环积分清零，防止由于累计过量导致的超调震荡 */
      .kd = 0.165f,
      .target = 0.0f,
      .integralLimit = 1000.0f,
      .outputLimit = 10.0f,
      .prevActual = 0.0f,
  };
  g_driveController.right.position = g_driveController.left.position;

  /**
   * 直立平衡环参数 (初值建议):
   * - 目标: 机械中值 (需要实测，通常在 0 附近)
   * - kp: 决定站立力度
   * - kd: 决定阻尼，防止震荡
   */
  g_driveController.balance.mechanicalMedian = -4.6f;
  g_driveController.balance.pid = (PIDController){
      .kp = -5.45f, 
      .ki = -0.01f,
      .kd = -0.20f,  // 大幅增加阻尼，压制前后晃动
      .target = 0.0f,
      .integralLimit = 5.0f,
      .outputLimit = 100.0f,
  };

  /**
   * 速度外环参数:
   * - 目标: 0 (保持静止)
   * - kp: 产生阻力，防止跑偏。注意极性！
   * - ki: 消除长期位移误差
   */
  g_driveController.balance.velocityPid = (PIDController){
      .kp = -4.10f,
      .ki = -0.01f,
      .kd = -0.20, 
      .target = 0.0f,
      .integralLimit = 5.0f,
      .outputLimit = 40.0f,
  };

  g_driveController.enableBalanceLoop = 0; // 默认不开启
}

/**
 * @brief 执行双环PID控制
 * 每个控制周期(10ms)调用一次
 * 同时更新左右轮
 */
void double_pid(void) {
  float balance_out = 0.0f;

  /** 1. 从底层刷新电机/编码器状态 */
  DriveBase_UpdateState(&g_driveController.driveBase);

  /** 2. 左轮双环PID (目前会刷新速度，但强制设 duty=0) */
  WheelController_Update(
      &g_driveController.left, &g_driveController.driveBase.left,
      g_driveController.leftDirectionSign,
      g_driveController.leftMotorOutputSign, &g_driveController);

  /** 3. 右轮双环PID */
  WheelController_Update(
      &g_driveController.right, &g_driveController.driveBase.right,
      g_driveController.rightDirectionSign,
      g_driveController.rightMotorOutputSign, &g_driveController);

  /** 4. 如果开启了平衡环，计算串级 PID */
  if (g_driveController.enableBalanceLoop) {
    // 强制从 BNO08X 的 DMA 缓存中拉取最新数据
    extern uint8_t imu_use_bno08x;
    if (imu_use_bno08x) {
      DriveController_UpdateImu(bno08x_data.pitch, bno08x_data.roll,
                                bno08x_data.yaw, 0.0f, 0.0f, 0.0f);
    }

    /** 【速度外环】 每 50ms 计算一次 (5个10ms周期) */
    static uint8_t velocity_divider = 0;
    if (++velocity_divider >= 5) {
      velocity_divider = 0;
      
      float avg_speed = (g_driveController.left.speedRps + g_driveController.right.speedRps) * 0.5f;
      /** 这里的 speedRps 向前为负
       * error = target(0) - speed(-x) = +x. kp为正 -> output为正 -> 目标角度变大(后仰) ✓
       */
      PIDController_Update(&g_driveController.balance.velocityPid, avg_speed);
      
      /** 速度环输出直接修改直立环的目标角度 */
      g_driveController.balance.pid.target = g_driveController.balance.mechanicalMedian + 
                                             g_driveController.balance.velocityPid.output;
    }

    /** 【直立内环】 每 10ms 执行一次 */
    float current_pitch = g_driveController.balance.imu.pitch;
    float error = current_pitch - g_driveController.balance.pid.target;

    /** BNO08X 角速度滤波计算 */
    static float prev_pitch = 0.0f;
    static float filtered_gyroX = 0.0f;
    float current_gyroX = g_driveController.balance.imu.gyroX;

    if (current_gyroX == 0.0f) {
      float raw_gyro = (current_pitch - prev_pitch) / CONTROL_SAMPLE_PERIOD_S;
      filtered_gyroX = 0.8f * raw_gyro + 0.2f * filtered_gyroX;
      current_gyroX = filtered_gyroX;
    }
    prev_pitch = current_pitch;

    /** 计算平衡占空比 (内环输出) */
    balance_out = (g_driveController.balance.pid.kp * error) +
                  (g_driveController.balance.pid.kd * current_gyroX);
    
    g_driveController.balance.pid.output = balance_out;
  }

  /** 5. 如果有平衡环输出，将其叠加到左右轮的最终 duty 上 */
  if (g_driveController.enableBalanceLoop) {
    /**
     * 这里使用 += 加法 (正向逻辑)。
     * 如果向前倒，应该输出正值让轮子向前调。
     * 如果轮子还是向后足起，把 kp 改为负数 (-8.0)。
     */
    g_driveController.left.duty += balance_out;
    g_driveController.right.duty += balance_out;

    // 限制叠加后的占空比
    if (g_driveController.left.duty > 100.0f)
      g_driveController.left.duty = 100.0f;
    if (g_driveController.left.duty < -100.0f)
      g_driveController.left.duty = -100.0f;
    if (g_driveController.right.duty > 100.0f)
      g_driveController.right.duty = 100.0f;
    if (g_driveController.right.duty < -100.0f)
      g_driveController.right.duty = -100.0f;

    // 立即刷新电机输出 (因为 WheelController_Update 已经调用过一次
    // SetDuty，这里需要覆盖)
    Motor_SetDuty(g_driveController.driveBase.left.motor,
                  (int16_t)lroundf(g_driveController.left.duty *
                                   g_driveController.leftMotorOutputSign));
    Motor_SetDuty(g_driveController.driveBase.right.motor,
                  (int16_t)lroundf(g_driveController.right.duty *
                                   g_driveController.rightMotorOutputSign));
  }
}