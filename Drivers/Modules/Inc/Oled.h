#ifndef __OLED_H
#define __OLED_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
/* ---- Display geometry (SH1106 1.3" I2C module) ---- */
#define OLED_WIDTH          128   /* visible pixels                        */
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)   /* 8 pages of 8px rows     */
#define OLED_COL_OFFSET     2     /* SH1106 RAM is 132 cols; 2 are hidden  */

/* 7-bit SH1106 I2C address is 0x3C on almost all 1.3" modules (a few use
 * 0x3D). HAL wants the address left-shifted by 1 (8-bit form used by
 * HAL_I2C_Mem_Write). */
#define OLED_I2C_ADDR       (0x3C << 1)


// FUNCTION PROTOTYPES
void Oled_Init(I2C_HandleTypeDef *i2c);
void Oled_Clear(void);
void Oled_ShowLines(const char *line1, uint8_t scale1, const char *line2, uint8_t scale2);
void Oled_ShowPasswordMask(uint8_t length);   //  Xuat ra chuoi ****
void Oled_ShowLockCountdown(uint32_t seconds); // Hien "Secure!" + thoi gian dem nguoc (giay)

#endif
