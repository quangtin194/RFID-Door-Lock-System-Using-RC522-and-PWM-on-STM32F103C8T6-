#ifndef __OLED_H
#define __OLED_H
/* ---- Display geometry (SH1106 1.3" I2C module) ---- */
#define OLED_WIDTH          128   /* visible pixels                        */
#define OLED_HEIGHT         64
#define OLED_PAGES          (OLED_HEIGHT / 8)   /* 8 pages of 8px rows     */
#define OLED_COL_OFFSET     2     /* SH1106 RAM is 132 cols; 2 are hidden  */

/* 7-bit SH1106 I2C address is 0x3C on almost all 1.3" modules (a few use
 * 0x3D). HAL wants the address left-shifted by 1 (8-bit form used by
 * HAL_I2C_Mem_Write). */
#define OLED_I2C_ADDR       (0x3C << 1)

// INCLUDE & DEFINE
#include "stm32f1xx_hal.h"

// FUNCTION PROTOTYPES
void Oled_Init(I2C_HandleTypeDef *i2c);

typedef enum {
    OLED_MSG_SCANNING = 0,  /* IDLE state                    -> "Scanning!"                      */
    OLED_MSG_WELCOME,       /* ACCESS_ALLOWED                -> "Welcome"                         */
    OLED_MSG_DENIED,        /* ACCESS_DENIED                 -> "Denied"                           */
    OLED_MSG_ADMIN_MENU,    /* ADMIN_MODE                    -> "Hi Boss!" / "1:ADD 2:DELETE"      */
    OLED_MSG_CARD_EXISTS,   /* ADD_CARD, duplicate UID       -> "Card Exists"                      */
    OLED_MSG_CARD_ADDED,    /* ADD_CARD, success             -> "Card Added!"                      */
    OLED_MSG_CARD_DELETED,  /* DELETE_CARD, success          -> "Card Deleted!"                    */
    OLED_MSG_NOT_FOUND,
    OLED_MSG_ADMIN_CARD,
    OLED_MSG_SCAN_ADD_CARD,
    OLED_MSG_SCAN_DELETE_CARD,
    OLED_MSG_ERROR
} Oled_Msg_t;

void Oled_Init(I2C_HandleTypeDef *hi2c);

void Oled_Clear(void);

void Oled_ShowStatus(Oled_Msg_t msg); // thay Oled_Display()

#endif