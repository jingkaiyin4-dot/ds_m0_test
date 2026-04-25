#include "oled_hardware_spi.h"
#include "oledfont.h"
#include "clock.h"

#include <stdbool.h>

#define ST7789_WIDTH            240U
#define ST7789_HEIGHT           320U
#define ST7789_X_OFFSET         0U
#define ST7789_Y_OFFSET         0U

static bool g_displayInverted;
static bool g_displayRotated180;
static uint16_t g_oledFgColor = OLED_COLOR_WHITE;
static uint16_t g_oledBgColor = OLED_COLOR_BLACK;

static void ST7789_WriteCommand(uint8_t cmd)
{
    while (DL_SPI_isBusy(SPI_OLED_INST));
    while (DL_SPI_isTXFIFOFull(SPI_OLED_INST));

    OLED_DC_Clr();
    OLED_CS_Clr();
    DL_SPI_transmitData8(SPI_OLED_INST, cmd);
    while (DL_SPI_isBusy(SPI_OLED_INST));
    while (!DL_SPI_isTXFIFOEmpty(SPI_OLED_INST));
    OLED_CS_Set();
}

static void ST7789_WriteData(uint8_t data)
{
    while (DL_SPI_isBusy(SPI_OLED_INST));
    while (DL_SPI_isTXFIFOFull(SPI_OLED_INST));

    OLED_DC_Set();
    OLED_CS_Clr();
    DL_SPI_transmitData8(SPI_OLED_INST, data);
    while (DL_SPI_isBusy(SPI_OLED_INST));
    while (!DL_SPI_isTXFIFOEmpty(SPI_OLED_INST));
    OLED_CS_Set();
}

static void ST7789_WriteColor(uint16_t color)
{
    ST7789_WriteData((uint8_t) (color >> 8));
    ST7789_WriteData((uint8_t) color);
}

static void ST7789_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    x0 += ST7789_X_OFFSET;
    x1 += ST7789_X_OFFSET;
    y0 += ST7789_Y_OFFSET;
    y1 += ST7789_Y_OFFSET;

    ST7789_WriteCommand(0x2A);
    ST7789_WriteData((uint8_t) (x0 >> 8));
    ST7789_WriteData((uint8_t) x0);
    ST7789_WriteData((uint8_t) (x1 >> 8));
    ST7789_WriteData((uint8_t) x1);

    ST7789_WriteCommand(0x2B);
    ST7789_WriteData((uint8_t) (y0 >> 8));
    ST7789_WriteData((uint8_t) y0);
    ST7789_WriteData((uint8_t) (y1 >> 8));
    ST7789_WriteData((uint8_t) y1);

    ST7789_WriteCommand(0x2C);
}

static void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
    uint16_t color)
{
    uint32_t pixelCount;

    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT) || (width == 0U) || (height == 0U)) {
        return;
    }

    if ((x + width) > ST7789_WIDTH) {
        width = (uint16_t) (ST7789_WIDTH - x);
    }
    if ((y + height) > ST7789_HEIGHT) {
        height = (uint16_t) (ST7789_HEIGHT - y);
    }

    ST7789_SetAddressWindow(x, y, (uint16_t) (x + width - 1U), (uint16_t) (y + height - 1U));
    pixelCount = (uint32_t) width * (uint32_t) height;
    while (pixelCount-- > 0U) {
        ST7789_WriteColor(color);
    }
}

static void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT)) {
        return;
    }

    ST7789_SetAddressWindow(x, y, x, y);
    ST7789_WriteColor(color);
}

static void ST7789_SetRotation(bool rotated180)
{
    uint8_t madctl = rotated180 ? 0xC0U : 0x00U;

    ST7789_WriteCommand(0x36);
    ST7789_WriteData(madctl);
}

static void ST7789_DrawGlyph6x8(uint16_t x, uint16_t y, uint8_t chr, uint16_t fgColor,
    uint16_t bgColor)
{
    uint8_t row;

    if ((chr < ' ') || (chr > '~')) {
        chr = ' ';
    }

    ST7789_SetAddressWindow(x, y, (uint16_t) (x + 5U), (uint16_t) (y + 7U));
    for (row = 0U; row < 8U; row++) {
        uint8_t column;

        for (column = 0U; column < 6U; column++) {
            uint8_t bits = asc2_0806[chr - ' '][column];
            uint16_t color = ((bits >> row) & 0x01U) ? fgColor : bgColor;
            ST7789_WriteColor(color);
        }
    }
}

static void ST7789_DrawGlyph8x16(uint16_t x, uint16_t y, uint8_t chr, uint16_t fgColor,
    uint16_t bgColor)
{
    uint8_t row;

    if ((chr < ' ') || (chr > '~')) {
        chr = ' ';
    }

    ST7789_SetAddressWindow(x, y, (uint16_t) (x + 7U), (uint16_t) (y + 15U));
    for (row = 0U; row < 8U; row++) {
        uint8_t column;

        for (column = 0U; column < 8U; column++) {
            uint8_t topBits = asc2_1608[chr - ' '][column];
            uint16_t topColor = ((topBits >> row) & 0x01U) ? fgColor : bgColor;
            ST7789_WriteColor(topColor);
        }
    }

    for (row = 0U; row < 8U; row++) {
        uint8_t column;

        for (column = 0U; column < 8U; column++) {
            uint8_t bottomBits = asc2_1608[chr - ' '][column + 8U];
            uint16_t bottomColor = ((bottomBits >> row) & 0x01U) ? fgColor : bgColor;
            ST7789_WriteColor(bottomColor);
        }
    }
}

void delay_ms(uint32_t ms)
{
    mspm0_delay_ms(ms);
}

void OLED_ColorTurn(uint8_t i)
{
    g_displayInverted = (i != 0U);
    ST7789_WriteCommand(g_displayInverted ? 0x21U : 0x20U);
}

void OLED_DisplayTurn(uint8_t i)
{
    g_displayRotated180 = (i != 0U);
    ST7789_SetRotation(g_displayRotated180);
}

void OLED_WR_Byte(uint8_t dat, uint8_t cmd)
{
    if (cmd == OLED_DATA) {
        ST7789_WriteData(dat);
    } else {
        ST7789_WriteCommand(dat);
    }
}

void OLED_Set_Pos(uint8_t x, uint8_t y)
{
    uint16_t pixelY = (uint16_t) y * 8U;
    ST7789_SetAddressWindow(x, pixelY, x, pixelY);
}

void OLED_Display_On(void)
{
    OLED_BLK_Set();
    ST7789_WriteCommand(0x29);
}

void OLED_Display_Off(void)
{
    ST7789_WriteCommand(0x28);
    OLED_BLK_Clr();
}

void OLED_Clear(void)
{
    ST7789_FillRect(0U, 0U, ST7789_WIDTH, ST7789_HEIGHT, g_oledBgColor);
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t sizey)
{
    uint16_t pixelX = x;
    uint16_t pixelY = (sizey == 16U) ? ((uint16_t) y * 8U) : ((uint16_t) y * 8U);

    if (sizey == 8U) {
        ST7789_DrawGlyph6x8(pixelX, pixelY, chr, g_oledFgColor, g_oledBgColor);
    } else if (sizey == 16U) {
        ST7789_DrawGlyph8x16(pixelX, pixelY, chr, g_oledFgColor, g_oledBgColor);
    }
}

uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- > 0U) {
        result *= m;
    }

    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t sizey)
{
    uint8_t t;
    uint8_t temp;
    uint8_t m = 0U;
    uint8_t enshow = 0U;

    if (sizey == 8U) {
        m = 2U;
    }

    for (t = 0U; t < len; t++) {
        temp = (uint8_t) ((num / oled_pow(10U, len - t - 1U)) % 10U);
        if ((enshow == 0U) && (t < (len - 1U))) {
            if (temp == 0U) {
                OLED_ShowChar((uint8_t) (x + (sizey / 2U + m) * t), y, ' ', sizey);
                continue;
            }
            enshow = 1U;
        }
        OLED_ShowChar((uint8_t) (x + (sizey / 2U + m) * t), y, (uint8_t) (temp + '0'), sizey);
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t sizey)
{
    uint8_t j = 0U;

    while (chr[j] != '\0') {
        OLED_ShowChar(x, y, chr[j++], sizey);
        if (sizey == 8U) {
            x = (uint8_t) (x + 6U);
        } else {
            x = (uint8_t) (x + (sizey / 2U));
        }
    }
}

void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t no, uint8_t sizey)
{
    uint16_t row;
    uint16_t pixelY = (uint16_t) y * 8U;

    if ((sizey != 16U) || (no >= (sizeof(Hzk) / sizeof(Hzk[0])))) {
        return;
    }

    ST7789_SetAddressWindow(x, pixelY, (uint16_t) (x + 15U), (uint16_t) (pixelY + 15U));
    for (row = 0U; row < 8U; row++) {
        uint16_t col;

        for (col = 0U; col < 16U; col++) {
            uint8_t topBits = Hzk[no][col];
            uint16_t topColor = ((topBits >> row) & 0x01U) ? g_oledFgColor : g_oledBgColor;
            ST7789_WriteColor(topColor);
        }
    }

    for (row = 0U; row < 8U; row++) {
        uint16_t col;

        for (col = 0U; col < 16U; col++) {
            uint8_t bottomBits = Hzk[no][col + 16U];
            uint16_t bottomColor = ((bottomBits >> row) & 0x01U) ? g_oledFgColor : g_oledBgColor;
            ST7789_WriteColor(bottomColor);
        }
    }
}

void OLED_DrawBMP(uint8_t x, uint8_t y, uint8_t sizex, uint8_t sizey, uint8_t BMP[])
{
    uint16_t byteIndex = 0U;
    uint16_t pageCount = (uint16_t) (sizey / 8U + ((sizey % 8U) ? 1U : 0U));
    uint16_t page;

    for (page = 0U; page < pageCount; page++) {
        uint16_t col;

        for (col = 0U; col < sizex; col++) {
            uint8_t bits = BMP[byteIndex++];
            uint8_t bit;

            for (bit = 0U; bit < 8U; bit++) {
                uint16_t drawY = (uint16_t) y * 8U + page * 8U + bit;
                if (drawY >= ((uint16_t) y * 8U + sizey)) {
                    break;
                }
                ST7789_DrawPixel((uint16_t) x + col, drawY,
                    ((bits >> bit) & 0x01U) ? g_oledFgColor : g_oledBgColor);
            }
        }
    }
}

void OLED_SetTheme(uint16_t fgColor, uint16_t bgColor)
{
    g_oledFgColor = fgColor;
    g_oledBgColor = bgColor;
}

void OLED_Init(void)
{
    OLED_BLK_Set();
    OLED_CS_Set();
    OLED_DC_Set();

    OLED_RES_Clr();
    delay_ms(50U);
    OLED_RES_Set();
    delay_ms(120U);

    ST7789_WriteCommand(0x11);
    delay_ms(120U);

    ST7789_WriteCommand(0x3A);
    ST7789_WriteData(0x55);

    ST7789_WriteCommand(0x36);
    ST7789_WriteData(0x00);

    ST7789_WriteCommand(0x2A);
    ST7789_WriteData(0x00);
    ST7789_WriteData(0x00);
    ST7789_WriteData(0x00);
    ST7789_WriteData(0xEF);

    ST7789_WriteCommand(0x2B);
    ST7789_WriteData(0x00);
    ST7789_WriteData(0x00);
    ST7789_WriteData(0x01);
    ST7789_WriteData(0x3F);

    ST7789_WriteCommand(0xB2);
    ST7789_WriteData(0x0C);
    ST7789_WriteData(0x0C);
    ST7789_WriteData(0x00);
    ST7789_WriteData(0x33);
    ST7789_WriteData(0x33);

    ST7789_WriteCommand(0xB7);
    ST7789_WriteData(0x35);

    ST7789_WriteCommand(0xBB);
    ST7789_WriteData(0x19);

    ST7789_WriteCommand(0xC0);
    ST7789_WriteData(0x2C);

    ST7789_WriteCommand(0xC2);
    ST7789_WriteData(0x01);

    ST7789_WriteCommand(0xC3);
    ST7789_WriteData(0x12);

    ST7789_WriteCommand(0xC4);
    ST7789_WriteData(0x20);

    ST7789_WriteCommand(0xC6);
    ST7789_WriteData(0x0F);

    ST7789_WriteCommand(0xD0);
    ST7789_WriteData(0xA4);
    ST7789_WriteData(0xA1);

    ST7789_WriteCommand(0xE0);
    ST7789_WriteData(0xD0);
    ST7789_WriteData(0x04);
    ST7789_WriteData(0x0D);
    ST7789_WriteData(0x11);
    ST7789_WriteData(0x13);
    ST7789_WriteData(0x2B);
    ST7789_WriteData(0x3F);
    ST7789_WriteData(0x54);
    ST7789_WriteData(0x4C);
    ST7789_WriteData(0x18);
    ST7789_WriteData(0x0D);
    ST7789_WriteData(0x0B);
    ST7789_WriteData(0x1F);
    ST7789_WriteData(0x23);

    ST7789_WriteCommand(0xE1);
    ST7789_WriteData(0xD0);
    ST7789_WriteData(0x04);
    ST7789_WriteData(0x0C);
    ST7789_WriteData(0x11);
    ST7789_WriteData(0x13);
    ST7789_WriteData(0x2C);
    ST7789_WriteData(0x3F);
    ST7789_WriteData(0x44);
    ST7789_WriteData(0x51);
    ST7789_WriteData(0x2F);
    ST7789_WriteData(0x1F);
    ST7789_WriteData(0x1F);
    ST7789_WriteData(0x20);
    ST7789_WriteData(0x23);

    g_displayInverted = true;
    ST7789_WriteCommand(0x21);
    g_displayRotated180 = false;
    ST7789_SetRotation(g_displayRotated180);
    ST7789_WriteCommand(0x29);

    OLED_Clear();
}
