#ifndef _ENCODER_H_
#define _ENCODER_H_

#include <stdint.h>

typedef enum {
    ENCODER_LEFT = 0,
    ENCODER_RIGHT = 1,
} EncoderChannel;

void GROUP1_IRQHandler(void);
int32_t Encoder_GetDelta(EncoderChannel channel);
void Encoder_ResetAll(void);

#endif
