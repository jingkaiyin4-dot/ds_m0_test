#include "ti_msp_dl_config.h"
#include "Motor.h"

#include <stdbool.h>

static bool g_motorTimerStarted;

static void Motor_SetPwmCompare(MotorChannel channel, uint16_t dutyPercent)
{
    uint32_t compareValue = 7999U - ((7999U * dutyPercent) / 100U);

    if (channel == MOTOR_RIGHT) {
        DL_TimerA_setCaptureCompareValue(Motor_INST, compareValue, GPIO_Motor_C0_IDX);
    } else {
        DL_TimerA_setCaptureCompareValue(Motor_INST, compareValue, GPIO_Motor_C1_IDX);
    }
}

static void Motor_SetDirection(MotorChannel channel, bool forward)
{
    if (channel == MOTOR_RIGHT) {
        if (forward) {
            DL_GPIO_setPins(MOTOR_MOTOR_LA_PORT, MOTOR_MOTOR_LA_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_LB_PORT, MOTOR_MOTOR_LB_PIN);
        } else {
            DL_GPIO_setPins(MOTOR_MOTOR_LB_PORT, MOTOR_MOTOR_LB_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_LA_PORT, MOTOR_MOTOR_LA_PIN);
        }
    } else {
        if (forward) {
            DL_GPIO_setPins(MOTOR_MOTOR_RB_PORT, MOTOR_MOTOR_RB_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_RA_PORT, MOTOR_MOTOR_RA_PIN);
        } else {
            DL_GPIO_setPins(MOTOR_MOTOR_RA_PORT, MOTOR_MOTOR_RA_PIN);
            DL_GPIO_clearPins(MOTOR_MOTOR_RB_PORT, MOTOR_MOTOR_RB_PIN);
        }
    }
}

void Motor_SetDuty(MotorChannel channel, int16_t duty)
{
    bool forward;
    uint16_t magnitude;

    if (!g_motorTimerStarted) {
        DL_Timer_startCounter(Motor_INST);
        g_motorTimerStarted = true;
    }

    if (duty > 100) {
        duty = 100;
    } else if (duty < -100) {
        duty = -100;
    }

    forward = (duty >= 0);
    magnitude = (uint16_t) (forward ? duty : -duty);

    Motor_SetDirection(channel, forward);
    Motor_SetPwmCompare(channel, magnitude);
}

void Motor_StopAll(void)
{
    Motor_SetDuty(MOTOR_LEFT, 0);
    Motor_SetDuty(MOTOR_RIGHT, 0);
}

void SetPWM_and_Motor(int Channel, int Duty)
{
    Motor_SetDuty((Channel == 0) ? MOTOR_LEFT : MOTOR_RIGHT, (int16_t) Duty);
}
