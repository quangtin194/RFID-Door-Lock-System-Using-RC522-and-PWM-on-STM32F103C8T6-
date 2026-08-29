#ifndef __APP_H
#define __APP_H

// INCLUDE & DEFINE
#include "Oled.h"
#include "UART.h"
#include "RC522.h"
#include "Servo.h"
#include "Buzzer.h"
#include "Keypad.h"
#include "stdint.h"
#include "stdio.h"

#define CLOSE_ANGLE 0
#define OPEN_ANGLE 90
#define TIMEOUT_S_WAIT 2000    // Short wait
#define TIMEOUT_L_WAIT 5000    // Long wait
#define SPAM_TIME 10000
#define MAX_DENY 3 
#define LOCK 3000


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
    ERROR_STATE,

    ADD_MENU,          // Menu con sau khi bam phim 1 (Them the) o Admin Mode
    DELETE_MENU,       // Menu con sau khi bam phim 2 (Xoa the) o Admin Mode
    DELETE_UID_LIST,   // Chon vi tri trong danh sach UID va xoa bang phim *
    UID_SLOT_DELETED,  // Da xoa UID tai vi tri da chon (gan ve 0x00000000)
    UID_SLOT_EMPTY,    // Vi tri da chon dang trong san, khong co gi de xoa

    PASSWORD_INPUT,
    ADD_ADMIN_CARD,
    DEL_ADMIN_CARD,
    ADMIN_ADDED,
    ADMIN_DELETED,
    ADMIN_CHANGE_DENIED,
    LOCKED

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
