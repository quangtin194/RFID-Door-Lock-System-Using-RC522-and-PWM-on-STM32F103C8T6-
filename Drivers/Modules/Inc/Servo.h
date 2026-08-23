#ifndef __SERVO_H
#define __SERVO_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
#include <stm32f103xb.h>
#include "stdint.h"

// STRUCT DEFINITIONS
typedef struct {
    TIM_HandleTypeDef *htim;
    uint32_t Channel;
} Servo_t;

// FUNCTION PROTOTYPES
void Servo_Init(Servo_t *servo);
void Servo_SetAngle(uint8_t angle);

#endif

