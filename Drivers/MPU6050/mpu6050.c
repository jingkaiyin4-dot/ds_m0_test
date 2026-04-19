#include "ti_msp_dl_config.h"

#include <math.h>
#include <stdint.h>

#include "clock.h"
#include "mpu6050.h"
#include "mspm0_i2c.h"

#define MPU6050_ADDR_LOW        (0x68U)
#define MPU6050_ADDR_HIGH       (0x69U)
#define MPU6050_REG_SMPLRT_DIV  (0x19U)
#define MPU6050_REG_CONFIG      (0x1AU)
#define MPU6050_REG_GYRO_CONFIG (0x1BU)
#define MPU6050_REG_ACCEL_CONFIG (0x1CU)
#define MPU6050_REG_INT_PIN_CFG  (0x37U)
#define MPU6050_REG_INT_ENABLE  (0x38U)
#define MPU6050_REG_ACCEL_XOUT_H (0x3BU)
#define MPU6050_REG_PWR_MGMT_1  (0x6BU)
#define MPU6050_REG_WHO_AM_I    (0x75U)

#define MPU6050_ACCEL_SCALE_2G  (16384.0f)
#define MPU6050_GYRO_SCALE_250DPS (131.0f)
#define MPU6050_FILTER_ALPHA    (0.96f)
#define MPU6050_YAW_DEADBAND_DPS (1.2f)
#define MPU6050_YAW_RETURN_RATE  (0.15f)
#define MPU6050_YAW_BIAS_TRACK_ALPHA (0.02f)

static uint8_t g_mpu6050_ready;
static uint8_t g_mpu6050_last_whoami;
static uint8_t g_mpu6050_addr;
static volatile uint8_t g_mpu6050_sample_pending;
static float g_mpu6050_yaw_bias_dps;

short gyro[3], accel[3], sensors;
float pitch, roll, yaw;
MPU6050_Euler_t mpu6050_euler;
MPU6050_Gyro_t mpu6050_gyro;

static int MPU6050_WriteReg(uint8_t reg, uint8_t value)
{
    return mspm0_i2c_write(g_mpu6050_addr, reg, 1U, &value);
}

static int MPU6050_ReadRegs(uint8_t reg, uint8_t *data, uint8_t length)
{
    return mspm0_i2c_read(g_mpu6050_addr, reg, length, data);
}

static int16_t MPU6050_ReadS16(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

static float MPU6050_ApplyYawSuppression(float gz, float dt)
{
    float yawRate = gz - g_mpu6050_yaw_bias_dps;

    if ((fabsf(mpu6050_gyro.x_dps) < 3.0f) &&
        (fabsf(mpu6050_gyro.y_dps) < 3.0f) &&
        (fabsf(gz) < 3.0f) &&
        (fabsf(accel[0]) < 3000) &&
        (fabsf(accel[1]) < 3000)) {
        g_mpu6050_yaw_bias_dps += (gz - g_mpu6050_yaw_bias_dps) * MPU6050_YAW_BIAS_TRACK_ALPHA;
        yawRate = gz - g_mpu6050_yaw_bias_dps;
    }

    if (fabsf(yawRate) < MPU6050_YAW_DEADBAND_DPS) {
        yawRate = 0.0f;

        if (fabsf(yaw) < MPU6050_YAW_RETURN_RATE) {
            yaw = 0.0f;
        } else if (yaw > 0.0f) {
            yaw -= MPU6050_YAW_RETURN_RATE * dt;
        } else {
            yaw += MPU6050_YAW_RETURN_RATE * dt;
        }
    }

    return yawRate;
}

static void MPU6050_ResetOutputs(void)
{
    uint8_t i;

    for (i = 0; i < 3U; i++) {
        accel[i] = 0;
        gyro[i] = 0;
    }

    sensors = 0;
    pitch = 0.0f;
    roll = 0.0f;
    yaw = 0.0f;
    g_mpu6050_yaw_bias_dps = 0.0f;

    mpu6050_euler.pitch = 0.0f;
    mpu6050_euler.roll = 0.0f;
    mpu6050_euler.yaw = 0.0f;
    mpu6050_euler.status = 0U;

    mpu6050_gyro.x_dps = 0.0f;
    mpu6050_gyro.y_dps = 0.0f;
    mpu6050_gyro.z_dps = 0.0f;
}

void MPU6050_Init(void)
{
    uint8_t whoami = 0U;

    g_mpu6050_ready = 0U;
    g_mpu6050_last_whoami = 0U;
    g_mpu6050_addr = MPU6050_ADDR_LOW;
    g_mpu6050_sample_pending = 0U;
    MPU6050_ResetOutputs();

    if (MPU6050_ReadRegs(MPU6050_REG_WHO_AM_I, &whoami, 1U) != 0) {
        g_mpu6050_addr = MPU6050_ADDR_HIGH;
        if (MPU6050_ReadRegs(MPU6050_REG_WHO_AM_I, &whoami, 1U) != 0) {
            return;
        }
    }
    g_mpu6050_last_whoami = whoami;

    if ((whoami != MPU6050_ADDR_LOW) && (whoami != MPU6050_ADDR_HIGH)) {
        return;
    }

    if (MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x00U) != 0) {
        return;
    }
    mspm0_delay_ms(50);

    if (MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x04U) != 0) {
        return;
    }
    if (MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03U) != 0) {
        return;
    }
    if (MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00U) != 0) {
        return;
    }
    if (MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00U) != 0) {
        return;
    }
    if (MPU6050_WriteReg(MPU6050_REG_INT_PIN_CFG, 0x10U) != 0) {
        return;
    }
    if (MPU6050_WriteReg(MPU6050_REG_INT_ENABLE, 0x01U) != 0) {
        return;
    }

    g_mpu6050_ready = 1U;
    g_mpu6050_sample_pending = 1U;
    mpu6050_euler.status = 1U;
}

int Read_Quad(void)
{
    uint8_t raw[14];
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    static unsigned long lastMs = 0U;
    unsigned long nowMs;
    float dt;
    float accelPitch;
    float accelRoll;

    if (g_mpu6050_ready == 0U) {
        return -1;
    }

    if (MPU6050_ReadRegs(MPU6050_REG_ACCEL_XOUT_H, raw, sizeof(raw)) != 0) {
        return -1;
    }

    accel[0] = MPU6050_ReadS16(&raw[0]);
    accel[1] = MPU6050_ReadS16(&raw[2]);
    accel[2] = MPU6050_ReadS16(&raw[4]);
    gyro[0] = MPU6050_ReadS16(&raw[8]);
    gyro[1] = MPU6050_ReadS16(&raw[10]);
    gyro[2] = MPU6050_ReadS16(&raw[12]);

    ax = (float)accel[0] / MPU6050_ACCEL_SCALE_2G;
    ay = (float)accel[1] / MPU6050_ACCEL_SCALE_2G;
    az = (float)accel[2] / MPU6050_ACCEL_SCALE_2G;
    gx = (float)gyro[0] / MPU6050_GYRO_SCALE_250DPS;
    gy = (float)gyro[1] / MPU6050_GYRO_SCALE_250DPS;
    gz = (float)gyro[2] / MPU6050_GYRO_SCALE_250DPS;

    mpu6050_gyro.x_dps = gx;
    mpu6050_gyro.y_dps = gy;
    mpu6050_gyro.z_dps = gz;

    accelPitch = atan2f(ay, sqrtf((ax * ax) + (az * az))) * 57.2957795f;
    accelRoll = atan2f(-ax, az) * 57.2957795f;

    mspm0_get_clock_ms(&nowMs);
    if (lastMs == 0U) {
        dt = 0.02f;
    } else {
        dt = (float)(nowMs - lastMs) / 1000.0f;
        if (dt <= 0.0f) {
            dt = 0.02f;
        }
    }
    lastMs = nowMs;

    pitch = (MPU6050_FILTER_ALPHA * (pitch + (gx * dt))) + ((1.0f - MPU6050_FILTER_ALPHA) * accelPitch);
    roll = (MPU6050_FILTER_ALPHA * (roll + (gy * dt))) + ((1.0f - MPU6050_FILTER_ALPHA) * accelRoll);
    gz = MPU6050_ApplyYawSuppression(gz, dt);
    yaw += gz * dt;

    mpu6050_euler.pitch = pitch;
    mpu6050_euler.roll = roll;
    mpu6050_euler.yaw = yaw;
    mpu6050_euler.status = 1U;
    sensors = 1;

    return 0;
}

void MPU6050_OnInterrupt(void)
{
    g_mpu6050_sample_pending = 1U;
}

uint8_t MPU6050_HasPendingSample(void)
{
    return g_mpu6050_sample_pending;
}

void MPU6050_ClearPendingSample(void)
{
    g_mpu6050_sample_pending = 0U;
}

uint8_t MPU6050_IsReady(void)
{
    return g_mpu6050_ready;
}

uint8_t MPU6050_GetWhoAmI(void)
{
    return g_mpu6050_last_whoami;
}

uint8_t MPU6050_GetAddress(void)
{
    return g_mpu6050_addr;
}
