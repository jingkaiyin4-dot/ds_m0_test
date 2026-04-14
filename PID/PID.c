#include "ti_msp_dl_config.h"
#include "Encoder.h"
#include "Motor.h"
#include "PID.h"

#include <math.h>

#define MG513XP26_ENCODER_COUNTS_PER_MOTOR_REV   (11.0f)
#define MG513XP26_GEAR_RATIO                     (26.0f)
#define CONTROL_SAMPLE_PERIOD_S                  (0.01f)

DriveController g_driveController;

static float PIDController_Update(PIDController *pid, float actual)
{
    float error = pid->target - actual;
    float derivative = error - pid->prevError;

    pid->actual = actual;
    pid->integral += error;

    if (pid->integral > pid->integralLimit) {
        pid->integral = pid->integralLimit;
    } else if (pid->integral < -pid->integralLimit) {
        pid->integral = -pid->integralLimit;
    }

    pid->output = pid->kp * error + pid->ki * pid->integral + pid->kd * derivative;

    if (pid->output > pid->outputLimit) {
        pid->output = pid->outputLimit;
    } else if (pid->output < -pid->outputLimit) {
        pid->output = -pid->outputLimit;
    }

    pid->prevError = error;
    return pid->output;
}

static void WheelController_Update(WheelController *wheel,
    EncoderChannel channel,
    MotorChannel motorChannel,
    const DriveController *drive)
{
    int32_t delta = Encoder_GetDelta(channel);
    float motorRps = ((float) delta) / drive->countsPerMotorRev / drive->samplePeriodS;

    wheel->speedRps = motorRps / drive->gearRatio;
    wheel->positionCounts += (float) delta;

    PIDController_Update(&wheel->position, wheel->positionCounts);
    wheel->speed.target = wheel->position.output;
    wheel->duty = PIDController_Update(&wheel->speed, wheel->speedRps);

    Motor_SetDuty(motorChannel, (int16_t) lroundf(wheel->duty));
}

void DriveController_Init(void)
{
    g_driveController.countsPerMotorRev = MG513XP26_ENCODER_COUNTS_PER_MOTOR_REV;
    g_driveController.gearRatio = MG513XP26_GEAR_RATIO;
    g_driveController.samplePeriodS = CONTROL_SAMPLE_PERIOD_S;

    g_driveController.left.speed = (PIDController) {
        .kp = 0.01f,
        .ki = 0.05f,
        .kd = 0.7f,
        .integralLimit = 1000.0f,
        .outputLimit = 50.0f,
    };
    g_driveController.right.speed = g_driveController.left.speed;

    g_driveController.left.position = (PIDController) {
        .kp = 0.01f,
        .ki = 0.0f,
        .kd = 0.7f,
        .target = 351.0f * 6.0f,
        .integralLimit = 1000.0f,
        .outputLimit = 30.0f,
    };
    g_driveController.right.position = g_driveController.left.position;
}

void double_pid(void)
{
    WheelController_Update(&g_driveController.left, ENCODER_LEFT, MOTOR_LEFT, &g_driveController);
    WheelController_Update(&g_driveController.right, ENCODER_RIGHT, MOTOR_RIGHT, &g_driveController);
}
