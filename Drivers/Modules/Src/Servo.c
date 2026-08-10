// INCLUDE & DEFINE
#include "Servo.h"
#define SERVO_PULSE_0_DEG 500
#define SERVO_PULSE_90_DEG 1500
// VARIABLE DEFINITIONS
static Servo_t *Servo_Handle = NULL;
// FUNCTION DEFINITIONS
void Servo_Init(Servo_t *servo) {
    if ( servo == NULL) return;
    Servo_Handle = servo; // gán vào biến toàn cục để dùng ở hàm Servo_SetAngle
    __HAL_TIM_SET_COMPARE(Servo_Handle->htim, Servo_Handle->Channel, SERVO_PULSE_0_DEG);
    HAL_TIM_PWM_Start(Servo_Handle->htim, Servo_Handle->Channel);
}
void Servo_SetAngle(uint8_t angle){
    if (Servo_Handle == NULL) return;
    if ( angle == 90){
        __HAL_TIM_SET_COMPARE(Servo_Handle->htim, Servo_Handle->Channel, SERVO_PULSE_90_DEG);
    }
    else if (angle == 0){
        __HAL_TIM_SET_COMPARE(Servo_Handle->htim, Servo_Handle->Channel, SERVO_PULSE_0_DEG);
    }
    else{
        return;
    }
}

