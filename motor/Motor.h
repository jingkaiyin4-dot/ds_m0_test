#ifndef _MOTOR_H_
#define _MOTOR_H_

#include <stdint.h>

typedef enum {
    MOTOR_LEFT = 0,
    MOTOR_RIGHT = 1,
} MotorChannel;

void Motor_SetDuty(MotorChannel channel, int16_t duty);
void Motor_StopAll(void);

/* Compatibility wrapper for existing call sites. */
void SetPWM_and_Motor(int Channel, int Duty);

#endif
