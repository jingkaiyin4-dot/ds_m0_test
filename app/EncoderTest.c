#include "EncoderTest.h"

#include "ti_msp_dl_config.h"
#include "CameraControl.h"
#include "Encoder.h"
#include "Motor.h"
#include "oled_hardware_spi.h"

#include <stdint.h>

static int32_t g_leftEncoderCount;

static void OLED_ShowSignedValueCompact(uint8_t x, uint8_t y, int32_t value)
{
    uint32_t magnitude;

    /* 先清掉这一行旧内容，避免正负号位数变化时残留字符。 */
    for (uint8_t i = 0; i < 6U; i++) {
        OLED_ShowChar(x + (i * 8U), y, ' ', 16);
    }

    if (value < 0) {
        OLED_ShowChar(x, y, '-', 16);
        magnitude = (uint32_t) (-value);
    } else {
        OLED_ShowChar(x, y, '+', 16);
        magnitude = (uint32_t) value;
    }

    OLED_ShowNum(x + 8U, y, magnitude % 100000U, 5U, 16);
}

void EncoderTest_Init(void)
{
    g_leftEncoderCount = 0;

    OLED_Clear();
    /*
     * CM: 最近一次相机命令
     * LC: 左轮累计编码器计数
     * LD: 左轮本周期编码器增量
     * LM: 左轮当前电机占空比
     */
    OLED_ShowString(0, 0, (uint8_t *) "CM", 16);
    OLED_ShowString(0, 2, (uint8_t *) "LC", 16);
    OLED_ShowString(0, 4, (uint8_t *) "LD", 16);
    OLED_ShowString(0, 6, (uint8_t *) "LM", 16);
}

void EncoderTest_Update(void)
{
    /* 读出左轮本周期增量。Encoder_GetDelta 会在读取后自动清零。 */
    int32_t leftDelta = Encoder_GetDelta(ENCODER_LEFT);

    g_leftEncoderCount += leftDelta;

    /* 这个测试页主要用于观察“相机控制 -> 电机 duty -> 编码器反馈”是否一致。 */
    OLED_ShowChar(32, 0, CameraControl_GetLastCommand(), 16);
    OLED_ShowSignedValueCompact(32, 2, g_leftEncoderCount);
    OLED_ShowSignedValueCompact(32, 4, leftDelta);
    OLED_ShowSignedValueCompact(32, 6, CameraControl_GetLeftMotorDuty());
}

void EncoderTest_Stop(void)
{
    Motor_StopAll();
}
