#ifndef _CAMERA_CONTROL_H_
#define _CAMERA_CONTROL_H_

#include <stdint.h>

void CameraControl_Init(void);
void CameraControl_Process(void);
uint8_t CameraControl_GetLastCommand(void);
int16_t CameraControl_GetLeftMotorDuty(void);

#endif
