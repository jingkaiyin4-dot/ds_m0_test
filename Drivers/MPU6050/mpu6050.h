#ifndef _MPU6050_H_
#define _MPU6050_H_

#include <stdint.h>

typedef struct {
    float pitch;
    float roll;
    float yaw;
    uint8_t status;
} MPU6050_Euler_t;

typedef struct {
    float x_dps;
    float y_dps;
    float z_dps;
} MPU6050_Gyro_t;

void MPU6050_Init(void);
int Read_Quad(void);
void MPU6050_OnInterrupt(void);
uint8_t MPU6050_HasPendingSample(void);
void MPU6050_ClearPendingSample(void);
uint8_t MPU6050_IsReady(void);
uint8_t MPU6050_GetWhoAmI(void);
uint8_t MPU6050_GetAddress(void);

extern short gyro[3], accel[3];
extern float pitch, roll, yaw;
extern MPU6050_Euler_t mpu6050_euler;
extern MPU6050_Gyro_t mpu6050_gyro;

#endif  /* #ifndef _MPU6050_H_ */
