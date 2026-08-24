#ifndef __APP_H
#define __APP_H

// INCLUDE & DEFINE
#include "Oled.h"
#include "UART.h"
#include "RC522.h"
#include "Servo.h"
#include "Buzzer.h"
<<<<<<< HEAD
#include "Keypad.h"
=======
#include "Button.h"
>>>>>>> origin/main
#include "stdint.h"

#define CLOSE_ANGLE 0
#define OPEN_ANGLE 90
#define TIMEOUT_S_WAIT 1000    // Short wait
#define TIMEOUT_L_WAIT 5000    // Long wait
<<<<<<< HEAD
#define SPAM_TIME 10000
#define MAX_DENY 3
#define LOCK 3000
=======
>>>>>>> origin/main


// ENUM DEFINITIONS
typedef enum {
    IDLE,
    VERIFY_UID,
    ADMIN_MODE,
    ACCESS_ALLOWED,
    ACCESS_DENIED,
    ADD_CARD,
    DELETE_CARD,
    CARD_ADDED,
    CARD_EXISTS,
    CARD_DELETED,
    DELETE_DENIED,
<<<<<<< HEAD
    ERROR_STATE,

    PASSWORD_INPUT,
    CHANGE_ADMIN_CARD,
    ADMIN_CHANGED,
    ADMIN_CHANGE_DENIED,
    LOCKED

} AppState_t;

// FUNCTION PROTOTYPES
void App_Init(
    Keypad_t *keypad,
=======
    ERROR_STATE

} AppState_t;


// FUNCTION PROTOTYPES
void App_Init(
    Button_t *button,
>>>>>>> origin/main
    Buzzer_t *buzzer,
    UART_HandleTypeDef *uart,
    I2C_HandleTypeDef *oled,
    SPI_HandleTypeDef *rc522,
    Servo_t *servo
);
void App_Run(void);

#endif 