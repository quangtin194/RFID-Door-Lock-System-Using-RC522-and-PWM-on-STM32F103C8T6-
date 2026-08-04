// INCLUDE & DEFINE
#include "Oled.h"

// VARIABLE DEFINITIONS
static I2C_HandleTypeDef *Oled_Handle;

// FUNCTION DEFINITIONS
void Oled_Init(I2C_HandleTypeDef *i2c) {
    Oled_Handle = i2c;
}

void Oled_Display(const char* message) {

}
