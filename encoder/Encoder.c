#include "ti_msp_dl_config.h"
#include "Encoder.h"

#include <stdbool.h>

static volatile int32_t g_encoderCounts[2];

/*
 * Physical wiring uses PB3/PB5 for the right encoder and PB6/PB7 for the left
 * encoder, even though the SysConfig-generated pin names still use LA/LB/RA/RB.
 */
#define ENCODER_RIGHT_A_PIN Encoder_PIN_LA_PIN
#define ENCODER_RIGHT_B_PIN Encoder_PIN_LB_PIN
#define ENCODER_LEFT_A_PIN  Encoder_PIN_RA_PIN
#define ENCODER_LEFT_B_PIN  Encoder_PIN_RB_PIN

static void Encoder_Update(EncoderChannel channel, bool forward)
{
    if (forward) {
        g_encoderCounts[channel]++;
    } else {
        g_encoderCounts[channel]--;
    }
}

void GROUP1_IRQHandler(void)
{
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(
        Encoder_PORT,
        ENCODER_RIGHT_A_PIN | ENCODER_LEFT_A_PIN | GPIO_MPU6050_PIN_INT_PIN);

    if (pending & ENCODER_RIGHT_A_PIN) {
        bool forward = (DL_GPIO_readPins(Encoder_PORT, ENCODER_RIGHT_B_PIN) == 0U);
        Encoder_Update(ENCODER_RIGHT, forward);
        DL_GPIO_clearInterruptStatus(Encoder_PORT, ENCODER_RIGHT_A_PIN);
    }

    if (pending & ENCODER_LEFT_A_PIN) {
        bool forward = (DL_GPIO_readPins(Encoder_PORT, ENCODER_LEFT_B_PIN) != 0U);
        Encoder_Update(ENCODER_LEFT, forward);
        DL_GPIO_clearInterruptStatus(Encoder_PORT, ENCODER_LEFT_A_PIN);
    }

    if (pending & GPIO_MPU6050_PIN_INT_PIN) {
        DL_GPIO_clearInterruptStatus(Encoder_PORT, GPIO_MPU6050_PIN_INT_PIN);
    }
}

int32_t Encoder_GetDelta(EncoderChannel channel)
{
    int32_t delta;

    __disable_irq();
    delta = g_encoderCounts[channel];
    g_encoderCounts[channel] = 0;
    __enable_irq();

    return delta;
}

void Encoder_ResetAll(void)
{
    __disable_irq();
    g_encoderCounts[ENCODER_LEFT] = 0;
    g_encoderCounts[ENCODER_RIGHT] = 0;
    __enable_irq();
}
