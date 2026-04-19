#ifndef UART1_H
#define UART1_H

#include <stdint.h>

typedef enum {
    UART_CAM_MODE_NONE = 0,
    UART_CAM_MODE_TOF400F,
    UART_CAM_MODE_CAMERA_CONTROL
} UART_CAM_Mode;

typedef void (*UART_CAM_RxHandler)(uint8_t data);

void UART_CAM_ServiceInit(void);
void UART_CAM_RegisterHandler(UART_CAM_Mode mode, UART_CAM_RxHandler handler);
void UART_CAM_SelectMode(UART_CAM_Mode mode);
UART_CAM_Mode UART_CAM_GetMode(void);
void UART_CAM_ClearRxFifo(void);
void UART_CAM_SendByte(uint8_t data);
void UART_CAM_SendBuffer(const uint8_t *data, uint8_t length);

#endif
