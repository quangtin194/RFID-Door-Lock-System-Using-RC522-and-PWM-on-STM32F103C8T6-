#ifndef __BUTTON_H
#define __BUTTON_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
#include <stm32f103xb.h>

// STRUCT DEFINITIONS
typedef struct {
        IRQn_Type Add_Button;
        IRQn_Type Del_Button;
    } Button_t;

// FUNCTION PROTOTYPES
void Button_Init(Button_t button);
void Button_EnableEXTI(void);
void Button_DisableEXTI(void);

#endif

