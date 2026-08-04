#ifndef __OLED_H
#define __OLED_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"

// FUNCTION PROTOTYPES
void Oled_Init(I2C_HandleTypeDef *i2c);
void Oled_Display(const char* message);

#endif