#include "Grayscale.h"
#include "../PID/PID.h"
#include "../motor/Motor.h"
#include "ti_msp_dl_config.h"

uint8_t Grayscale_ReadBinary(void) {
  uint8_t binary = 0;

  /**
   * 将8个引脚的状态读取并拼装为8位二进制数
   * 左侧(HUIDU_1) 对应最高位(Bit 7)
   * 右侧(HUIDU_8) 对应最低位(Bit 0)
   */
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_1_PORT, GPIO_HUIDU_PIN_HUIDU_1_PIN))
    binary |= (1 << 7);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_2_PORT, GPIO_HUIDU_PIN_HUIDU_2_PIN))
    binary |= (1 << 6);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_3_PORT, GPIO_HUIDU_PIN_HUIDU_3_PIN))
    binary |= (1 << 5);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_4_PORT, GPIO_HUIDU_PIN_HUIDU_4_PIN))
    binary |= (1 << 4);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_5_PORT, GPIO_HUIDU_PIN_HUIDU_5_PIN))
    binary |= (1 << 3);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_6_PORT, GPIO_HUIDU_PIN_HUIDU_6_PIN))
    binary |= (1 << 2);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_7_PORT, GPIO_HUIDU_PIN_HUIDU_7_PIN))
    binary |= (1 << 1);
  if (DL_GPIO_readPins(GPIO_HUIDU_PIN_HUIDU_8_PORT, GPIO_HUIDU_PIN_HUIDU_8_PIN))
    binary |= (1 << 0);

  return binary;
}

void Grayscale_Process(uint8_t state, float base_speed) {
  float left_speed = base_speed;
  float right_speed = base_speed;
  bool should_stop = false;

  /**
   * 根据 8位状态值 进行转向处理
   * 这里直接将速度“给死”（绝对值赋值），避免累加或计算误差
   */
  switch (state) {
  /* 直行状态 */
  case 0b00011000: // 0x18: 完美居中
    left_speed = 3.0f;
    right_speed = 3.0f;
    break;
  case 0b00010000: // 0x10: 略微偏左
    left_speed = 2.5f;
    right_speed = 3.0f;
    break;
  case 0b00001000: // 0x08: 略微偏右
    left_speed = 3.0f;
    right_speed = 2.5f;
    break;

  /* 需要左转 (减速左轮，加速右轮) */
  case 0b00110000: // 0x30
    left_speed = 3.0f;
    right_speed = 3.5f;
    break;
  case 0b01100000: // 0x60
    left_speed = 2.0f;
    right_speed = 4.0f;
    break;
  case 0b00100000: // 0x20
    left_speed = 2.0f;
    right_speed = 4.2f;
    break;
  case 0b11000000:     // 0xC0
    left_speed = 1.0f; // 左轮轻微反转帮助急转
    right_speed = 4.0f;
    break;

  case 0b10000000: // 0x80: 极其偏右，急左转
    left_speed = .0f;
    right_speed = 4.0f;
    break;

  /* 需要右转 (加速左轮，减速右轮) */
  case 0b00001100: // 0x0C
    left_speed = 3.0f;
    right_speed = 3.0f;
    break;
  case 0b00000010: // 0x02
    left_speed = 4.5f;
    right_speed = 1.0f;
    break;
  case 0b00000110: // 0x06
    left_speed = 5.5f;
    right_speed = 1.0f;
    break;
    //   case 0b00000011: // 0x03
    //     left_speed = 7.5f;
    //     right_speed = 1.0f; // 右轮轻微反转帮助急转
    //     break;
  //   case 0b00000001: // 0x01: 极其偏左，急右转
  //     left_speed = 8.5f;
  //     right_speed = 1.0f;
  //     break;
  case 0b00000001: // 0x01: 极其偏左，急右转
    left_speed = 0.0f;
    right_speed = 0.0f;
    break;
  case 0b11111110: // 0x01: 极其偏左，急右转
    left_speed = 0.0f;
    right_speed = 0.0f;
    break;
  /* 停止情况处理 */
  case 0b11111111: // 0xFF: 遇到十字路口或停止线(全黑)
  case 0b00000000: // 0x00: 丢线(全白)
    should_stop = true;
    break;

  default:
    left_speed = base_speed;
    right_speed = base_speed;
    break;
  }

  if (should_stop) {
    /** 直接调用底层函数停止电机输出 */
    Motor_StopAll();
    g_driveController.left.speed.target = 0.0f;
    g_driveController.right.speed.target = 0.0f;
  } else {
    /** 速度限幅保护 */
    if (left_speed > 10.0f)
      left_speed = 10.0f;
    if (left_speed < -10.0f)
      left_speed = -10.0f;
    if (right_speed > 10.0f)
      right_speed = 10.0f;
    if (right_speed < -10.0f)
      right_speed = -10.0f;

    /** 设定速度环PID的目标速度 */
    g_driveController.left.speed.target = left_speed;
    g_driveController.right.speed.target = right_speed;
  }
}
