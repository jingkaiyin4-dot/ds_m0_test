#ifndef _PID_H_
#define _PID_H_

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float target;
    float actual;
    float integral;
    float prevError;
    float output;
    float integralLimit;
    float outputLimit;
} PIDController;

typedef struct {
    PIDController speed;
    PIDController position;
    float speedRps;
    float positionCounts;
    float duty;
} WheelController;

typedef struct {
    WheelController left;
    WheelController right;
    float countsPerMotorRev;
    float gearRatio;
    float samplePeriodS;
} DriveController;

extern DriveController g_driveController;

void DriveController_Init(void);
void double_pid(void);

#endif
