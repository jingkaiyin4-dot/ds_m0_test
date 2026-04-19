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

static uint8_t oled_buffer[32];
static uint16_t tof400f_distance_mm;
static uint8_t tof400f_ok;
static uint8_t uart_cam_use_tof;
static uint8_t imu_use_bno08x;
static unsigned long now_ms;
static unsigned long imu_sample_ms;

#ifndef APP_USE_BNO08X
#define APP_USE_BNO08X 0U
#endif

int main(void)
{
    SYSCFG_DL_init();
    SysTick_Init();
    OLED_Init();
    CameraControl_Init();
    TOF400F_Init();

    imu_use_bno08x = APP_USE_BNO08X;
    if (imu_use_bno08x != 0U) {
        BNO08X_Init();
    } else {
        MPU6050_Init();
    }

    uart_cam_use_tof = (DL_GPIO_readPins(GPIO_TRANS_PORT, GPIO_TRANS_PIN_TRANS_CAM_TOF_PIN) != 0U) ? 1U : 0U;
    if (uart_cam_use_tof != 0U) {
        CameraControl_DisableUart();
        TOF400F_EnableUart();
    } else {
        TOF400F_DisableUart();
        CameraControl_EnableUart();
    }

    enable_group1_irq = 1U;
    Interrupt_Init();

    if (imu_use_bno08x != 0U) {
        OLED_ShowString(0,7,(uint8_t *)"BNO08X",8);
    } else {
        OLED_ShowString(0,7,(uint8_t *)"MPU6050",8);
    }
    OLED_ShowString(0,0,(uint8_t *)"Pitch",8);
    OLED_ShowString(0,2,(uint8_t *)" Roll",8);
    OLED_ShowString(0,4,(uint8_t *)"  Yaw",8);
    OLED_ShowString(72,6,(uint8_t *)"C/R",8);
    if (uart_cam_use_tof != 0U) {
        OLED_ShowString(0,6,(uint8_t *)"TOF",8);
    } else {
        OLED_ShowString(0,6,(uint8_t *)"CAM",8);
    }

    tof400f_ok = 0;
    imu_sample_ms = 0U;
    if (uart_cam_use_tof != 0U) {
        TOF400F_Query();
    }

    static unsigned long tof400f_query_ms = 0;

    while (1)
    {
        if (uart_cam_use_tof == 0U) {
            CameraControl_Process();
        }

        mspm0_get_clock_ms(&now_ms);
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

        sprintf((char *)oled_buffer, "%c%c", (imu_use_bno08x != 0U) ? 'B' : 'M', (uart_cam_use_tof != 0U) ? 'T' : 'C');
        OLED_ShowString(96,6,oled_buffer,8);

        mspm0_delay_ms(10);
    }
}
