/**
 * @file AppTypes.h
 * @brief 应用层公共类型定义
 */

#ifndef _APP_TYPES_H_
#define _APP_TYPES_H_

#include <stdbool.h>
#include <stdint.h>

/** 总线类型 */
typedef enum {
    APP_BUS_NONE = 0,
    APP_BUS_UART,
    APP_BUS_I2C,
} AppBusType;

/** 总线设备描述 */
typedef struct {
    AppBusType type;
    const char *name;
    uint8_t address;
    uint32_t speed_hz;
} AppBusDevice;

/** 按键状态 */
typedef struct {
    uint8_t pressed;        /** 当前按下状态 */
    uint8_t clicked;        /** 单次点击标志 */
    uint32_t stable_since_ms; /** 稳定时间戳 */
} AppButtonState;

/**
 * @brief 电机驱动状态
 * 由Motor_FillDriveState填充，供PID读取
 */
typedef struct {
    int16_t duty;              /** 当前输出的duty值 */
    int32_t encoder_delta;     /** 本周期编码器增量(用于计算速度) */
    int32_t encoder_total;     /** 编码器累计值(用于计算位置) */
} DriveMotorState;

#endif