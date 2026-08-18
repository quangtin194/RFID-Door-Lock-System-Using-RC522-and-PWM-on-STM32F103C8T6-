// INCLUDE & DEFINE
#include "Keypad.h"
#include "stm32f1xx.h"
#include "stm32f1xx_hal_gpio.h"
#include <stdint.h>

#define KEYPAD_BOUNCE_MS 20   // thoi gian chong nhieu 

// VARIABLE DEFINITIONS
Keypad_t Keypad_Handle;

// Ma tran phim: hang x cot
//   R1: 1 2 3
//   R2: 4 5 6
//   R3: 7 8 9
//   R4: * 0 #
static const uint8_t Keypad_Map[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

// FUNCTION DEFINITIONS
void Keypad_Init(Keypad_t *keypad)
{
    Keypad_Handle = *keypad;
    // kéo hàng lên high
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++)
        HAL_GPIO_WritePin(Keypad_Handle.Row, Keypad_Handle.Row_Pins[r], GPIO_PIN_SET);
}

static uint8_t Keypad_ReadButton(void){
    uint8_t rows, cols;
    for (rows = 0; rows < KEYPAD_ROWS; rows++){
        // Dat tat ca hang o HIGH
        for (uint8_t rr = 0; rr < KEYPAD_ROWS; rr++){
            HAL_GPIO_WritePin(Keypad_Handle.Row, Keypad_Handle.Row_Pins[rr], GPIO_PIN_SET);
        }
        // Keo hang dang quet xuong LOW
        HAL_GPIO_WritePin(Keypad_Handle.Row, Keypad_Handle.Row_Pins[rows], GPIO_PIN_RESET);
        
        // Doc 3 cot
        for (cols = 0; cols < KEYPAD_COLS; cols++){
            if(HAL_GPIO_ReadPin(Keypad_Handle.Col, Keypad_Handle.Col_Pins[cols]) == GPIO_PIN_RESET){
                return Keypad_Map[rows][cols];
            }
        }
    }
    return 0;
}

// Goi lien tuc trong App_Run:
//  - Chong nhieu: phim phai on dinh >= KEYPAD_DEBOUNCE_MS moi tin
//  - Chong lap: chi tra ve 1 lan cho moi lan nhan phim
uint8_t Keypad_Scan(void)
{
    static uint8_t last_state = 0;      // phim on dinh o lan quet truoc
    static uint8_t last_reported = 0;   // phim da bao cho App
    static uint32_t last_change = 0;    // thoi diem trang thai doi lan cuoi

    uint8_t raw = Keypad_ReadButton();

    // Phim vua doi -> dat moc chong nhieu, chua bao gi
    if (raw != last_state)
    {
        last_state = raw;
        last_change = HAL_GetTick();
        return 0;
    }

    // Chua du thoi gian on dinh -> bo qua
    if ((HAL_GetTick() - last_change) < KEYPAD_BOUNCE_MS)
        return 0;

    // Trang thai on dinh
    if (raw == 0)
    {
        last_reported = 0;   // da nha phim -> cho phep lan nhan ke tiep
        return 0;
    }

    // Co phim moi duoc nhan -> bao 1 lan duy nhat
    if (raw != last_reported)
    {
        last_reported = raw;
        return raw;
    }

    return 0;   // dang giu phim -> khong bao lap
}