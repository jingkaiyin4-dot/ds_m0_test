#ifndef TOF400F_H
#define TOF400F_H

#include <stdint.h>

void TOF400F_Init(void);
void TOF400F_EnableUart(void);
void TOF400F_DisableUart(void);
void TOF400F_Query(void);
void TOF400F_StartAuto(void);
uint16_t TOF400F_ReadDistanceMm(void);
uint8_t TOF400F_IsReady(void);
uint16_t TOF400F_GetIrqCount(void);
uint8_t TOF400F_GetLastChar(void);
uint8_t TOF400F_GetTxCount(void);

#endif
