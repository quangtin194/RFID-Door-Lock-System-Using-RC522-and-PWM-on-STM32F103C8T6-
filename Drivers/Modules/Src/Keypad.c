// INCLUDE & DEFINE
#include "Keypad.h"

// VARIABLE DEFINITIONS
Keypad_t Keypad_Handle;

// FUNCTION DEFINITIONS
void Keypad_Init(Keypad_t *keypad) {
    Keypad_Handle = *keypad;
    Keypad_DisableEXTI();
}

void Keypad_EnableEXTI(void) {
    __HAL_GPIO_EXTI_CLEAR_IT(Keypad_Handle.Col1_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(Keypad_Handle.Col2_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(Keypad_Handle.Col3_Pin);

    HAL_NVIC_ClearPendingIRQ(Keypad_Handle.Col1_IRQn);

    HAL_NVIC_EnableIRQ(Keypad_Handle.Col1_IRQn);
}

void Keypad_DisableEXTI(void) {
    HAL_NVIC_DisableIRQ(Keypad_Handle.Col1_IRQn);

}

Key_t Keypad_Scan(void) {

    return 0;
}

void Keypad_Password_Append(Key_t key) {

}

void Keypad_Password_Del(void) {

}

uint8_t Keypad_Password_GetLength(void) {

    return 0;
}

uint8_t Keypad_Password_Verify(void) {

    return 0;
}

void Keypad_Password_Reset(void) {
    
} 