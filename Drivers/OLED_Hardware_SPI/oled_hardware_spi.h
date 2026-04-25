/*
 * SysConfig Configuration Steps:
 *   SPI:
 *     1. Add a SPI module.
 *     2. Name it as "SPI_OLED".
 *     3. Set "Target Bit Rate" to "10000000". (optional)
 *     4. Set "Communication Direction" to "PICO only".
 *     5. Check the box "Enable Command Data Mode".
 *     6. Set the pins according to your needs.
 *   GPIO:
 *     1. Add a GPIO module.
 *     2. Name the group as "GPIO_OLED".
 *     3. Name the pin as "PIN_RES".
 *     4. Set the pin according to your needs.
 */

#ifndef __OLED_HARDWARE_SPI_H
#define __OLED_HARDWARE_SPI_H

#include "ti_msp_dl_config.h"

#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据

#define OLED_COLOR_BLACK   0x0000U
#define OLED_COLOR_WHITE   0xFFFFU
#define OLED_COLOR_BLUE    0x001FU
#define OLED_COLOR_RED     0xF800U
#define OLED_COLOR_GREEN   0x07E0U
#define OLED_COLOR_YELLOW  0xFFE0U
#define OLED_COLOR_CYAN    0x07FFU
#define OLED_COLOR_MAGENTA 0xF81FU

#ifndef GPIO_OLED_PIN_RES_PORT
#if defined(GPIO_OLED_PIN_OLED_RES_PIN)
#define GPIO_OLED_PIN_RES_PORT GPIO_OLED_PORT
#define GPIO_OLED_PIN_RES_PIN  GPIO_OLED_PIN_OLED_RES_PIN
#else
#define GPIO_OLED_PIN_RES_PORT GPIO_OLED_PORT 
#endif
#endif

#ifndef GPIO_OLED_PIN_DC_PORT
#if defined(GPIO_OLED_PIN_OLED_DC_PIN)
#define GPIO_OLED_PIN_DC_PORT GPIO_OLED_PORT
#define GPIO_OLED_PIN_DC_PIN  GPIO_OLED_PIN_OLED_DC_PIN
#else
#define GPIO_OLED_PIN_DC_PORT GPIO_OLED_PORT
#endif
#endif

#ifndef GPIO_OLED_PIN_CS_PORT
#if defined(GPIO_OLED_PIN_OLED_CS_PIN)
#define GPIO_OLED_PIN_CS_PORT GPIO_OLED_PORT
#define GPIO_OLED_PIN_CS_PIN  GPIO_OLED_PIN_OLED_CS_PIN
#else
#define GPIO_OLED_PIN_CS_PORT GPIO_OLED_PORT
#endif
#endif

#ifndef GPIO_OLED_PIN_BLK_PORT
#if defined(GPIO_OLED_PIN_OLED_BLK_PIN)
#define GPIO_OLED_PIN_BLK_PORT GPIO_OLED_PORT
#define GPIO_OLED_PIN_BLK_PIN  GPIO_OLED_PIN_OLED_BLK_PIN
#else
#define GPIO_OLED_PIN_BLK_PORT GPIO_OLED_PORT
#endif
#endif

//----------------------------------------------------------------------------------
//OLED SSD1306 复位/RES
#define		OLED_RES_Set()				(DL_GPIO_setPins(GPIO_OLED_PIN_RES_PORT, GPIO_OLED_PIN_RES_PIN))
#define		OLED_RES_Clr()			    (DL_GPIO_clearPins(GPIO_OLED_PIN_RES_PORT, GPIO_OLED_PIN_RES_PIN))

// OLED 命令/数据选择
#define     OLED_DC_Set()               (DL_GPIO_setPins(GPIO_OLED_PIN_DC_PORT, GPIO_OLED_PIN_DC_PIN))
#define     OLED_DC_Clr()               (DL_GPIO_clearPins(GPIO_OLED_PIN_DC_PORT, GPIO_OLED_PIN_DC_PIN))

// OLED 片选
#define     OLED_CS_Set()               (DL_GPIO_setPins(GPIO_OLED_PIN_CS_PORT, GPIO_OLED_PIN_CS_PIN))
#define     OLED_CS_Clr()               (DL_GPIO_clearPins(GPIO_OLED_PIN_CS_PORT, GPIO_OLED_PIN_CS_PIN))

// OLED 背光
#define     OLED_BLK_Set()              (DL_GPIO_setPins(GPIO_OLED_PIN_BLK_PORT, GPIO_OLED_PIN_BLK_PIN))
#define     OLED_BLK_Clr()              (DL_GPIO_clearPins(GPIO_OLED_PIN_BLK_PORT, GPIO_OLED_PIN_BLK_PIN))
				   

//OLED控制用函数
void delay_ms(uint32_t ms);
void OLED_ColorTurn(uint8_t i);
void OLED_DisplayTurn(uint8_t i);
void OLED_WR_Byte(uint8_t dat,uint8_t cmd);
void OLED_Set_Pos(uint8_t x, uint8_t y);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t sizey);
uint32_t oled_pow(uint8_t m,uint8_t n);
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t sizey);
void OLED_ShowString(uint8_t x,uint8_t y,uint8_t *chr,uint8_t sizey);
void OLED_ShowChinese(uint8_t x,uint8_t y,uint8_t no,uint8_t sizey);
void OLED_DrawBMP(uint8_t x,uint8_t y,uint8_t sizex, uint8_t sizey,uint8_t BMP[]);
void OLED_Init(void);
void OLED_SetTheme(uint16_t fgColor, uint16_t bgColor);

#endif /* #ifndef __OLED_HARDWARE_SPI_H */
