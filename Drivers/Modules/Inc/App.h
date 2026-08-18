#ifndef __APP_H
#define __APP_H

// INCLUDE & DEFINE
#include "Oled.h"
#include "UART.h"
#include "RC522.h"
#include "Servo.h"
#include "Buzzer.h"
//#include "Button.h"
#include "stdint.h"
#include "Keypad.h"

#define CLOSE_ANGLE 0
#define OPEN_ANGLE 90
#define TIMEOUT_S_WAIT 1000    // Short wait
#define TIMEOUT_L_WAIT 5000    // Long wait


// ENUM DEFINITIONS
typedef enum {
    IDLE,
    VERIFY_UID,
    ADMIN_MODE,
    ACCESS_ALLOWED,
    ACCESS_DENIED,
    ADD_CARD,
    DELETE_CARD,
    DELETE_MENU,
    DELETE_BY_SCAN,
    CARD_ADDED,
    CARD_EXISTS,
    CARD_DELETED,
    DELETE_DENIED,
    ERROR_STATE
} AppState_t;


// FUNCTION PROTOTYPES
void App_Init(
    Keypad_t *keypad,
    Buzzer_t *buzzer,
    UART_HandleTypeDef *uart,
    I2C_HandleTypeDef *oled,
    SPI_HandleTypeDef *rc522,
    Servo_t *servo
);
void App_Run(void);

#endif 