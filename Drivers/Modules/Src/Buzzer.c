// INCLUDE & DEFINE
#include "Buzzer.h"

// VARIABLE DEFINITIONS
static Buzzer_t Buzzer_Handle;

// FUNCTION DEFINITIONS
void Buzzer_Init(Buzzer_t *buzzer) {
    Buzzer_Handle = *buzzer;
}

void Buzzer_off(void) {
    HAL_GPIO_WritePin(Buzzer_Handle.Port, Buzzer_Handle.Pin, GPIO_PIN_RESET);
}

void Buzzer_on(void) {
    HAL_GPIO_WritePin(Buzzer_Handle.Port, Buzzer_Handle.Pin, GPIO_PIN_SET);
}