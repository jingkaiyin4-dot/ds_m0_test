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

#include "ti_msp_dl_config.h"
#include "main.h"
#include "stdio.h"
#include "app/Grayscale.h"

static uint8_t oled_buffer[32];
static uint16_t tof400f_distance_mm;
static uint8_t tof400f_ok;
static uint8_t uart_cam_use_tof;
static uint8_t imu_use_bno08x;
static unsigned long now_ms;
static unsigned long imu_sample_ms;

/** PID控制标志，供 interrupt.c 的定时器中断读取 */
uint8_t drive_pid_active;
static AppButtonState g_modeButton;

static const AppBusDevice g_bno08xBus = { APP_BUS_UART, "BNO08X", 0U, 115200U };
static const AppBusDevice g_camBus = { APP_BUS_UART, "CAM", 0U, 115200U };
static const AppBusDevice g_tofBus = { APP_BUS_UART, "TOF400F", 0U, 115200U };
static const AppBusDevice g_mpuBus = { APP_BUS_I2C, "MPU6050", 0x68U, 400000U };

#define APP_KEY_DEBOUNCE_MS 30U

/** 目标行驶距离(圈数): 1圈=286counts */
#define APP_DRIVE_PID_TARGET_TURNS  3.0f
/** 到达判定误差(编码器计数)，小于此误差认为到达目标 */
#define APP_DRIVE_PID_STOP_ERROR    20.0f
/** 超限保护(编码器计数)，超过此值强制停止 */
#define APP_DRIVE_PID_ABORT_COUNTS  5000.0f

/* MG513XP26 减速电机参数: 需和 PID.c 保持一致 */
#define MG513XP26_ENCODER_COUNTS_PER_MOTOR_REV   (11.0f)   /** 电机轴每转脉冲数 */
#define MG513XP26_GEAR_RATIO                     (26.0f)   /** 减速比 */
#define CONTROL_SAMPLE_PERIOD_S                  (0.01f)  /** 控制周期10ms */

/**
 * @brief 启动双环PID控制
 * 1. 复位编码器
 * 2. 初始化PID控制器
 * 3. 清零历史状态
 * 4. 设置目标位置(编码器计数)
 */
static void App_StartDrivePidTest(void)
{
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
    g_driveController.left.position.target =
        APP_DRIVE_PID_TARGET_TURNS * g_driveController.countsPerMotorRev * g_driveController.gearRatio;
    g_driveController.right.position.target = g_driveController.left.position.target;

    /** 覆盖位置环PID系数(可选，如不需要可删除) */
    g_driveController.left.position.kp = 0.006f;
    g_driveController.left.position.ki = 0.0f;
    g_driveController.left.position.kd = 0.165f;
    g_driveController.right.position.kp = 0.006f;
    g_driveController.right.position.ki = 0.0f;
    g_driveController.right.position.kd = 0.165f;

    /** 启动PID控制 */
    drive_pid_active = 1U;
}

static void App_StopDrivePidTest(void)
{
    Motor_StopAll();
    drive_pid_active = 0U;
}

static void App_ApplyDisplayTheme(void)
{
    if (imu_use_bno08x != 0U) {
        OLED_SetTheme(OLED_COLOR_YELLOW, OLED_COLOR_BLUE);
    } else {
        OLED_SetTheme(OLED_COLOR_WHITE, OLED_COLOR_BLACK);
    }

    OLED_Clear();
    OLED_ShowString(0,0,(uint8_t *)"Pitch",8);
    OLED_ShowString(0,2,(uint8_t *)" Roll",8);
    OLED_ShowString(0,4,(uint8_t *)"  Yaw",8);
    OLED_ShowString(72,6,(uint8_t *)"C/R",8);
    if (uart_cam_use_tof != 0U) {
        OLED_ShowString(0,6,(uint8_t *)"TOF",8);
    } else {
        OLED_ShowString(0,6,(uint8_t *)"CAM",8);
    }
    if (imu_use_bno08x != 0U) {
        OLED_ShowString(0,7,(uint8_t *)"BNO08X",8);
        OLED_ShowString(0,8,(uint8_t *)"L CMD:",8);
        OLED_ShowString(0,9,(uint8_t *)"L ENC:",8);
        OLED_ShowString(0,10,(uint8_t *)"R CMD:",8);
        OLED_ShowString(0,11,(uint8_t *)"R ENC:",8);
    } else {
        OLED_ShowString(0,7,(uint8_t *)"MPU6050",8);
        OLED_ShowString(0,8,(uint8_t *)"L CMD:",8);
        OLED_ShowString(0,9,(uint8_t *)"L ENC:",8);
        OLED_ShowString(0,10,(uint8_t *)"R CMD:",8);
        OLED_ShowString(0,11,(uint8_t *)"R ENC:",8);
    }
}

static void App_ButtonUpdate(AppButtonState *button, uint8_t rawPressed, uint32_t now)
{
    static uint8_t lastSamplePressed = 0U;

    button->clicked = 0U;

    if (rawPressed != lastSamplePressed) {
        lastSamplePressed = rawPressed;
        button->stable_since_ms = now;
    }

    if ((now - button->stable_since_ms) >= APP_KEY_DEBOUNCE_MS) {
        if (button->pressed != rawPressed) {
            button->pressed = rawPressed;
            if (button->pressed != 0U) {
                button->clicked = 1U;
            }
        }
    }
}

static uint8_t App_KeyPollToggleImu(void)
{
    uint8_t samplePressed = (DL_GPIO_readPins(GPIO_KEY_PIN_KEY_MODE_PORT, GPIO_KEY_PIN_KEY_MODE_PIN) == 0U) ? 1U : 0U;

    App_ButtonUpdate(&g_modeButton, samplePressed, now_ms);
    return g_modeButton.clicked;
}

#ifndef APP_USE_BNO08X
#define APP_USE_BNO08X 1U
#endif

int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();
    OLED_Init();
    DriveController_Init();
    CameraControl_Init();
    TOF400F_Init();

    imu_use_bno08x = APP_USE_BNO08X;
    BNO08X_Init();
    MPU6050_Init();

    /* PA12 uses pull-up: low selects TOF, high selects CAM. */
    uart_cam_use_tof = (DL_GPIO_readPins(GPIO_TRANS_PORT, GPIO_TRANS_PIN_TRANS_CAM_TOF_PIN) == 0U) ? 1U : 0U;
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
    g_modeButton.pressed = 0U;
    g_modeButton.clicked = 0U;
    g_modeButton.stable_since_ms = 0U;
    App_ApplyDisplayTheme();
    if (imu_use_bno08x != 0U) {
        App_StartDrivePidTest();
    }
    if (uart_cam_use_tof != 0U) {
        TOF400F_Query();
    }

    static unsigned long tof400f_query_ms = 0;

    while (1)
    {
        mspm0_get_clock_ms(&now_ms);
        if (App_KeyPollToggleImu() != 0U) {
            imu_use_bno08x ^= 1U;
            if (imu_use_bno08x != 0U) {
                App_StartDrivePidTest();
            } else {
                App_StopDrivePidTest();
            }
            App_ApplyDisplayTheme();
        }

        if ((imu_use_bno08x == 0U) && (MPU6050_IsReady() != 0U)) {
            if ((MPU6050_HasPendingSample() != 0U) || ((now_ms - imu_sample_ms) >= 20U)) {
                MPU6050_ClearPendingSample();
                Read_Quad();
                imu_sample_ms = now_ms;
            }
        }

        if ((uart_cam_use_tof != 0U) && ((now_ms - tof400f_query_ms) >= 100U)) {
            tof400f_query_ms = now_ms;
            TOF400F_Query();
        }

        /* PID控制已移至 TIMA0 定时器中断(10ms)执行，此处不再调用 */

        /** 读取灰度传感器并在OLED上显示二进制(第13行) */
        uint8_t gray_val = Grayscale_ReadBinary();
        sprintf((char *)oled_buffer, "GRAY:%c%c%c%c%c%c%c%c",
            (gray_val & 0x80) ? '1' : '0',
            (gray_val & 0x40) ? '1' : '0',
            (gray_val & 0x20) ? '1' : '0',
            (gray_val & 0x10) ? '1' : '0',
            (gray_val & 0x08) ? '1' : '0',
            (gray_val & 0x04) ? '1' : '0',
            (gray_val & 0x02) ? '1' : '0',
            (gray_val & 0x01) ? '1' : '0');
        OLED_ShowString(0, 13, oled_buffer, 8);

        /** 如果PID控制激活，则将灰度值传入状态机计算转速 */
        if (drive_pid_active != 0U) {
            /** 强行关闭外环(位置环)，让底层只执行速度环，从而让 Grayscale 直接接管速度控制 */
            g_driveController.enablePositionLoop = 0;
            Grayscale_Process(gray_val, 4.0f); // 设定基础速度为 4.0 RPS
        }

        if (uart_cam_use_tof != 0U) {
            tof400f_ok = TOF400F_IsReady();

            if (tof400f_ok != 0U) {
                tof400f_distance_mm = TOF400F_ReadDistanceMm();
                sprintf((char *)oled_buffer, "%4umm", (unsigned int)tof400f_distance_mm);
            } else {
                sprintf((char *)oled_buffer, "I%u C%02X", (unsigned int)TOF400F_GetIrqCount(), TOF400F_GetLastChar());
            }
        } else {
            sprintf((char *)oled_buffer, "%c %02X/%1u", CameraControl_GetLastCommand(), CameraControl_GetPendingCommand(), CameraControl_IsStepperRunning());
        }
        OLED_ShowString(24,6,oled_buffer,8);

        if (imu_use_bno08x != 0U) {
            sprintf((char *)oled_buffer, "%-6.1f", bno08x_data.pitch);
        } else {
            if (MPU6050_IsReady() != 0U) {
                sprintf((char *)oled_buffer, "%-6.1f", mpu6050_euler.pitch);
            } else {
                sprintf((char *)oled_buffer, "W%02X A%02X", MPU6050_GetWhoAmI(), MPU6050_GetAddress());
            }
        }
        OLED_ShowString(5*8,0,oled_buffer,16);
        if (imu_use_bno08x != 0U) {
            sprintf((char *)oled_buffer, "%-6.1f", bno08x_data.roll);
        } else {
            if (MPU6050_IsReady() != 0U) {
                sprintf((char *)oled_buffer, "%-6.1f", mpu6050_euler.roll);
            } else {
                sprintf((char *)oled_buffer, "GY%5.1f", mpu6050_gyro.y_dps);
            }
        }
        OLED_ShowString(5*8,2,oled_buffer,16);
        if (imu_use_bno08x != 0U) {
            sprintf((char *)oled_buffer, "%-6.1f", bno08x_data.yaw);
        } else {
            if (MPU6050_IsReady() != 0U) {
                sprintf((char *)oled_buffer, "%-6.1f", mpu6050_euler.yaw);
            } else {
                sprintf((char *)oled_buffer, "RD%1u", MPU6050_IsReady());
            }
        }
        OLED_ShowString(5*8,4,oled_buffer,16);

        /* 显示左轮 */
        sprintf((char *)oled_buffer, "%7ld", (long) g_driveController.driveBase.left.state.encoder_total);
        OLED_ShowString(36,9,oled_buffer,8);

        /* 显示右轮 */
        sprintf((char *)oled_buffer, "%7ld", (long) g_driveController.driveBase.right.state.encoder_total);
        OLED_ShowString(36,11,oled_buffer,8);

        mspm0_delay_ms(5);
    }
}
