#include "ti_msp_dl_config.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "clock.h"
#include "mpu6050.h"
#include "mspm0_i2c.h"

#define BNO08X_I2C_ADDR                 (0x4BU)
#define BNO08X_I2C_ALT_ADDR             (0x4AU)

#define BNO08X_CHANNEL_COMMAND          (0U)
#define BNO08X_CHANNEL_EXECUTABLE       (1U)
#define BNO08X_CHANNEL_CONTROL          (2U)
#define BNO08X_CHANNEL_REPORTS          (3U)

#define BNO08X_SHTP_REPORT_PRODUCT_ID_RESPONSE (0xF8U)
#define BNO08X_SHTP_REPORT_PRODUCT_ID_REQUEST  (0xF9U)
#define BNO08X_SHTP_REPORT_SET_FEATURE  (0xFDU)
#define BNO08X_SHTP_REPORT_BASE_TS      (0xFBU)

#define BNO08X_SENSOR_ROTATION_VECTOR   (0x05U)
#define BNO08X_SENSOR_GAME_RV           (0x08U)

#define BNO08X_Q14_SCALE                (16384.0f)
#define BNO08X_I2C_TIMEOUT_MS           (20U)
#define BNO08X_STARTUP_DELAY_MS         (50U)
#define BNO08X_PACKET_RETRY_COUNT       (30U)
#define BNO08X_POLL_INTERVAL_MS         (20U)
#define BNO08X_INIT_WAIT_MS             (800U)

typedef struct {
    I2C_Regs *i2c;
    I2C_Regs *tryI2c;
    uint8_t address;
    uint8_t bus;
    uint8_t tryAddress;
    uint8_t tryBus;
    uint8_t seqNum[6];
    uint8_t rxBuf[128];
    uint8_t txBuf[32];
    uint8_t initialized;
    uint8_t hasData;
    uint8_t lastReportId;
    uint8_t lastChannel;
    uint8_t lastPayload0;
    uint16_t lastPacketLen;
} BNO08X_Context_t;

static BNO08X_Context_t g_bno08x;

unsigned long sensor_timestamp;
short gyro[3], accel[3], sensors;
float pitch, roll, yaw;
BNO08X_Euler_t bno08x_euler;
BNO08X_Quat_t bno08x_quat;

static uint16_t BNO08X_Read16(const uint8_t *data)
{
    return (uint16_t) data[0] | ((uint16_t) data[1] << 8);
}

static int16_t BNO08X_ReadS16(const uint8_t *data)
{
    return (int16_t) BNO08X_Read16(data);
}

static void BNO08X_ResetOutputs(void)
{
    memset(&bno08x_euler, 0, sizeof(bno08x_euler));
    memset(&bno08x_quat, 0, sizeof(bno08x_quat));
    pitch = 0.0f;
    roll = 0.0f;
    yaw = 0.0f;
}

static void BNO08X_QuaternionToEuler(float qr, float qi, float qj, float qk)
{
    float sqr = qr * qr;
    float sqi = qi * qi;
    float sqj = qj * qj;
    float sqk = qk * qk;

    yaw = atan2f(2.0f * (qi * qj + qk * qr), (sqi - sqj - sqk + sqr)) * 57.2957795f;
    pitch = asinf(-2.0f * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr)) * 57.2957795f;
    roll = atan2f(2.0f * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr)) * 57.2957795f;

    bno08x_quat.real = qr;
    bno08x_quat.i = qi;
    bno08x_quat.j = qj;
    bno08x_quat.k = qk;

    bno08x_euler.yaw = yaw;
    bno08x_euler.pitch = pitch;
    bno08x_euler.roll = roll;
}

static int BNO08X_I2C_ReadRaw(BNO08X_Context_t *ctx, uint8_t *data, uint16_t length)
{
    uint16_t received = 0;
    unsigned long startMs, nowMs;

    if (length == 0U) {
        return 0;
    }

    mspm0_get_clock_ms(&startMs);

    while (!(DL_I2C_getControllerStatus(ctx->tryI2c) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        mspm0_get_clock_ms(&nowMs);
        if (nowMs >= (startMs + BNO08X_I2C_TIMEOUT_MS)) {
            return -1;
        }
    }

    DL_I2C_clearInterruptStatus(ctx->tryI2c, DL_I2C_INTERRUPT_CONTROLLER_RX_DONE);
    DL_I2C_startControllerTransfer(ctx->tryI2c, ctx->tryAddress, DL_I2C_CONTROLLER_DIRECTION_RX, length);

    while (received < length) {
        if (!DL_I2C_isControllerRXFIFOEmpty(ctx->tryI2c)) {
            data[received++] = DL_I2C_receiveControllerData(ctx->tryI2c);
            continue;
        }

        mspm0_get_clock_ms(&nowMs);
        if (nowMs >= (startMs + BNO08X_I2C_TIMEOUT_MS)) {
            return -1;
        }
    }

    return 0;
}

static int BNO08X_I2C_WriteRaw(BNO08X_Context_t *ctx, const uint8_t *data, uint16_t length)
{
    uint16_t sent = 0;
    unsigned long startMs, nowMs;

    if (length == 0U) {
        return 0;
    }

    mspm0_get_clock_ms(&startMs);

    while (!(DL_I2C_getControllerStatus(ctx->tryI2c) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        mspm0_get_clock_ms(&nowMs);
        if (nowMs >= (startMs + BNO08X_I2C_TIMEOUT_MS)) {
            return -1;
        }
    }

    DL_I2C_clearInterruptStatus(ctx->tryI2c, DL_I2C_INTERRUPT_CONTROLLER_TX_DONE);
    DL_I2C_flushControllerTXFIFO(ctx->tryI2c);
    sent += DL_I2C_fillControllerTXFIFO(ctx->tryI2c, data, length);
    DL_I2C_startControllerTransfer(ctx->tryI2c, ctx->tryAddress, DL_I2C_CONTROLLER_DIRECTION_TX, length);

    while (sent < length) {
        uint16_t fillCnt = DL_I2C_fillControllerTXFIFO(ctx->tryI2c, &data[sent], length - sent);

        if (fillCnt != 0U) {
            sent += fillCnt;
            continue;
        }

        mspm0_get_clock_ms(&nowMs);
        if (nowMs >= (startMs + BNO08X_I2C_TIMEOUT_MS)) {
            return -1;
        }
    }

    while (!DL_I2C_getRawInterruptStatus(ctx->tryI2c, DL_I2C_INTERRUPT_CONTROLLER_TX_DONE)) {
        mspm0_get_clock_ms(&nowMs);
        if (nowMs >= (startMs + BNO08X_I2C_TIMEOUT_MS)) {
            return -1;
        }
    }

    return 0;
}

static int BNO08X_SoftReset(BNO08X_Context_t *ctx)
{
    static const uint8_t resetPacket[] = {0x05, 0x00, 0x01, 0x00, 0x01};

    if (BNO08X_I2C_WriteRaw(ctx, resetPacket, sizeof(resetPacket)) != 0) {
        return -1;
    }
    mspm0_delay_ms(50);
    return 0;
}

static int BNO08X_SendPacket(BNO08X_Context_t *ctx, uint8_t channel, uint8_t payloadLen)
{
    uint8_t packet[32];
    uint16_t totalLen = (uint16_t) payloadLen + 4U;

    packet[0] = (uint8_t) (totalLen & 0xFFU);
    packet[1] = (uint8_t) (totalLen >> 8);
    packet[2] = channel;
    packet[3] = ctx->seqNum[channel]++;
    memcpy(&packet[4], ctx->txBuf, payloadLen);

    return BNO08X_I2C_WriteRaw(ctx, packet, totalLen);
}

static int BNO08X_EnableReport(BNO08X_Context_t *ctx, uint8_t reportId, uint32_t intervalUs)
{
    memset(ctx->txBuf, 0, sizeof(ctx->txBuf));
    ctx->txBuf[0] = BNO08X_SHTP_REPORT_SET_FEATURE;
    ctx->txBuf[1] = reportId;
    ctx->txBuf[2] = 0x00;
    ctx->txBuf[3] = 0x00;
    ctx->txBuf[4] = 0x00;
    ctx->txBuf[5] = (uint8_t) (intervalUs & 0xFFU);
    ctx->txBuf[6] = (uint8_t) ((intervalUs >> 8) & 0xFFU);
    ctx->txBuf[7] = (uint8_t) ((intervalUs >> 16) & 0xFFU);
    ctx->txBuf[8] = (uint8_t) ((intervalUs >> 24) & 0xFFU);

    return BNO08X_SendPacket(ctx, BNO08X_CHANNEL_CONTROL, 17U);
}

static int BNO08X_RequestProductId(BNO08X_Context_t *ctx)
{
    memset(ctx->txBuf, 0, sizeof(ctx->txBuf));
    ctx->txBuf[0] = BNO08X_SHTP_REPORT_PRODUCT_ID_REQUEST;
    return BNO08X_SendPacket(ctx, BNO08X_CHANNEL_CONTROL, 2U);
}

static int BNO08X_ReadPacket(BNO08X_Context_t *ctx, uint16_t *packetLen)
{
    uint8_t header[4];
    uint16_t totalLen;
    uint16_t cargoLen;
    uint16_t offset = 0;

    memset(ctx->rxBuf, 0, sizeof(ctx->rxBuf));

    if (BNO08X_I2C_ReadRaw(ctx, header, sizeof(header)) != 0) {
        return -1;
    }

    totalLen = (uint16_t) header[0] | ((uint16_t) header[1] << 8);
    totalLen &= (uint16_t) ~0x8000U;
    if (totalLen < 4U) {
        return -1;
    }

    memcpy(ctx->rxBuf, header, sizeof(header));
    ctx->lastChannel = header[2];
    ctx->lastPacketLen = totalLen;
    cargoLen = totalLen - 4U;

    while (cargoLen > 0U) {
        uint16_t chunk = (cargoLen > 28U) ? 28U : cargoLen;
        uint8_t temp[32];

        if (BNO08X_I2C_ReadRaw(ctx, temp, (uint16_t) (chunk + 4U)) != 0) {
            return -1;
        }

        memcpy(&ctx->rxBuf[4U + offset], &temp[4], chunk);
        offset += chunk;
        cargoLen -= chunk;
    }

    ctx->lastPayload0 = ctx->rxBuf[4];
    if ((totalLen >= 10U) && (ctx->rxBuf[4] == BNO08X_SHTP_REPORT_BASE_TS)) {
        ctx->lastReportId = ctx->rxBuf[9];
    }

    *packetLen = totalLen;
    return 0;
}

static int BNO08X_WaitForReport(BNO08X_Context_t *ctx, uint16_t *packetLen)
{
    uint8_t retry;

    for (retry = 0; retry < BNO08X_PACKET_RETRY_COUNT; retry++) {
        if (BNO08X_ReadPacket(ctx, packetLen) != 0) {
            continue;
        }

        if (ctx->rxBuf[2] == BNO08X_CHANNEL_REPORTS) {
            return 0;
        }

        if ((ctx->rxBuf[2] == BNO08X_CHANNEL_CONTROL) && (ctx->rxBuf[4] == BNO08X_SHTP_REPORT_PRODUCT_ID_RESPONSE)) {
            continue;
        }
    }

    if (ctx->tryI2c == I2C_OLED_INST) {
        SYSCFG_DL_I2C_OLED_init();
    }

    return -1;
}

static uint8_t BNO08X_DataReady(void)
{
    return (DL_GPIO_readPins(GPIOB, GPIO_MPU6050_PIN_INT_PIN) == 0U) ? 1U : 0U;
}

static int BNO08X_WaitForProductId(BNO08X_Context_t *ctx)
{
    unsigned long startMs;
    unsigned long nowMs;
    uint16_t packetLen;

    mspm0_get_clock_ms(&startMs);

    while (1) {
        mspm0_get_clock_ms(&nowMs);
        if ((nowMs - startMs) >= BNO08X_INIT_WAIT_MS) {
            break;
        }

        if (BNO08X_ReadPacket(ctx, &packetLen) != 0) {
            mspm0_delay_ms(2);
            continue;
        }

        if ((packetLen >= 6U) && (ctx->rxBuf[2] == BNO08X_CHANNEL_CONTROL) && (ctx->rxBuf[4] == BNO08X_SHTP_REPORT_PRODUCT_ID_RESPONSE)) {
            return 0;
        }

        mspm0_delay_ms(2);
    }

    return -1;
}

static int BNO08X_Open(BNO08X_Context_t *ctx, I2C_Regs *i2c, uint8_t bus, uint8_t address)
{
    memset(ctx->seqNum, 0, sizeof(ctx->seqNum));
    ctx->tryI2c = i2c;
    ctx->tryBus = bus;
    ctx->tryAddress = address;

    if (BNO08X_SoftReset(ctx) != 0) {
        return -1;
    }

    mspm0_delay_ms(BNO08X_STARTUP_DELAY_MS);

    /* Drain any startup advertisements before requesting product info. */
    {
        uint8_t flushCount;
        uint16_t packetLen;

        for (flushCount = 0; flushCount < 4U; flushCount++) {
            if (BNO08X_ReadPacket(ctx, &packetLen) != 0) {
                break;
            }
            mspm0_delay_ms(2);
        }
    }

    if (BNO08X_RequestProductId(ctx) != 0) {
        return -1;
    }

    if (BNO08X_WaitForProductId(ctx) != 0) {
        return -1;
    }

    ctx->i2c = i2c;
    ctx->bus = bus;
    ctx->address = address;

    if (ctx->i2c == I2C_OLED_INST) {
        SYSCFG_DL_I2C_OLED_init();
    }

    return 0;
}

static int BNO08X_ParseSensorReport(BNO08X_Context_t *ctx, uint16_t packetLen)
{
    const uint8_t *payload = &ctx->rxBuf[4];
    uint8_t channel = ctx->rxBuf[2];
    uint8_t reportId;
    float qi, qj, qk, qr;

    if ((channel != BNO08X_CHANNEL_REPORTS) || (packetLen < 19U)) {
        return -1;
    }

    if (payload[0] != BNO08X_SHTP_REPORT_BASE_TS) {
        return -1;
    }

    reportId = payload[5];
    if ((reportId != BNO08X_SENSOR_ROTATION_VECTOR) && (reportId != BNO08X_SENSOR_GAME_RV)) {
        return -1;
    }

    qi = (float) BNO08X_ReadS16(&payload[9]) / BNO08X_Q14_SCALE;
    qj = (float) BNO08X_ReadS16(&payload[11]) / BNO08X_Q14_SCALE;
    qk = (float) BNO08X_ReadS16(&payload[13]) / BNO08X_Q14_SCALE;
    qr = (float) BNO08X_ReadS16(&payload[15]) / BNO08X_Q14_SCALE;

    BNO08X_QuaternionToEuler(qr, qi, qj, qk);
    bno08x_euler.status = payload[7] & 0x03U;
    bno08x_quat.status = payload[7] & 0x03U;
    ctx->hasData = 1U;
    ctx->lastReportId = reportId;
    return 0;
}

void MPU6050_Init(void)
{
    memset(&g_bno08x, 0, sizeof(g_bno08x));
    BNO08X_ResetOutputs();

    if (BNO08X_Open(&g_bno08x, I2C_mpu_INST, 0U, BNO08X_I2C_ADDR) != 0) {
        return;
    }

    if (BNO08X_EnableReport(&g_bno08x, BNO08X_SENSOR_GAME_RV, 5000U) != 0) {
        return;
    }
    mspm0_delay_ms(20);
    g_bno08x.initialized = 1U;
}

int Read_Quad(void)
{
    uint16_t packetLen;
    static unsigned long lastPollMs = 0;
    unsigned long nowMs;

    if (g_bno08x.initialized == 0U) {
        return -1;
    }

    mspm0_get_clock_ms(&nowMs);
    if ((BNO08X_DataReady() == 0U) && ((nowMs - lastPollMs) < BNO08X_POLL_INTERVAL_MS)) {
        return 1;
    }
    lastPollMs = nowMs;

    if (BNO08X_WaitForReport(&g_bno08x, &packetLen) != 0) {
        return -1;
    }

    return BNO08X_ParseSensorReport(&g_bno08x, packetLen);
}

uint8_t BNO08X_IsReady(void)
{
    return g_bno08x.initialized;
}

uint8_t BNO08X_HasData(void)
{
    return g_bno08x.hasData;
}

uint8_t BNO08X_GetBus(void)
{
    return g_bno08x.bus;
}

uint8_t BNO08X_GetAddress(void)
{
    return g_bno08x.address;
}

uint8_t BNO08X_GetTryBus(void)
{
    return g_bno08x.tryBus;
}

uint8_t BNO08X_GetTryAddress(void)
{
    return g_bno08x.tryAddress;
}

uint8_t BNO08X_GetIntLevel(void)
{
    return (DL_GPIO_readPins(GPIOB, GPIO_MPU6050_PIN_INT_PIN) == 0U) ? 0U : 1U;
}

uint8_t BNO08X_GetLastChannel(void)
{
    return g_bno08x.lastChannel;
}

uint8_t BNO08X_GetLastPayload0(void)
{
    return g_bno08x.lastPayload0;
}

uint8_t BNO08X_GetLastReportId(void)
{
    return g_bno08x.lastReportId;
}

uint16_t BNO08X_GetLastPacketLen(void)
{
    return g_bno08x.lastPacketLen;
}
