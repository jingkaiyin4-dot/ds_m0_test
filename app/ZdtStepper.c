#include "ZdtStepper.h"

#include "ti_msp_dl_config.h"
#include "clock.h"

#define ZDT_STEPPER_RX_BUF_SIZE   (32U)
#define ZDT_STEPPER_MMCL_BUF_SIZE (64U)

typedef struct {
    uint8_t data[ZDT_STEPPER_RX_BUF_SIZE];
    uint8_t count;
    uint8_t frameReady;
    uint8_t lastCmd;
    uint8_t lastCount;
    uint8_t lastByte0;
    uint8_t lastByte1;
    uint8_t lastByte2;
    uint8_t lastByte3;
} ZdtStepperRxFrame;

static uint8_t ZdtStepper_GetExpectedFrameLength(uint8_t cmd)
{
    switch (cmd) {
        case 0x35U:
            return 6U;
        case 0x36U:
            return 8U;
        case 0x3AU:
            return 4U;
        case 0x3DU:
            return 4U;
        default:
            return 0U;
    }
}

static volatile uint8_t g_bj1RxBuf[ZDT_STEPPER_RX_BUF_SIZE];
static volatile uint8_t g_bj2RxBuf[ZDT_STEPPER_RX_BUF_SIZE];
static volatile uint8_t g_bj1RxCount;
static volatile uint8_t g_bj2RxCount;
static volatile ZdtStepperRxFrame g_bj1Frame;
static volatile ZdtStepperRxFrame g_bj2Frame;

static uint8_t g_bj1MmclBuf[ZDT_STEPPER_MMCL_BUF_SIZE];
static uint8_t g_bj2MmclBuf[ZDT_STEPPER_MMCL_BUF_SIZE];
static uint16_t g_bj1MmclCount;
static uint16_t g_bj2MmclCount;
static uint32_t g_bj1TxTimeoutCount;
static uint32_t g_bj2TxTimeoutCount;

static UART_Regs *ZdtStepper_GetInstance(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? UART_bj1_INST : UART_bj2_INST;
}

static uint8_t *ZdtStepper_GetMmclBuf(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? g_bj1MmclBuf : g_bj2MmclBuf;
}

static uint16_t *ZdtStepper_GetMmclCount(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? &g_bj1MmclCount : &g_bj2MmclCount;
}

static volatile ZdtStepperRxFrame *ZdtStepper_GetFrame(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? &g_bj1Frame : &g_bj2Frame;
}

static void ZdtStepper_RxPushByte(ZdtStepperPort port, uint8_t byte)
{
    volatile uint8_t *rxBuf = (port == ZDT_STEPPER_BJ1) ? g_bj1RxBuf : g_bj2RxBuf;
    volatile uint8_t *rxCount = (port == ZDT_STEPPER_BJ1) ? &g_bj1RxCount : &g_bj2RxCount;
    volatile ZdtStepperRxFrame *frame = ZdtStepper_GetFrame(port);

    if (*rxCount >= ZDT_STEPPER_RX_BUF_SIZE) {
        *rxCount = 0U;
    }

    rxBuf[*rxCount] = byte;
    (*rxCount)++;

    if ((*rxCount >= 2U) && (ZdtStepper_GetExpectedFrameLength(rxBuf[1]) != 0U) &&
        (*rxCount >= ZdtStepper_GetExpectedFrameLength(rxBuf[1]))) {
        uint8_t i;
        uint8_t frameLength = ZdtStepper_GetExpectedFrameLength(rxBuf[1]);
        frame->count = frameLength;
        for (i = 0U; i < frameLength; i++) {
            frame->data[i] = rxBuf[i];
        }
        frame->frameReady = (frame->data[frameLength - 1U] == 0x6BU) ? 1U : 0U;
        frame->lastCount = frameLength;
        frame->lastByte0 = frame->data[0];
        frame->lastByte1 = (frameLength > 1U) ? frame->data[1] : 0U;
        frame->lastByte2 = (frameLength > 2U) ? frame->data[2] : 0U;
        frame->lastByte3 = (frameLength > 3U) ? frame->data[3] : 0U;
        frame->lastCmd = (frameLength > 1U) ? frame->data[1] : 0U;
        *rxCount = 0U;
    } else if ((*rxCount >= 2U) && (ZdtStepper_GetExpectedFrameLength(rxBuf[1]) == 0U)) {
        *rxCount = 0U;
    }
}

static uint8_t ZdtStepper_GetFrameCount(ZdtStepperPort port)
{
    volatile ZdtStepperRxFrame *frame = ZdtStepper_GetFrame(port);
    return (frame->frameReady != 0U) ? frame->count : 0U;
}

static uint8_t ZdtStepper_GetFrameByte(ZdtStepperPort port, uint8_t index)
{
    volatile ZdtStepperRxFrame *frame = ZdtStepper_GetFrame(port);
    return ((frame->frameReady != 0U) && (index < frame->count)) ? frame->data[index] : 0U;
}

static const ZdtStepperAxis g_gimbal_yaw_axis = { ZDT_STEPPER_BJ1, 1U };
static const ZdtStepperAxis g_gimbal_pitch_axis = { ZDT_STEPPER_BJ2, 2U };

static float ZdtStepper_AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static void ZdtStepper_SyncAllAxes(void)
{
    ZdtStepper_SynchronousMotion(g_gimbal_yaw_axis.port, 0U);
    mspm0_delay_ms(2);
    ZdtStepper_SynchronousMotion(g_gimbal_pitch_axis.port, 0U);
}

static void ZdtStepper_GimbalSendVelocityAxis(const ZdtStepperAxis *axis, float vel, uint16_t acc)
{
    uint8_t dir = (vel < 0.0f) ? 1U : 0U;
    ZdtStepper_VelControl(axis->port, axis->addr, dir, acc, ZdtStepper_AbsFloat(vel), false);
}

static void ZdtStepper_GimbalSendBypassAxis(const ZdtStepperAxis *axis, float pos_deg, float vel, uint8_t relative_mode)
{
    uint8_t dir = (pos_deg < 0.0f) ? 1U : 0U;
    ZdtStepper_BypassPosControl(axis->port, axis->addr, dir, vel, ZdtStepper_AbsFloat(pos_deg), relative_mode, false);
}

static void ZdtStepper_GimbalSendTrajAxis(const ZdtStepperAxis *axis, float pos_deg, float vel, uint16_t acc, uint16_t dec, uint8_t relative_mode)
{
    uint8_t dir = (pos_deg < 0.0f) ? 1U : 0U;
    ZdtStepper_TrajPosControl(axis->port, axis->addr, dir, acc, dec, vel, ZdtStepper_AbsFloat(pos_deg), relative_mode, false);
}

static void ZdtStepper_SendBytes(ZdtStepperPort port, const uint8_t *data, uint8_t len)
{
    UART_Regs *instance = ZdtStepper_GetInstance(port);
    uint32_t *timeout_count = (port == ZDT_STEPPER_BJ1) ? &g_bj1TxTimeoutCount : &g_bj2TxTimeoutCount;

    for (uint8_t i = 0; i < len; i++) {
        uint32_t wait_count = 0U;
        while (DL_UART_Main_isTXFIFOFull(instance)) {
            wait_count++;
            if (wait_count > 200000U) {
                (*timeout_count)++;
                return;
            }
        }
        DL_UART_Main_transmitData(instance, data[i]);
    }

    {
        uint32_t wait_count = 0U;
        while (DL_UART_Main_isBusy(instance)) {
            wait_count++;
            if (wait_count > 200000U) {
                (*timeout_count)++;
                return;
            }
        }
    }
}

static void ZdtStepper_MMCL_Append(ZdtStepperPort port, const uint8_t *data, uint8_t len)
{
    uint8_t *buffer = ZdtStepper_GetMmclBuf(port);
    uint16_t *count = ZdtStepper_GetMmclCount(port);

    if ((*count + len) > ZDT_STEPPER_MMCL_BUF_SIZE) {
        *count = 0U;
        return;
    }

    for (uint8_t i = 0; i < len; i++) {
        buffer[*count] = data[i];
        (*count)++;
    }
}

void ZdtStepper_Init(void)
{
    uint8_t dummy[8];

    g_bj1RxCount = 0U;
    g_bj2RxCount = 0U;
    g_bj1MmclCount = 0U;
    g_bj2MmclCount = 0U;
    g_bj1TxTimeoutCount = 0U;
    g_bj2TxTimeoutCount = 0U;
    g_bj1Frame.count = 0U;
    g_bj1Frame.frameReady = 0U;
    g_bj1Frame.lastCmd = 0U;
    g_bj1Frame.lastCount = 0U;
    g_bj1Frame.lastByte0 = 0U;
    g_bj1Frame.lastByte1 = 0U;
    g_bj1Frame.lastByte2 = 0U;
    g_bj1Frame.lastByte3 = 0U;
    g_bj2Frame.count = 0U;
    g_bj2Frame.frameReady = 0U;
    g_bj2Frame.lastCmd = 0U;
    g_bj2Frame.lastCount = 0U;
    g_bj2Frame.lastByte0 = 0U;
    g_bj2Frame.lastByte1 = 0U;
    g_bj2Frame.lastByte2 = 0U;
    g_bj2Frame.lastByte3 = 0U;

    DL_UART_drainRXFIFO(UART_bj1_INST, dummy, 8);
    DL_UART_drainRXFIFO(UART_bj2_INST, dummy, 8);
    NVIC_ClearPendingIRQ(UART_bj1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_bj2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_bj1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_bj2_INST_INT_IRQN);
}

void ZdtStepper_ClearRx(ZdtStepperPort port)
{
    volatile ZdtStepperRxFrame *frame = ZdtStepper_GetFrame(port);

    __disable_irq();
    if (port == ZDT_STEPPER_BJ1) {
        g_bj1RxCount = 0U;
    } else {
        g_bj2RxCount = 0U;
    }
    frame->count = 0U;
    frame->frameReady = 0U;
    __enable_irq();
}

void ZdtStepper_MMCL_EnControl(ZdtStepperPort port, uint8_t addr, bool state, bool syncFlag)
{
    uint8_t cmd[6];

    cmd[0] = addr;
    cmd[1] = 0xF3;
    cmd[2] = 0xAB;
    cmd[3] = (uint8_t) state;
    cmd[4] = (uint8_t) syncFlag;
    cmd[5] = 0x6B;

    ZdtStepper_MMCL_Append(port, cmd, 6);
}

void ZdtStepper_MMCL_VelControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, bool syncFlag)
{
    uint8_t cmd[8];

    cmd[0] = addr;
    cmd[1] = 0xF6;
    cmd[2] = dir;
    cmd[3] = (uint8_t) (vel >> 8);
    cmd[4] = (uint8_t) (vel & 0xFF);
    cmd[5] = acc;
    cmd[6] = (uint8_t) syncFlag;
    cmd[7] = 0x6B;

    ZdtStepper_MMCL_Append(port, cmd, 8);
}

void ZdtStepper_VelControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t acc, float vel, bool syncFlag)
{
    uint8_t cmd[9];
    uint16_t vel_x10 = 0U;

    if (vel < 0.0f) {
        vel = 0.0f;
    }
    vel_x10 = (uint16_t) (vel * 10.0f);

    cmd[0] = addr;
    cmd[1] = 0xF6;
    cmd[2] = dir;
    cmd[3] = (uint8_t) (acc >> 8);
    cmd[4] = (uint8_t) (acc & 0xFF);
    cmd[5] = (uint8_t) (vel_x10 >> 8);
    cmd[6] = (uint8_t) (vel_x10 & 0xFF);
    cmd[7] = (uint8_t) syncFlag;
    cmd[8] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 9);
}

void ZdtStepper_VelCurrentControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t acc, float vel, bool syncFlag, uint16_t max_cur_ma)
{
    uint8_t cmd[11];
    uint16_t vel_x10 = 0U;

    if (vel < 0.0f) {
        vel = 0.0f;
    }
    vel_x10 = (uint16_t) (vel * 10.0f);

    cmd[0] = addr;
    cmd[1] = 0xC6;
    cmd[2] = dir;
    cmd[3] = (uint8_t) (acc >> 8);
    cmd[4] = (uint8_t) (acc & 0xFF);
    cmd[5] = (uint8_t) (vel_x10 >> 8);
    cmd[6] = (uint8_t) (vel_x10 & 0xFF);
    cmd[7] = (uint8_t) syncFlag;
    cmd[8] = (uint8_t) (max_cur_ma >> 8);
    cmd[9] = (uint8_t) (max_cur_ma & 0xFF);
    cmd[10] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 11);
}

void ZdtStepper_Stop(ZdtStepperPort port, uint8_t addr, bool syncFlag)
{
    uint8_t cmd[5];

    cmd[0] = addr;
    cmd[1] = 0xFE;
    cmd[2] = 0x98;
    cmd[3] = (uint8_t) syncFlag;
    cmd[4] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 5);
}

void ZdtStepper_SynchronousMotion(ZdtStepperPort port, uint8_t addr)
{
    uint8_t cmd[4];

    cmd[0] = addr;
    cmd[1] = 0xFF;
    cmd[2] = 0x66;
    cmd[3] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 4);
}

void ZdtStepper_BypassPosControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, float vel, float pos_deg, uint8_t relative_mode, bool syncFlag)
{
    uint8_t cmd[12];
    uint16_t vel_x10 = 0U;
    uint32_t pos_x10 = 0U;

    if (vel < 0.0f) {
        vel = 0.0f;
    }
    if (pos_deg < 0.0f) {
        pos_deg = -pos_deg;
    }

    vel_x10 = (uint16_t) (vel * 10.0f);
    pos_x10 = (uint32_t) (pos_deg * 10.0f);

    cmd[0] = addr;
    cmd[1] = 0xFB;
    cmd[2] = dir;
    cmd[3] = (uint8_t) (vel_x10 >> 8);
    cmd[4] = (uint8_t) (vel_x10 & 0xFF);
    cmd[5] = (uint8_t) (pos_x10 >> 24);
    cmd[6] = (uint8_t) (pos_x10 >> 16);
    cmd[7] = (uint8_t) (pos_x10 >> 8);
    cmd[8] = (uint8_t) (pos_x10 & 0xFF);
    cmd[9] = relative_mode;
    cmd[10] = (uint8_t) syncFlag;
    cmd[11] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 12);
}
void ZdtStepper_TrajPosControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t acc, uint16_t dec, float vel, float pos_deg, uint8_t relative_mode, bool syncFlag)
{
    uint8_t cmd[16];
    uint16_t vel_x10 = 0U;
    uint32_t pos_x10 = 0U;

    if (vel < 0.0f) {
        vel = 0.0f;
    }
    if (pos_deg < 0.0f) {
        pos_deg = -pos_deg;
    }

    vel_x10 = (uint16_t) (vel * 10.0f);
    pos_x10 = (uint32_t) (pos_deg * 10.0f);

    cmd[0] = addr;
    cmd[1] = 0xFD;
    cmd[2] = dir;
    cmd[3] = (uint8_t) (acc >> 8);
    cmd[4] = (uint8_t) (acc & 0xFF);
    cmd[5] = (uint8_t) (dec >> 8);
    cmd[6] = (uint8_t) (dec & 0xFF);
    cmd[7] = (uint8_t) (vel_x10 >> 8);
    cmd[8] = (uint8_t) (vel_x10 & 0xFF);
    cmd[9] = (uint8_t) (pos_x10 >> 24);
    cmd[10] = (uint8_t) (pos_x10 >> 16);
    cmd[11] = (uint8_t) (pos_x10 >> 8);
    cmd[12] = (uint8_t) (pos_x10 & 0xFF);
    cmd[13] = relative_mode;
    cmd[14] = (uint8_t) syncFlag;
    cmd[15] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 16);
}

void ZdtStepper_SmoothStop(ZdtStepperPort port, uint8_t addr, bool syncFlag)
{
    ZdtStepper_VelControl(port, addr, 0U, 200U, 0.0f, syncFlag);
}


void ZdtStepper_GimbalSetVelocityEx(float yaw_vel, float pitch_vel, uint16_t acc)
{
    ZdtStepper_GimbalSendVelocityAxis(&g_gimbal_yaw_axis, yaw_vel, acc);
    mspm0_delay_ms(10);
    ZdtStepper_GimbalSendVelocityAxis(&g_gimbal_pitch_axis, pitch_vel, acc);
    mspm0_delay_ms(10);
    ZdtStepper_SyncAllAxes();
}

void ZdtStepper_GimbalSetVelocity(float yaw_vel, float pitch_vel)
{
    ZdtStepper_GimbalSetVelocityEx(yaw_vel, pitch_vel, 25U);
}

void ZdtStepper_GimbalMoveAngle(float yaw_deg, float pitch_deg, float vel)
{
    ZdtStepper_GimbalSendBypassAxis(&g_gimbal_yaw_axis, yaw_deg, vel, 2U);
    mspm0_delay_ms(10);
    ZdtStepper_GimbalSendBypassAxis(&g_gimbal_pitch_axis, pitch_deg, vel, 2U);
    mspm0_delay_ms(10);
    ZdtStepper_SyncAllAxes();
}

void ZdtStepper_GimbalMoveDistance(float yaw_deg, float pitch_deg, float vel)
{
    ZdtStepper_GimbalMoveAngle(yaw_deg, pitch_deg, vel);
}

void ZdtStepper_GimbalMoveToPosition(float yaw_deg, float pitch_deg, float vel)
{
    ZdtStepper_GimbalSendBypassAxis(&g_gimbal_yaw_axis, yaw_deg, vel, 1U);
    mspm0_delay_ms(10);
    ZdtStepper_GimbalSendBypassAxis(&g_gimbal_pitch_axis, pitch_deg, vel, 1U);
    mspm0_delay_ms(10);
    ZdtStepper_SyncAllAxes();
}

void ZdtStepper_GimbalMoveAngleSmooth(float yaw_deg, float pitch_deg, float vel, uint16_t acc, uint16_t dec)
{
    ZdtStepper_GimbalSendTrajAxis(&g_gimbal_yaw_axis, yaw_deg, vel, acc, dec, 2U);
    mspm0_delay_ms(10);
    ZdtStepper_GimbalSendTrajAxis(&g_gimbal_pitch_axis, pitch_deg, vel, acc, dec, 2U);
    mspm0_delay_ms(10);
    ZdtStepper_SyncAllAxes();
}

void ZdtStepper_GimbalMoveToPositionSmooth(float yaw_deg, float pitch_deg, float vel, uint16_t acc, uint16_t dec)
{
    ZdtStepper_GimbalSendTrajAxis(&g_gimbal_yaw_axis, yaw_deg, vel, acc, dec, 1U);
    mspm0_delay_ms(10);
    ZdtStepper_GimbalSendTrajAxis(&g_gimbal_pitch_axis, pitch_deg, vel, acc, dec, 1U);
    mspm0_delay_ms(10);
    ZdtStepper_SyncAllAxes();
}

void ZdtStepper_GimbalStopSmooth(void)
{
    ZdtStepper_SmoothStop(g_gimbal_yaw_axis.port, g_gimbal_yaw_axis.addr, false);
    mspm0_delay_ms(10);
    ZdtStepper_SmoothStop(g_gimbal_pitch_axis.port, g_gimbal_pitch_axis.addr, false);
    mspm0_delay_ms(10);
    ZdtStepper_SyncAllAxes();
}

void ZdtStepper_GimbalEmergencyStop(void)
{
    ZdtStepper_Stop(g_gimbal_yaw_axis.port, g_gimbal_yaw_axis.addr, false);
    mspm0_delay_ms(10);
    ZdtStepper_Stop(g_gimbal_pitch_axis.port, g_gimbal_pitch_axis.addr, false);
}

void ZdtStepper_ReadVel(ZdtStepperPort port, uint8_t addr)
{
    uint8_t cmd[3];

    cmd[0] = addr;
    cmd[1] = 0x35;
    cmd[2] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 3);
}

bool ZdtStepper_ParseVel(ZdtStepperPort port, float *vel)
{
    uint8_t count;
    uint8_t sign;
    uint16_t raw;

    if (vel == 0) {
        return false;
    }

    count = ZdtStepper_GetFrameCount(port);
    if (count < 6U) {
        return false;
    }

    if (ZdtStepper_GetFrameByte(port, 1U) != 0x35U) {
        return false;
    }

    sign = ZdtStepper_GetFrameByte(port, 2U);
    raw = ((uint16_t) ZdtStepper_GetFrameByte(port, 3U) << 8) | ZdtStepper_GetFrameByte(port, 4U);
    *vel = (float) raw * 0.1f;
    if (sign != 0U) {
        *vel = -*vel;
    }

    return true;
}

void ZdtStepper_ReadPos(ZdtStepperPort port, uint8_t addr)
{
    uint8_t cmd[3];

    cmd[0] = addr;
    cmd[1] = 0x36;
    cmd[2] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 3);
}

bool ZdtStepper_ParsePos(ZdtStepperPort port, int32_t *pos)
{
    uint8_t count;
    uint8_t sign;
    uint32_t raw;

    if (pos == 0) {
        return false;
    }

    count = ZdtStepper_GetFrameCount(port);
    if (count < 8U) {
        return false;
    }

    if (ZdtStepper_GetFrameByte(port, 1U) != 0x36U) {
        return false;
    }

    sign = ZdtStepper_GetFrameByte(port, 2U);
    raw = ((uint32_t) ZdtStepper_GetFrameByte(port, 3U) << 24) |
          ((uint32_t) ZdtStepper_GetFrameByte(port, 4U) << 16) |
          ((uint32_t) ZdtStepper_GetFrameByte(port, 5U) << 8) |
          (uint32_t) ZdtStepper_GetFrameByte(port, 6U);
    *pos = (int32_t) raw;
    if (sign != 0U) {
        *pos = -*pos;
    }

    return true;
}

void ZdtStepper_ReadFlag(ZdtStepperPort port, uint8_t addr)
{
    uint8_t cmd[3];

    cmd[0] = addr;
    cmd[1] = 0x3A;
    cmd[2] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 3);
}

bool ZdtStepper_ParseFlag(ZdtStepperPort port, uint8_t *flag)
{
    uint8_t count;

    if (flag == 0) {
        return false;
    }

    count = ZdtStepper_GetFrameCount(port);
    if (count < 4U) {
        return false;
    }

    if (ZdtStepper_GetFrameByte(port, 1U) != 0x3AU) {
        return false;
    }

    *flag = ZdtStepper_GetFrameByte(port, 2U);
    return true;
}

void ZdtStepper_ReadIo(ZdtStepperPort port, uint8_t addr)
{
    uint8_t cmd[3];

    cmd[0] = addr;
    cmd[1] = 0x3D;
    cmd[2] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 3);
}

bool ZdtStepper_ParseIo(ZdtStepperPort port, uint8_t *io)
{
    uint8_t count;

    if (io == 0) {
        return false;
    }

    count = ZdtStepper_GetFrameCount(port);
    if (count < 4U) {
        return false;
    }

    if (ZdtStepper_GetFrameByte(port, 1U) != 0x3DU) {
        return false;
    }

    *io = ZdtStepper_GetFrameByte(port, 2U);
    return true;
}

void ZdtStepper_EnControl(ZdtStepperPort port, uint8_t addr, bool state, bool syncFlag)
{
    uint8_t cmd[6];

    cmd[0] = addr;
    cmd[1] = 0xF3;
    cmd[2] = 0xAB;
    cmd[3] = (uint8_t) state;
    cmd[4] = (uint8_t) syncFlag;
    cmd[5] = 0x6B;

    ZdtStepper_SendBytes(port, cmd, 6);
}

void ZdtStepper_MMCL_PosControl(ZdtStepperPort port, uint8_t addr, uint8_t dir, uint16_t vel, uint8_t acc, uint32_t clk, bool relative, bool syncFlag)
{
    uint8_t cmd[13];

    cmd[0] = addr;
    cmd[1] = 0xFD;
    cmd[2] = dir;
    cmd[3] = (uint8_t) (vel >> 8);
    cmd[4] = (uint8_t) (vel & 0xFF);
    cmd[5] = acc;
    cmd[6] = (uint8_t) (clk >> 24);
    cmd[7] = (uint8_t) (clk >> 16);
    cmd[8] = (uint8_t) (clk >> 8);
    cmd[9] = (uint8_t) clk;
    cmd[10] = (uint8_t) relative;
    cmd[11] = (uint8_t) syncFlag;
    cmd[12] = 0x6B;

    ZdtStepper_MMCL_Append(port, cmd, 13);
}

void ZdtStepper_MultiMotorCmd(ZdtStepperPort port, uint8_t addr)
{
    uint8_t *buffer = ZdtStepper_GetMmclBuf(port);
    uint16_t *count = ZdtStepper_GetMmclCount(port);
    uint8_t frame[ZDT_STEPPER_MMCL_BUF_SIZE + 5U];
    uint16_t length;

    if (*count == 0U) {
        return;
    }

    length = (uint16_t) (*count + 5U);
    frame[0] = addr;
    frame[1] = 0xAA;
    frame[2] = (uint8_t) (length >> 8);
    frame[3] = (uint8_t) length;

    for (uint16_t i = 0; i < *count; i++) {
        frame[4 + i] = buffer[i];
    }

    frame[4 + *count] = 0x6B;
    ZdtStepper_SendBytes(port, frame, (uint8_t) (*count + 5U));
    *count = 0U;
}

uint32_t ZdtStepper_GetTxTimeoutCount(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? g_bj1TxTimeoutCount : g_bj2TxTimeoutCount;
}

uint8_t ZdtStepper_GetRxCount(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? g_bj1RxCount : g_bj2RxCount;
}

uint8_t ZdtStepper_GetRxByte(ZdtStepperPort port, uint8_t index)
{
    if (port == ZDT_STEPPER_BJ1) {
        return (index < g_bj1RxCount) ? g_bj1RxBuf[index] : 0U;
    }

    return (index < g_bj2RxCount) ? g_bj2RxBuf[index] : 0U;
}

void UART_bj1_INST_IRQHandler(void)
{
    while (DL_UART_Main_isRXFIFOEmpty(UART_bj1_INST) == false) {
        ZdtStepper_RxPushByte(ZDT_STEPPER_BJ1, (uint8_t) DL_UART_Main_receiveData(UART_bj1_INST));
    }
}

uint8_t ZdtStepper_GetLastFrameCount(ZdtStepperPort port)
{
    return ZdtStepper_GetFrameCount(port);
}

uint8_t ZdtStepper_GetLastFrameCmd(ZdtStepperPort port)
{
    volatile ZdtStepperRxFrame *frame = ZdtStepper_GetFrame(port);
    return frame->lastCmd;
}

uint8_t ZdtStepper_GetLastFrameByte(ZdtStepperPort port, uint8_t index)
{
    volatile ZdtStepperRxFrame *frame = ZdtStepper_GetFrame(port);

    switch (index) {
        case 0U:
            return frame->lastByte0;
        case 1U:
            return frame->lastByte1;
        case 2U:
            return frame->lastByte2;
        case 3U:
            return frame->lastByte3;
        default:
            return 0U;
    }
}

uint8_t ZdtStepper_GetPendingRxCount(ZdtStepperPort port)
{
    return (port == ZDT_STEPPER_BJ1) ? g_bj1RxCount : g_bj2RxCount;
}

void UART_bj2_INST_IRQHandler(void)
{
    while (DL_UART_Main_isRXFIFOEmpty(UART_bj2_INST) == false) {
        ZdtStepper_RxPushByte(ZDT_STEPPER_BJ2, (uint8_t) DL_UART_Main_receiveData(UART_bj2_INST));
    }
}
