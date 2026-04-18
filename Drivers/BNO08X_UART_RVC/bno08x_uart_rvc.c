#include "bno08x_uart_rvc.h"

#if defined UART_BNO08X_INST

uint8_t bno08x_dmaBuffer[19];
BNO08X_RVC_Data_t bno08x_data;

void BNO08X_Init(void)
{
    static const DL_DMA_Config bno08xDmaConfig = {
        .transferMode = DL_DMA_SINGLE_TRANSFER_MODE,
        .extendedMode = DL_DMA_NORMAL_MODE,
        .destIncrement = DL_DMA_ADDR_INCREMENT,
        .srcIncrement = DL_DMA_ADDR_UNCHANGED,
        .destWidth = DL_DMA_WIDTH_BYTE,
        .srcWidth = DL_DMA_WIDTH_BYTE,
        .trigger = UART_BNO08_INST_DMA_TRIGGER,
        .triggerType = DL_DMA_TRIGGER_TYPE_EXTERNAL,
    };

    bno08x_data.index = 0U;
    bno08x_data.pitch = 0.0f;
    bno08x_data.roll = 0.0f;
    bno08x_data.yaw = 0.0f;
    bno08x_data.ax = 0;
    bno08x_data.ay = 0;
    bno08x_data.az = 0;

    for (uint8_t i = 0U; i < 19U; i++) {
        bno08x_dmaBuffer[i] = 0U;
    }

    /*
     * Force the UART runtime behavior to match the known-good DMA example,
     * even if SysConfig still generated plain RX interrupt settings.
     */
    DL_UART_Main_disableInterrupt(UART_BNO08X_INST, DL_UART_MAIN_INTERRUPT_RX);
    DL_UART_Main_enableInterrupt(UART_BNO08X_INST, DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR);
    DL_UART_Main_enableDMAReceiveEvent(UART_BNO08X_INST, DL_UART_DMA_INTERRUPT_RX);
    DL_UART_Main_enableFIFOs(UART_BNO08X_INST);
    DL_UART_Main_setRXFIFOThreshold(UART_BNO08X_INST, DL_UART_RX_FIFO_LEVEL_1_2_FULL);
    DL_UART_Main_setRXInterruptTimeout(UART_BNO08X_INST, 1);

    /* SysConfig currently generated an incompatible DMA layout for BNO08X. */
    DL_DMA_initChannel(DMA, DMA_BNO08X_CHAN_ID, (DL_DMA_Config *) &bno08xDmaConfig);

    DL_DMA_setSrcAddr(DMA, DMA_BNO08X_CHAN_ID, (uint32_t) (&UART_BNO08X_INST->RXDATA));
    DL_DMA_setDestAddr(DMA, DMA_BNO08X_CHAN_ID, (uint32_t) &bno08x_dmaBuffer[0]);
    DL_DMA_setTransferSize(DMA, DMA_BNO08X_CHAN_ID, 18);
    DL_DMA_enableChannel(DMA, DMA_BNO08X_CHAN_ID);

    NVIC_EnableIRQ(UART_BNO08X_INST_INT_IRQN);
}

#endif
