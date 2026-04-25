#ifndef _GRAYSCALE_H_
#define _GRAYSCALE_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 读取8路灰度传感器并合并为8位二进制数
 * 
 * 最左侧传感器(PB12)对应最高位(Bit 7)
 * 最右侧传感器(PB24)对应最低位(Bit 0)
 * 返回值的二进制形式直接对应传感器从左到右的状态
 * (假设高电平为1，低电平为0)
 * 
 * @return uint8_t 8位合并状态
 */
uint8_t Grayscale_ReadBinary(void);

/**
 * @brief 根据灰度状态设置双环速度目标进行循迹
 * 
 * @param state 8路灰度合并的状态值
 * @param base_speed 循迹的基础目标转速(RPS)
 */
void Grayscale_Process(uint8_t state, float base_speed);

#endif
