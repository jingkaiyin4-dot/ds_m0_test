#ifndef _APP_BUS_H_
#define _APP_BUS_H_

#include "AppTypes.h"

#include <stdint.h>

typedef struct {
    AppBusDevice device;
    uint8_t reg;
    uint8_t *buffer;
    uint16_t length;
} AppBusTransfer;

typedef struct {
    int status;
    uint16_t transferred;
} AppBusResult;

#endif
