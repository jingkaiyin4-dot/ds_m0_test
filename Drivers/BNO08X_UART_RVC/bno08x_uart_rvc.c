#include "bno08x_uart_rvc.h"

#include <string.h>

#if defined UART_BNO08X_INST

BNO08X_RVC_Data_t bno08x_data;

void BNO08X_Init(void)
{
    memset(&bno08x_data, 0, sizeof(bno08x_data));
    NVIC_EnableIRQ(UART_BNO08X_INST_INT_IRQN);
}

#endif
