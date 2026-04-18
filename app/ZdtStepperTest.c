#include "ZdtStepperTest.h"

#include "clock.h"
#include "oled_hardware_spi.h"
#include "ZdtStepper.h"

#include <stdint.h>

#define ZDT_TEST_ADDR_1 (1U)
#define ZDT_TEST_ADDR_2 (2U)
#define ZDT_TEST_ACC    (25U)
#define ZDT_TEST_VEL    (1500.0f)

static uint32_t g_sendCount;
static uint32_t g_startTick;

static void ZdtStepperTest_Motor1Run(void)
{
    ZdtStepper_ClearRx(ZDT_STEPPER_BJ1);
    ZdtStepper_VelControl(ZDT_STEPPER_BJ1, ZDT_TEST_ADDR_1, 1U, ZDT_TEST_ACC, ZDT_TEST_VEL, false);
    mspm0_delay_ms(10);
    ZdtStepper_SynchronousMotion(ZDT_STEPPER_BJ1, 0U);
}

static void ZdtStepperTest_Motor2Run(void)
{
    ZdtStepper_ClearRx(ZDT_STEPPER_BJ2);
    ZdtStepper_VelControl(ZDT_STEPPER_BJ2, ZDT_TEST_ADDR_2, 1U, ZDT_TEST_ACC, ZDT_TEST_VEL, false);
    mspm0_delay_ms(10);
    ZdtStepper_SynchronousMotion(ZDT_STEPPER_BJ2, 0U);
}

void ZdtStepperTest_Init(void)
{
    g_sendCount = 0U;
    g_startTick = tick_ms;

    ZdtStepper_Init();

    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t *) "BJ2+AD2", 16);
    OLED_ShowString(0, 2, (uint8_t *) "Vel:", 16);
    OLED_ShowString(0, 4, (uint8_t *) "Cnt:", 16);
    OLED_ShowString(0, 6, (uint8_t *) "TO:", 16);
}

void ZdtStepperTest_Update(void)
{
    if ((g_sendCount == 0U) && ((tick_ms - g_startTick) >= 500U)) {
        ZdtStepperTest_Motor2Run();
        g_sendCount = 1U;
    }

    OLED_ShowNum(40, 2, (uint32_t) ZDT_TEST_VEL, 5, 16);
    OLED_ShowNum(40, 4, g_sendCount, 5, 16);
    OLED_ShowNum(24, 6, ZdtStepper_GetTxTimeoutCount(ZDT_STEPPER_BJ2), 5, 16);
}
