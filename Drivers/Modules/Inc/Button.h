#ifndef __BUTTON_H
#define __BUTTON_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
#include <stm32f103xb.h>

// STRUCT DEFINITIONS
typedef struct {
        uint16_t Add_but;
        uint16_t Del_but;
        IRQn_Type Add_Button_IRQ;
        IRQn_Type Del_Button_IRQ;

    } Button_t;


extern Button_t Button_Handle;

// FUNCTION PROTOTYPES
void Button_Init(Button_t *button);
void Button_EnableEXTI(void);
void Button_DisableEXTI(void);

#endif

