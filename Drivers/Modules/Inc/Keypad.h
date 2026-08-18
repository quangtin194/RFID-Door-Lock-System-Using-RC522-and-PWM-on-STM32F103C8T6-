#ifndef __KEYPAD_H
#define __KEYPAD_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
// STRUCT DEFINITIONS
typedef struct {
    GPIO_TypeDef *Row;
    GPIO_TypeDef *Col;
    uint16_t Row_Pins[KEYPAD_ROWS];
    uint16_t Col_Pins[KEYPAD_COLS];
    } Keypad_t;

extern Keypad_t Keypad_Handle;

// FUNCTION PROTOTYPES
void Keypad_Init(Keypad_t *keypad);
uint8_t Keypad_Scan(void);

#endif

