#ifndef __UART_H
#define __UART_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
#include <string.h>

// FUNCTION PROTOTYPES
void UART_Print_UID(void);
void UART_Init(UART_HandleTypeDef *uart);
void UART_PC_Print(const char* message);

#endif