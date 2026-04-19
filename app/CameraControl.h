#ifndef _CAMERA_CONTROL_H_
#define _CAMERA_CONTROL_H_

#include <stdint.h>

void CameraControl_Init(void);
void CameraControl_EnableUart(void);
void CameraControl_DisableUart(void);
void CameraControl_Start(void);
void CameraControl_Stop(void);
void CameraControl_Process(void);
uint8_t CameraControl_GetPendingCommand(void);
uint8_t CameraControl_GetLastCommand(void);
uint8_t CameraControl_IsStepperRunning(void);
uint8_t CameraControl_GetLastRawByte(void);
int16_t CameraControl_GetLeftMotorDuty(void);

#endif
