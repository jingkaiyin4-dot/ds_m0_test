#ifndef _MAIN_H_
#define _MAIN_H_

#include "clock.h"
#include "interrupt.h"

#include "oled_software_spi.h"
#include "oled_hardware_spi.h"
#include "ultrasonic_capture.h"
#include "ultrasonic_gpio.h"
#include "bno08x_uart_rvc.h"
#include "Drivers/MPU6050/mpu6050.h"
#include "tof400f.h"
#include "app/CameraControl.h"
#include "app/ZdtStepper.h"
#include "app/AppTypes.h"
#include "app/DriveBase.h"
#include "motor/Motor.h"
#include "PID/PID.h"

void App_StartDrivePidTest(void);
void App_StopDrivePidTest(void);
void App_ResetDriveState(void);
void App_SetImuSource(uint8_t useBno08x);
uint8_t App_GetImuSource(void);
uint8_t App_IsDrivePidActive(void);
uint8_t App_IsTofUartEnabled(void);

#endif  /* #ifndef _MAIN_H_ */
