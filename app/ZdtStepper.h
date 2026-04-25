#ifndef _ZDT_STEPPER_H_
#define _ZDT_STEPPER_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ZDT_STEPPER_BJ1 = 1,
    ZDT_STEPPER_BJ2 = 2,
} ZdtStepperPort;

typedef struct {
    ZdtStepperPort port;
    uint8_t addr;
} ZdtStepperAxis;

void ZdtStepper_Init(void);
void ZdtStepper_ClearRx(ZdtStepperPort port);
void ZdtStepper_MMCL_EnControl(ZdtStepperPort port, uint8_t addr, bool state, bool syncFlag);
void ZdtStepper_MMCL_VelControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool syncFlag);
void ZdtStepper_MMCL_PosControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, uint32_t clk, bool relative, bool syncFlag);
void ZdtStepper_MultiMotorCmd(ZdtStepperPort port, uint8_t addr);
void ZdtStepper_EnControl(ZdtStepperPort port, uint8_t addr, bool state, bool syncFlag);
void ZdtStepper_VelControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t acc, float vel, bool syncFlag);
void ZdtStepper_VelCurrentControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t acc, float vel, bool syncFlag, uint16_t max_cur_ma);
void ZdtStepper_Stop(ZdtStepperPort port, uint8_t addr, bool syncFlag);
void ZdtStepper_SynchronousMotion(ZdtStepperPort port, uint8_t addr);
void ZdtStepper_BypassPosControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, float vel, float pos_deg, uint8_t relative_mode, bool syncFlag);
void ZdtStepper_TrajPosControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t acc, uint16_t dec, float vel, float pos_deg, uint8_t relative_mode, bool syncFlag);
void ZdtStepper_SmoothStop(ZdtStepperPort port, uint8_t addr, bool syncFlag);
void ZdtStepper_GimbalSetVelocityEx(float yaw_vel, float pitch_vel, uint16_t acc);
void ZdtStepper_GimbalMoveAngle(float yaw_deg, float pitch_deg, float vel);
void ZdtStepper_GimbalMoveAngleSmooth(float yaw_deg, float pitch_deg, float vel, uint16_t acc, uint16_t dec);
void ZdtStepper_GimbalMoveToPosition(float yaw_deg, float pitch_deg, float vel);
void ZdtStepper_GimbalMoveToPositionSmooth(float yaw_deg, float pitch_deg, float vel, uint16_t acc, uint16_t dec);
void ZdtStepper_GimbalStopSmooth(void);
void ZdtStepper_GimbalSetVelocity(float yaw_vel, float pitch_vel);
void ZdtStepper_GimbalMoveDistance(float yaw_deg, float pitch_deg, float vel);
void ZdtStepper_GimbalEmergencyStop(void);
void ZdtStepper_ReadVel(ZdtStepperPort port, uint8_t addr);
bool ZdtStepper_ParseVel(ZdtStepperPort port, float *vel);
void ZdtStepper_ReadPos(ZdtStepperPort port, uint8_t addr);
bool ZdtStepper_ParsePos(ZdtStepperPort port, int32_t *pos);
void ZdtStepper_ReadFlag(ZdtStepperPort port, uint8_t addr);
bool ZdtStepper_ParseFlag(ZdtStepperPort port, uint8_t *flag);
void ZdtStepper_ReadIo(ZdtStepperPort port, uint8_t addr);
bool ZdtStepper_ParseIo(ZdtStepperPort port, uint8_t *io);
uint32_t ZdtStepper_GetTxTimeoutCount(ZdtStepperPort port);
uint8_t ZdtStepper_GetRxCount(ZdtStepperPort port);
uint8_t ZdtStepper_GetRxByte(ZdtStepperPort port, uint8_t index);
uint8_t ZdtStepper_GetLastFrameCount(ZdtStepperPort port);
uint8_t ZdtStepper_GetLastFrameCmd(ZdtStepperPort port);
uint8_t ZdtStepper_GetLastFrameByte(ZdtStepperPort port, uint8_t index);
uint8_t ZdtStepper_GetPendingRxCount(ZdtStepperPort port);

#endif
