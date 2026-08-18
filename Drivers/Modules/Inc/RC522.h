#ifndef __RC522_H
#define __RC522_H

// INCLUDE & DEFINE
#include "mfrc522.h"
#include "string.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_CARDS 4   // so the toi da luu duoc trong RAM

// ENUM 
typedef enum
{
    RC522_OK,          // RC522 bình thường và phát hiện có card
    RC522_NO_CARD,     // RC522 bình thường nhưng không có card
    RC522_ERROR        // RC522 lỗi (do phần cứng)
} RC522_Status_t;

typedef enum
{
    UID_ADMIN,
    UID_VALID,
    UID_INVALID,
    UID_NEW,
    UID_EXIST
} UID_Status_t;

// FUNCTION PROTOTYPES
void RC522_Init(SPI_HandleTypeDef *spi);
RC522_Status_t RC522_UID_Detected(void);
UID_Status_t RC522_UID_Verify(void);   // Hàm này sẽ trả về UID_ADMIN, UID_VALID hoặc UID_INVALID dựa trên UID được đọc từ thẻ RFID.
UID_Status_t RC522_UID_Delete(void); // Hàm này trả về UID_NEW, UID_ADMIN hoặc UID_EXIST
UID_Status_t RC522_UID_Add(void); // Hàm này trả về UID_NEW, UID_ADMIN hoặc UID_EXIST

uint8_t RC522_GetCardCount(void);                    // dem so the dang luu
uint8_t RC522_GetUID(uint8_t index, uint8_t *out);   // index 1-based, tra 1 neu slot co the
UID_Status_t RC522_DeleteByIndex(uint8_t index);     // index 1-based, xoa theo STT

#endif

