// INCLUDE & DEFINE
#include "Servo.h"

// VARIABLE DEFINITIONS
static Servo_t *Servo_Handle;

// FUNCTION DEFINITIONS
void Servo_Init(Servo_t *servo) {
    Servo_Handle = servo;
    HAL_TIM_PWM_Start(Servo_Handle->htim, Servo_Handle->Channel);
}

void Servo_SetAngle(uint8_t angle) {
    uint16_t ccr = 1000 + ((1000 * angle) / 180);
    __HAL_TIM_SET_COMPARE(Servo_Handle->htim, Servo_Handle->Channel, ccr);
}