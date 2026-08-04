// INCLUDE & DEFINE
#include "Servo.h"

// VARIABLE DEFINITIONS
static Servo_t *Servo_Handle;

// FUNCTION DEFINITIONS
void Servo_Init(Servo_t *servo) {
    Servo_Handle = servo;
    HAL_TIM_PWM_Start(Servo_Handle->htim, Servo_Handle->Channel);
}