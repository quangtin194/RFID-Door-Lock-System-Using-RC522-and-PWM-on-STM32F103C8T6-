// INCLUDE & DEFINE
#include "Button.h"

// VARIABLE DEFINITIONS
 Button_t Button_Handle;

// FUNCTION DEFINITIONS
void Button_Init(Button_t *button) {
    Button_Handle = *button;
    Button_DisableEXTI();
}

void Button_EnableEXTI(void) {
    HAL_NVIC_EnableIRQ(Button_Handle.Add_Button);
    HAL_NVIC_EnableIRQ(Button_Handle.Del_Button);
}

void Button_DisableEXTI(void) {
    HAL_NVIC_DisableIRQ(Button_Handle.Add_Button);
    HAL_NVIC_DisableIRQ(Button_Handle.Del_Button);
}