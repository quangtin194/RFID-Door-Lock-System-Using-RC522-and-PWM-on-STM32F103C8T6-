#ifndef __BUZZER_H
#define __BUZZER_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
#include <stm32f103xb.h>

// STRUCT DEFINITIONS
typedef struct {
    GPIO_TypeDef *Port;
    uint16_t Pin;
} Buzzer_t;

// FUNCTION PROTOTYPES
void Buzzer_Init(Buzzer_t *buzzer);
void Buzzer_off(void);
void Buzzer_on(void);

#endif

