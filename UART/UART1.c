#include "UART1.h"

#include "ti_msp_dl_config.h"

static UART_CAM_RxHandler g_uart_cam_handlers[3];
static UART_CAM_Mode g_uart_cam_mode = UART_CAM_MODE_NONE;
static uint8_t g_uart_cam_initialized = 0U;

void UART_CAM_ServiceInit(void)
{
    if (g_uart_cam_initialized != 0U) {
        return;
    }

    SYSCFG_DL_UART_CAM_init();
    NVIC_ClearPendingIRQ(UART_CAM_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_CAM_INST_INT_IRQN);
    g_uart_cam_initialized = 1U;
}

void UART_CAM_RegisterHandler(UART_CAM_Mode mode, UART_CAM_RxHandler handler)
{
    if ((uint32_t)mode < (sizeof(g_uart_cam_handlers) / sizeof(g_uart_cam_handlers[0]))) {
        g_uart_cam_handlers[(uint32_t)mode] = handler;
    }
}

void UART_CAM_SelectMode(UART_CAM_Mode mode)
{
    UART_CAM_ServiceInit();
    g_uart_cam_mode = mode;
    UART_CAM_ClearRxFifo();
    NVIC_ClearPendingIRQ(UART_CAM_INST_INT_IRQN);
}

UART_CAM_Mode UART_CAM_GetMode(void)
{
    return g_uart_cam_mode;
}

void UART_CAM_ClearRxFifo(void)
{
    uint8_t dummy[8];

    DL_UART_drainRXFIFO(UART_CAM_INST, dummy, sizeof(dummy));
}

void UART_CAM_SendByte(uint8_t data)
{
    UART_CAM_ServiceInit();
    DL_UART_transmitData(UART_CAM_INST, data);
}

void UART_CAM_SendBuffer(const uint8_t *data, uint8_t length)
{
    uint8_t i;

    UART_CAM_ServiceInit();
    for (i = 0; i < length; i++) {
        DL_UART_transmitData(UART_CAM_INST, data[i]);
    }
}

void UART_CAM_INST_IRQHandler(void)
{
    UART_CAM_RxHandler handler = 0;

    if ((uint32_t)g_uart_cam_mode < (sizeof(g_uart_cam_handlers) / sizeof(g_uart_cam_handlers[0]))) {
        handler = g_uart_cam_handlers[(uint32_t)g_uart_cam_mode];
    }

    while (DL_UART_Main_isRXFIFOEmpty(UART_CAM_INST) == false) {
        uint8_t data = DL_UART_Main_receiveData(UART_CAM_INST);

        if (handler != 0) {
            handler(data);
        }
    }
}
