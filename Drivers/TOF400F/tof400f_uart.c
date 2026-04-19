#include "tof400f.h"
#include "UART1.h"
#include "clock.h"

static uint8_t g_tof400f_ready = 0;
static uint8_t g_tof400f_rx_buf[7];
static uint8_t g_tof400f_rx_idx = 0;
static uint16_t g_tof400f_distance_mm = 0;
static uint16_t g_tof400f_irq_called = 0;
static uint8_t g_tof400f_last_char = 0;

static void TOF400F_OnRxByte(uint8_t c);

static uint16_t TOF400F_Crc16(const uint8_t *data, uint8_t len)
{
    uint16_t crc = 0xFFFF;
    uint8_t i;
    uint8_t j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            if ((crc & 0x0001U) != 0U) {
                crc = (crc >> 1) ^ 0xA001U;
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

void TOF400F_Query(void)
{
    static const uint8_t command[8] = {0x01, 0x03, 0x00, 0x10, 0x00, 0x01, 0xD5, 0xCA};

    UART_CAM_SendBuffer(command, sizeof(command));
}

void TOF400F_StartAuto(void)
{
    static const uint8_t command[8] = {0x01, 0x06, 0x00, 0x05, 0x01, 0xF4, 0x99, 0xDC};

    UART_CAM_SendBuffer(command, sizeof(command));
}

static void TOF400F_OnRxByte(uint8_t c)
{
    uint16_t crc_calc;
    uint16_t crc_rx;

    g_tof400f_irq_called++;
    g_tof400f_last_char = c;

    if (g_tof400f_rx_idx == 0U) {
        if (c != 0x01U) {
            return;
        }
    } else if (g_tof400f_rx_idx == 1U) {
        if (c != 0x03U) {
            g_tof400f_rx_idx = 0;
            if (c == 0x01U) {
                g_tof400f_rx_buf[g_tof400f_rx_idx++] = c;
            }
            return;
        }
    } else if (g_tof400f_rx_idx == 2U) {
        if (c != 0x02U) {
            g_tof400f_rx_idx = 0;
            if (c == 0x01U) {
                g_tof400f_rx_buf[g_tof400f_rx_idx++] = c;
            }
            return;
        }
    }

    g_tof400f_rx_buf[g_tof400f_rx_idx++] = c;

    if (g_tof400f_rx_idx >= sizeof(g_tof400f_rx_buf)) {
        crc_calc = TOF400F_Crc16(g_tof400f_rx_buf, 5);
        crc_rx = ((uint16_t)g_tof400f_rx_buf[6] << 8) | g_tof400f_rx_buf[5];

        if (crc_calc == crc_rx) {
            g_tof400f_distance_mm = ((uint16_t)g_tof400f_rx_buf[3] << 8) | g_tof400f_rx_buf[4];
            g_tof400f_ready = 1;
        }

        g_tof400f_rx_idx = 0;
    }
}

void TOF400F_Init(void)
{
    g_tof400f_ready = 0;
    g_tof400f_rx_idx = 0;
    g_tof400f_distance_mm = 0;
    g_tof400f_irq_called = 0;
    g_tof400f_last_char = 0;

    UART_CAM_ServiceInit();
    UART_CAM_RegisterHandler(UART_CAM_MODE_TOF400F, TOF400F_OnRxByte);
}

void TOF400F_EnableUart(void)
{
    g_tof400f_ready = 0;
    g_tof400f_rx_idx = 0;
    UART_CAM_SelectMode(UART_CAM_MODE_TOF400F);
}

void TOF400F_DisableUart(void)
{
    if (UART_CAM_GetMode() == UART_CAM_MODE_TOF400F) {
        UART_CAM_SelectMode(UART_CAM_MODE_NONE);
    }
}

uint16_t TOF400F_ReadDistanceMm(void)
{
    return g_tof400f_distance_mm;
}

uint8_t TOF400F_IsReady(void)
{
    return g_tof400f_ready;
}

uint16_t TOF400F_GetIrqCount(void)
{
    return g_tof400f_irq_called;
}

uint8_t TOF400F_GetLastChar(void)
{
    return g_tof400f_last_char;
}
