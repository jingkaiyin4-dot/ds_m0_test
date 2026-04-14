/*
 * SysConfig Configuration Steps:
 *   UART:
 *     1. Add an UART module.
 *     2. Name it as "UART_BNO08X".
 *     3. Set "Target Baud Rate" to "115200".
 *     4. Set "Communication Direction " to "RX only".
 *     5. Check the box "Enable FIFOs".
 *     6. Set "RX Timeout Interrupt Counts" to "1".
 *     7. Check the "RX timeout" box at "Enable Interrupts".
 *     8. Set "Configure DMA RX Trigger" to "UART RX interrupt".
 *     9. Set "DMA Channel RX Name" to "DMA_BNO08X".
 *     10. Set "Address Mode" to "Fixed addr. to Block addr.".
 *     11. Set "Source Length" and "Destination Length" to "Byte".
 *     12. Set the pin according to your needs.
 */

#ifndef _BNO08X_UART_RVC_H_
#define _BNO08X_UART_RVC_H_

#include "ti_msp_dl_config.h"

#if !defined(UART_BNO08X_INST) && defined(UART_1l_INST)
#define UART_BNO08X_INST UART_1l_INST
#define UART_BNO08X_INST_INT_IRQN UART_1l_INST_INT_IRQN
#define UART_BNO08X_INST_IRQHandler UART_1l_INST_IRQHandler
#endif

typedef struct {
    uint8_t index;
    float yaw;
    float pitch;
    float roll;
    int16_t ax;
    int16_t ay;
    int16_t az;
} BNO08X_RVC_Data_t;

#if defined UART_BNO08X_INST
void BNO08X_Init(void);
extern BNO08X_RVC_Data_t bno08x_data;
#endif

#endif  /* #ifndef _BNO08X_UART_RVC_H_ */
