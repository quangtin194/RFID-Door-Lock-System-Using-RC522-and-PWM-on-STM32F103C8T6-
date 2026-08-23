// INCLUDE & DEFINE
#include "Keypad.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>

// VARIABLE DEFINITIONS
Keypad_t Keypad_Handle;

#define PASSWORD_LENGTH 4
static uint8_t Password_Buffer[PASSWORD_LENGTH] = {0};
static uint8_t Password_Index = 0;
static const uint8_t Password_Store[PASSWORD_LENGTH] = {1, 2, 3, 4}; //pass:1234

// FUNCTION DEFINITIONS
void Keypad_Init(Keypad_t *keypad) {
    Keypad_Handle = *keypad;
    Keypad_DisableEXTI();
}

void Keypad_EnableEXTI(void) {
    __HAL_GPIO_EXTI_CLEAR_IT(Keypad_Handle.Col1_Pin | Keypad_Handle.Col2_Pin | Keypad_Handle.Col3_Pin);
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void Keypad_DisableEXTI(void) {
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
}

Key_t Keypad_Scan(void) {
    Key_t key =KEY_NONE;
    
    GPIO_TypeDef *rowPorts[KEYPAD_ROWS] ={
        Keypad_Handle.Row1_Port, Keypad_Handle.Row2_Port,Keypad_Handle.Row3_Port,Keypad_Handle.Row4_Port
    };
    GPIO_TypeDef *colPorts[KEYPAD_COLS] = {
        Keypad_Handle.Col1_Port, Keypad_Handle.Col2_Port,Keypad_Handle.Col3_Port
    };
    uint16_t rowPins[KEYPAD_ROWS] = {
        Keypad_Handle.Row1_Pin,Keypad_Handle.Row2_Pin,Keypad_Handle.Row3_Pin,Keypad_Handle.Row4_Pin
    };
    uint16_t colPins[KEYPAD_COLS] = {
        Keypad_Handle.Col1_Pin,Keypad_Handle.Col2_Pin,Keypad_Handle.Col3_Pin
    };
    Key_t keyMatrix[KEYPAD_ROWS][KEYPAD_COLS] = {
        {KEY_1,KEY_2,KEY_3},
        {KEY_4,KEY_5,KEY_6},
        {KEY_7,KEY_8,KEY_9},
        {KEY_THANG,KEY_0,KEY_SAO},
    };
    Keypad_DisableEXTI();  //tắt ngắt để phòng ngừa
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
            HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_SET);
        }
        HAL_GPIO_WritePin(rowPorts[r], rowPins[r], GPIO_PIN_RESET);
        HAL_Delay(1);

        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            if (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_RESET) {
                key = keyMatrix[r][c];
                break;
            }
        }
        if (key != KEY_NONE) break;
    }

    // Trả về low
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(rowPorts[i], rowPins[i], GPIO_PIN_RESET);
    }

    // Debounce
    uint32_t release_start = HAL_GetTick();
    while (HAL_GetTick() - release_start < 500) {
        uint8_t released = 1;
        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            if (HAL_GPIO_ReadPin(colPorts[c], colPins[c]) == GPIO_PIN_RESET) {
                released = 0;
                break;
            }
        }
        if (released) break;
        HAL_Delay(1);
    }

    Keypad_EnableEXTI();// bật ngắt lại 
    return key;
}
void Keypad_Password_Append(Key_t key) {
    if (key < KEY_0 || key > KEY_9) return; // Chi nhan phim so
    if (Password_Index >= PASSWORD_LENGTH) return; // Da day
    Password_Buffer[Password_Index++] = (uint8_t)(key - KEY_0);
}

void Keypad_Password_Del(void) {
    if (Password_Index > 0) {
        Password_Index--;
        Password_Buffer[Password_Index] = 0;
    }
}

uint8_t Keypad_Password_GetLength(void) {
    return Password_Index;
}

uint8_t Keypad_Password_Verify(void) {
    if (Password_Index != PASSWORD_LENGTH) return 0;
    for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
        if (Password_Buffer[i] != Password_Store[i]) return 0;
    }
    return 1;
}

void Keypad_Password_Reset(void) {
    Password_Index = 0;
    for (uint8_t i = 0; i < PASSWORD_LENGTH; i++) {
        Password_Buffer[i] = 0;
    }
}