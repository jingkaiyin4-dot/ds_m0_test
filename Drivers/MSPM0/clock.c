#include "ti_msp_dl_config.h"
#include "clock.h"

#include "ti/driverlib/dl_common.h"

volatile unsigned long tick_ms;
volatile uint32_t start_time;

int mspm0_delay_ms(unsigned long num_ms)
{
    if (num_ms != 0U) {
        DL_Common_delayCycles((uint32_t) num_ms * (CPUCLK_FREQ / 1000U));
    }
    return 0;
}

int mspm0_get_clock_ms(unsigned long *count)
{
    if (!count)
        return 1;
    count[0] = tick_ms;
    return 0;
}

void SysTick_Init(void)
{
    tick_ms = 0;
    DL_SYSTICK_config(CPUCLK_FREQ/1000);
    NVIC_SetPriority(SysTick_IRQn, 0);
    __enable_irq();
}
