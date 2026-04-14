#include "ti_msp_dl_config.h"
#include "interrupt.h"
#include "bno08x_uart_rvc.h"
#include "clock.h"
#include "wit.h"

uint8_t enable_group1_irq = 0;

void Interrupt_Init(void)
{
    if (enable_group1_irq) {
        NVIC_EnableIRQ(GPIOB_INT_IRQn);
    }
}

void SysTick_Handler(void)
{
    tick_ms++;
}

void TIMA0_IRQHandler(void)
{
    DL_TimerA_clearInterruptStatus(TIMER_0_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT);
}

#if defined UART_BNO08X_INST_IRQHandler
void UART_BNO08X_INST_IRQHandler(void)
{
    static uint8_t packet[19];
    static uint8_t index = 0;

    while (DL_UART_Main_isRXFIFOEmpty(UART_BNO08X_INST) == false)
    {
        uint8_t byte = DL_UART_Main_receiveData(UART_BNO08X_INST);

        if (index == 0U) {
            if (byte != 0xAAU) {
                continue;
            }
            packet[index++] = byte;
            continue;
        }

        if ((index == 1U) && (byte != 0xAAU)) {
            index = 0U;
            continue;
        }

        packet[index++] = byte;
        if (index >= sizeof(packet)) {
            uint8_t checkSum = 0;

            for (uint8_t i = 2U; i <= 14U; i++) {
                checkSum += packet[i];
            }

            if (checkSum == packet[18]) {
                bno08x_data.index = packet[2];
                bno08x_data.yaw = (int16_t)((packet[4] << 8) | packet[3]) / 100.0f;
                bno08x_data.pitch = (int16_t)((packet[6] << 8) | packet[5]) / 100.0f;
                bno08x_data.roll = (int16_t)((packet[8] << 8) | packet[7]) / 100.0f;
                bno08x_data.ax = (int16_t)((packet[10] << 8) | packet[9]);
                bno08x_data.ay = (int16_t)((packet[12] << 8) | packet[11]);
                bno08x_data.az = (int16_t)((packet[14] << 8) | packet[13]);
            }

            index = 0U;
        }
    }
}
#endif

#if defined UART_WIT_INST_IRQHandler
void UART_WIT_INST_IRQHandler(void)
{
    uint8_t checkSum, packCnt = 0;
    extern uint8_t wit_dmaBuffer[33];

    DL_DMA_disableChannel(DMA, DMA_WIT_CHAN_ID);
    uint8_t rxSize = 32 - DL_DMA_getTransferSize(DMA, DMA_WIT_CHAN_ID);

    if(DL_UART_isRXFIFOEmpty(UART_WIT_INST) == false)
        wit_dmaBuffer[rxSize++] = DL_UART_receiveData(UART_WIT_INST);

    while(rxSize >= 11)
    {
        checkSum=0;
        for(int i=packCnt*11; i<(packCnt+1)*11-1; i++)
            checkSum += wit_dmaBuffer[i];

        if((wit_dmaBuffer[packCnt*11] == 0x55) && (checkSum == wit_dmaBuffer[packCnt*11+10]))
        {
            if(wit_dmaBuffer[packCnt*11+1] == 0x51)
            {
                wit_data.ax = (int16_t)((wit_dmaBuffer[packCnt*11+3]<<8)|wit_dmaBuffer[packCnt*11+2]) / 2.048; //mg
                wit_data.ay = (int16_t)((wit_dmaBuffer[packCnt*11+5]<<8)|wit_dmaBuffer[packCnt*11+4]) / 2.048; //mg
                wit_data.az = (int16_t)((wit_dmaBuffer[packCnt*11+7]<<8)|wit_dmaBuffer[packCnt*11+6]) / 2.048; //mg
                wit_data.temperature =  (int16_t)((wit_dmaBuffer[packCnt*11+9]<<8)|wit_dmaBuffer[packCnt*11+8]) / 100.0; //°C
            }
            else if(wit_dmaBuffer[packCnt*11+1] == 0x52)
            {
                wit_data.gx = (int16_t)((wit_dmaBuffer[packCnt*11+3]<<8)|wit_dmaBuffer[packCnt*11+2]) / 16.384; //°/S
                wit_data.gy = (int16_t)((wit_dmaBuffer[packCnt*11+5]<<8)|wit_dmaBuffer[packCnt*11+4]) / 16.384; //°/S
                wit_data.gz = (int16_t)((wit_dmaBuffer[packCnt*11+7]<<8)|wit_dmaBuffer[packCnt*11+6]) / 16.384; //°/S
            }
            else if(wit_dmaBuffer[packCnt*11+1] == 0x53)
            {
                wit_data.roll  = (int16_t)((wit_dmaBuffer[packCnt*11+3]<<8)|wit_dmaBuffer[packCnt*11+2]) / 32768.0 * 180.0; //°
                wit_data.pitch = (int16_t)((wit_dmaBuffer[packCnt*11+5]<<8)|wit_dmaBuffer[packCnt*11+4]) / 32768.0 * 180.0; //°
                wit_data.yaw   = (int16_t)((wit_dmaBuffer[packCnt*11+7]<<8)|wit_dmaBuffer[packCnt*11+6]) / 32768.0 * 180.0; //°
                wit_data.version = (int16_t)((wit_dmaBuffer[packCnt*11+9]<<8)|wit_dmaBuffer[packCnt*11+8]);
            }
        }

        rxSize -= 11;
        packCnt++;
    }
    
    uint8_t dummy[4];
    DL_UART_drainRXFIFO(UART_WIT_INST, dummy, 4);

    DL_DMA_setDestAddr(DMA, DMA_WIT_CHAN_ID, (uint32_t) &wit_dmaBuffer[0]);
    DL_DMA_setTransferSize(DMA, DMA_WIT_CHAN_ID, 32);
    DL_DMA_enableChannel(DMA, DMA_WIT_CHAN_ID);
}
#endif

// void GROUP1_IRQHandler(void)
// {
//     switch (DL_Interrupt_getPendingGroup(DL_INTERRUPT_GROUP_1)) {
//         /* MPU6050 INT */
//         #if defined GPIO_MPU6050_PORT
//             #if defined GPIO_MPU6050_INT_IIDX
//             case GPIO_MPU6050_INT_IIDX:
//             #elif (GPIO_MPU6050_PORT == GPIOA) && (defined GPIO_MULTIPLE_GPIOA_INT_IIDX)
//             case GPIO_MULTIPLE_GPIOA_INT_IIDX:
//             #elif (GPIO_MPU6050_PORT == GPIOB) && (defined GPIO_MULTIPLE_GPIOB_INT_IIDX)
//             case GPIO_MULTIPLE_GPIOB_INT_IIDX:
//             #endif
//                 Read_Quad();
//                 break;
//         #endif
//     }
// }
