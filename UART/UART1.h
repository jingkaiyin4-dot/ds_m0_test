#ifndef _UART1_H_
#define _UART1_H_
#include<stdio.h>
void UART1_Init();
void UART_1l_INST_IRQHandler(void);
extern uint8_t Serial_TxPacket[];
extern uint8_t Serial_RxPacket[];
uint32_t Serial_RxPacket_Decimal[4];

#endif
