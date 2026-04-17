#include "ZdtStepper.h"

#include "ti_msp_dl_config.h"
#include "clock.h"

#define ZDT_STEPPER_RX_BUF_SIZE   (32U)
#define ZDT_STEPPER_MMCL_BUF_SIZE (64U)

static volatile uint8_t g_bj1RxBuf[ZDT_STEPPER_RX_BUF_SIZE];
static volatile uint8_t g_bj2RxBuf[ZDT_STEPPER_RX_BUF_SIZE];
static volatile uint8_t g_bj1RxCount;
static volatile uint8_t g_bj2RxCount;

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

    DL_UART_drainRXFIFO(UART_bj1_INST, dummy, 8);
    DL_UART_drainRXFIFO(UART_bj2_INST, dummy, 8);
    NVIC_ClearPendingIRQ(UART_bj1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_bj2_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_bj1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_bj2_INST_INT_IRQN);
}

void ZdtStepper_ClearRx(ZdtStepperPort port)
{
    __disable_irq();
    if (port == ZDT_STEPPER_BJ1) {
        g_bj1RxCount = 0U;
    } else {
        g_bj2RxCount = 0U;
    }
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

    count = ZdtStepper_GetRxCount(port);
    if (count != 6U) {
        return false;
    }

    if (ZdtStepper_GetRxByte(port, 1U) != 0x35U) {
        return false;
    }

    sign = ZdtStepper_GetRxByte(port, 2U);
    raw = ((uint16_t) ZdtStepper_GetRxByte(port, 3U) << 8) | ZdtStepper_GetRxByte(port, 4U);
    *vel = (float) raw * 0.1f;
    if (sign != 0U) {
        *vel = -*vel;
    }

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
    while ((DL_UART_Main_isRXFIFOEmpty(UART_bj1_INST) == false) && (g_bj1RxCount < ZDT_STEPPER_RX_BUF_SIZE)) {
        g_bj1RxBuf[g_bj1RxCount++] = (uint8_t) DL_UART_Main_receiveData(UART_bj1_INST);
    }

    while (DL_UART_Main_isRXFIFOEmpty(UART_bj1_INST) == false) {
        (void) DL_UART_Main_receiveData(UART_bj1_INST);
    }
}

void UART_bj2_INST_IRQHandler(void)
{
    while ((DL_UART_Main_isRXFIFOEmpty(UART_bj2_INST) == false) && (g_bj2RxCount < ZDT_STEPPER_RX_BUF_SIZE)) {
        g_bj2RxBuf[g_bj2RxCount++] = (uint8_t) DL_UART_Main_receiveData(UART_bj2_INST);
    }

    while (DL_UART_Main_isRXFIFOEmpty(UART_bj2_INST) == false) {
        (void) DL_UART_Main_receiveData(UART_bj2_INST);
    }
}
