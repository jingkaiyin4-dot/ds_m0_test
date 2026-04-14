/*
 * BNO08X over I2C minimal driver.
 *
 * This project originally used MPU6050 symbols, so we keep the same file names
 * to avoid changing the generated build lists. The implementation now talks to
 * GY-BNO08X through the existing I2C_MPU6050 instance.
 */

#ifndef _MPU6050_H_
#define _MPU6050_H_

#include <stdint.h>

typedef struct {
    float pitch;
    float roll;
    float yaw;
    uint8_t status;
} BNO08X_Euler_t;

typedef struct {
    float i;
    float j;
    float k;
    float real;
    uint8_t status;
} BNO08X_Quat_t;

void MPU6050_Init(void);
int Read_Quad(void);
uint8_t BNO08X_IsReady(void);
uint8_t BNO08X_HasData(void);
uint8_t BNO08X_GetBus(void);
uint8_t BNO08X_GetAddress(void);
uint8_t BNO08X_GetTryBus(void);
uint8_t BNO08X_GetTryAddress(void);
uint8_t BNO08X_GetIntLevel(void);
uint8_t BNO08X_GetLastChannel(void);
uint8_t BNO08X_GetLastPayload0(void);
uint8_t BNO08X_GetLastReportId(void);
uint16_t BNO08X_GetLastPacketLen(void);

extern short gyro[3], accel[3];
extern float pitch, roll, yaw;
extern BNO08X_Euler_t bno08x_euler;
extern BNO08X_Quat_t bno08x_quat;

#endif  /* #ifndef _MPU6050_H_ */
