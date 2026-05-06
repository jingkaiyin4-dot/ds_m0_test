/*
 * Copyright (c) 2021, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "main.h"
#include "app/AppUi.h"
#include "app/Grayscale.h"
#include "stdio.h"
#include "ti_msp_dl_config.h"

static uint8_t oled_buffer[32];
static uint16_t tof400f_distance_mm;
static uint8_t tof400f_ok;
static uint8_t uart_cam_use_tof;
uint8_t imu_use_bno08x;
static unsigned long now_ms;
static unsigned long imu_sample_ms;

uint8_t drive_pid_active;

static const AppBusDevice g_bno08xBus = {APP_BUS_UART, "BNO08X", 0U, 115200U};
static const AppBusDevice g_camBus = {APP_BUS_UART, "CAM", 0U, 115200U};
static const AppBusDevice g_tofBus = {APP_BUS_UART, "TOF400F", 0U, 115200U};
static const AppBusDevice g_mpuBus = {APP_BUS_I2C, "MPU6050", 0x68U, 400000U};

/** 目标行驶距离(圈数): 1圈=286counts */
#define APP_DRIVE_PID_TARGET_TURNS 3.0f
/** 到达判定误差(编码器计数)，小于此误差认为到达目标 */
#define APP_DRIVE_PID_STOP_ERROR 20.0f
/** 超限保护(编码器计数)，超过此值强制停止 */
#define APP_DRIVE_PID_ABORT_COUNTS 5000.0f

/* MG513XP26 减速电机参数: 需和 PID.c 保持一致 */
#define MG513XP26_ENCODER_COUNTS_PER_MOTOR_REV (11.0f) /** 电机轴每转脉冲数 */
#define MG513XP26_GEAR_RATIO (26.0f)                   /** 减速比 */
#define CONTROL_SAMPLE_PERIOD_S (0.01f)                /** 控制周期10ms */

/**
 * @brief 启动双环PID控制
 * 1. 复位编码器
 * 2. 初始化PID控制器
 * 3. 清零历史状态
 * 4. 设置目标位置(编码器计数)
 */
void App_StartDrivePidTest(void) {
  /** 1. 复位编码器 */
  Encoder_ResetAll();

  /** 2. 初始化PID控制器(在PID.c中设置电机参数和PID系数) */
  DriveController_Init();

  /** 3. 清零左右轮PID历史状态 */
  g_driveController.left.positionCounts = 0.0f;
  g_driveController.right.positionCounts = 0.0f;
  g_driveController.left.speed.actual = 0.0f;
  g_driveController.left.speed.integral = 0.0f;
  g_driveController.left.speed.prevError = 0.0f;
  g_driveController.left.speed.prevActual = 0.0f;
  g_driveController.left.speed.output = 0.0f;
  g_driveController.left.position.actual = 0.0f;
  g_driveController.left.position.integral = 0.0f;
  g_driveController.left.position.prevError = 0.0f;
  g_driveController.left.position.prevActual = 0.0f;
  g_driveController.left.position.output = 0.0f;
  g_driveController.right.speed.actual = 0.0f;
  g_driveController.right.speed.integral = 0.0f;
  g_driveController.right.speed.prevError = 0.0f;
  g_driveController.right.speed.prevActual = 0.0f;
  g_driveController.right.speed.output = 0.0f;
  g_driveController.right.position.actual = 0.0f;
  g_driveController.right.position.integral = 0.0f;
  g_driveController.right.position.prevError = 0.0f;
  g_driveController.right.position.prevActual = 0.0f;
  g_driveController.right.position.output = 0.0f;

  /** 4. 设置目标位置
   * 目标(编码器计数) = 目标(圈数) × 每圈脉冲数
   * 3圈 × 286 = 858 counts
   */
  g_driveController.left.position.target = APP_DRIVE_PID_TARGET_TURNS *
                                           g_driveController.countsPerMotorRev *
                                           g_driveController.gearRatio;
  g_driveController.right.position.target =
      g_driveController.left.position.target;

  /** 覆盖位置环PID系数(可选，如不需要可删除) */
  g_driveController.left.position.kp = 0.006f;
  g_driveController.left.position.ki = 0.0f;
  g_driveController.left.position.kd = 0.165f;
  g_driveController.right.position.kp = 0.006f;
  g_driveController.right.position.ki = 0.0f;
  g_driveController.right.position.kd = 0.165f;

  /** 开启平衡环，关闭位置环 */
  g_driveController.enablePositionLoop = 0;
  g_driveController.enableBalanceLoop = 1;

  /** 启动PID控制 */
  drive_pid_active = 1U;
}

void App_StopDrivePidTest(void) {
  Motor_StopAll();
  g_driveController.enableBalanceLoop = 0;
  drive_pid_active = 0U;
}

void App_ResetDriveState(void) {
  Encoder_ResetAll();
  g_driveController.left.positionCounts = 0.0f;
  g_driveController.right.positionCounts = 0.0f;
  g_driveController.left.speedRps = 0.0f;
  g_driveController.right.speedRps = 0.0f;
  g_driveController.left.duty = 0.0f;
  g_driveController.right.duty = 0.0f;
  g_driveController.balance.pid.output = 0.0f;
  g_driveController.balance.velocityPid.output = 0.0f;
}

void App_SetImuSource(uint8_t useBno08x) {
  imu_use_bno08x = (useBno08x != 0U) ? 1U : 0U;
}

uint8_t App_GetImuSource(void) {
  return imu_use_bno08x;
}

uint8_t App_IsDrivePidActive(void) {
  return drive_pid_active;
}

uint8_t App_IsTofUartEnabled(void) {
  return uart_cam_use_tof;
}

#ifndef APP_USE_BNO08X
#define APP_USE_BNO08X 1U
#endif

int main(void) {
  SYSCFG_DL_init();
  SysTick_Init();
  OLED_Init();
  DriveController_Init();
  CameraControl_Init();
  TOF400F_Init();
  AppUi_Init();

  imu_use_bno08x = APP_USE_BNO08X;
  BNO08X_Init();
  MPU6050_Init();

  /* PA12 uses pull-up: low selects TOF, high selects CAM. */
  /** PA12 原本用于 TOF 切换，现在已挪作灰度第 7 路使用，因此禁用此检测 */
  // uart_cam_use_tof = (DL_GPIO_readPins(GPIO_TRANS_PORT,
  // GPIO_TRANS_PIN_TRANS_CAM_TOF_PIN) == 0U) ? 1U : 0U;
  uart_cam_use_tof = 0U;
  if (uart_cam_use_tof != 0U) {
    CameraControl_DisableUart();
    TOF400F_EnableUart();
  } else {
    TOF400F_DisableUart();
    CameraControl_EnableUart();
  }

  enable_group1_irq = 1U;
  Interrupt_Init();

  tof400f_ok = 0;
  imu_sample_ms = 0U;
  drive_pid_active = 0U;
  if (imu_use_bno08x != 0U) {
    App_StartDrivePidTest();
  }
  if (uart_cam_use_tof != 0U) {
    TOF400F_Query();
  }

  static unsigned long tof400f_query_ms = 0;

  while (1) {
    mspm0_get_clock_ms(&now_ms);

    if (imu_use_bno08x != 0U) {
      DriveController_UpdateImu(bno08x_data.pitch, bno08x_data.roll,
                                bno08x_data.yaw, 0.0f, 0.0f, 0.0f);
    } else if (MPU6050_IsReady() != 0U) {
      if ((MPU6050_HasPendingSample() != 0U) ||
          ((now_ms - imu_sample_ms) >= 20U)) {
        MPU6050_ClearPendingSample();
        Read_Quad();
        DriveController_UpdateImu(mpu6050_euler.pitch, mpu6050_euler.roll,
                                  mpu6050_euler.yaw, mpu6050_gyro.x_dps,
                                  mpu6050_gyro.y_dps, mpu6050_gyro.z_dps);
        imu_sample_ms = now_ms;
      }
    }

    if ((uart_cam_use_tof != 0U) && ((now_ms - tof400f_query_ms) >= 100U)) {
      tof400f_query_ms = now_ms;
      TOF400F_Query();
    }

    AppUi_Update((uint32_t)now_ms);

    mspm0_delay_ms(5);
  }
}
