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
    __HAL_GPIO_EXTI_CLEAR_IT(Button_Handle.Add_but);
    __HAL_GPIO_EXTI_CLEAR_IT(Button_Handle.Del_but);

    HAL_NVIC_ClearPendingIRQ(Button_Handle.Add_Button_IRQ);
    HAL_NVIC_ClearPendingIRQ(Button_Handle.Del_Button_IRQ);

    HAL_NVIC_EnableIRQ(Button_Handle.Add_Button_IRQ);
    HAL_NVIC_EnableIRQ(Button_Handle.Del_Button_IRQ);
}

void Button_DisableEXTI(void) {
    HAL_NVIC_DisableIRQ(Button_Handle.Add_Button_IRQ);
    HAL_NVIC_DisableIRQ(Button_Handle.Del_Button_IRQ);

    __HAL_GPIO_EXTI_CLEAR_IT(Button_Handle.Add_but);
    __HAL_GPIO_EXTI_CLEAR_IT(Button_Handle.Del_but);

    HAL_NVIC_ClearPendingIRQ(Button_Handle.Add_Button_IRQ);
    HAL_NVIC_ClearPendingIRQ(Button_Handle.Del_Button_IRQ);
}