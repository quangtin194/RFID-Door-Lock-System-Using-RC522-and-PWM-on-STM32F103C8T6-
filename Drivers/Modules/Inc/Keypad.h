#ifndef __KEYPAD_H
#define __KEYPAD_H

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stm32f103xb.h>
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
// ENUM DEFINITIONS
typedef enum {
    KEY_NONE, 
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_THANG,
    KEY_SAO
} Key_t;

// STRUCT DEFINITIONS
typedef struct {
    GPIO_TypeDef *Col1_Port;
    GPIO_TypeDef *Col2_Port;
    GPIO_TypeDef *Col3_Port;
    uint16_t Col1_Pin;
    uint16_t Col2_Pin;
    uint16_t Col3_Pin;
    IRQn_Type Col1_IRQn;
    IRQn_Type Col2_IRQn;
    IRQn_Type Col3_IRQn;

    GPIO_TypeDef *Row1_Port;
    GPIO_TypeDef *Row2_Port;
    GPIO_TypeDef *Row3_Port;
    GPIO_TypeDef *Row4_Port;
    uint16_t Row1_Pin;
    uint16_t Row2_Pin;
    uint16_t Row3_Pin;
    uint16_t Row4_Pin;
} Keypad_t;
    
extern Keypad_t Keypad_Handle;

// FUNCTION PROTOTYPES
void Keypad_Init(Keypad_t *keypad);
void Keypad_EnableEXTI(void);
void Keypad_DisableEXTI(void);

Key_t Keypad_Scan(void);               // Xac dinh nut duoc nhan
void Keypad_Password_Append(Key_t key);  
void Keypad_Password_Del(void);        // Xoa 1 ky tu
uint8_t Keypad_Password_GetLength(void);  
uint8_t Keypad_Password_Verify(void);   // Check password, tra ve 0 (sai) hoac 1 (dung)
void Keypad_Password_Reset(void);          

#endif
