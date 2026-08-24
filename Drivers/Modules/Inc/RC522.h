#ifndef __RC522_H
#define __RC522_H

// INCLUDE & DEFINE
#include "mfrc522.h"
#include "string.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
void Flash_Save_Cards(void);
void Flash_Load_Cards(void);
void Flash_Print_Cards_UART(void); 
RC522_Status_t RC522_UID_Detected(void);
UID_Status_t RC522_UID_Verify(void);   // Hàm này sẽ trả về UID_ADMIN, UID_VALID hoặc UID_INVALID dựa trên UID được đọc từ thẻ RFID.
UID_Status_t RC522_UID_Delete(void); // Hàm này trả về UID_NEW, UID_ADMIN hoặc UID_EXIST
UID_Status_t RC522_UID_Add(void); // Hàm này trả về UID_NEW, UID_ADMIN hoặc UID_EXIST

// Nhung ham moi
UID_Status_t RC522_UID_AddAD(void);   // Thuc hien thay doi/khong thay doi the Admin. Sau do tra ve UID_NEW hoac gia tri khac
#endif


