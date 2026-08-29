#ifndef __RC522_H
#define __RC522_H

// INCLUDE & DEFINE
#include "mfrc522.h"
#include "string.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "UART.h"  

// So slot trong danh sach xoa qua UART: Card (1..MAX_CARDS) + Admin (sau do)
#define MAX_CARDS 5
#define MAX_ADMINS 3
#define SLOT_NONE 0xFF

// Slot dang duoc chon de xoa qua UART (dung chung giua RC522.c va App.c)
extern uint8_t Selected_Slot;

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
UID_Status_t RC522_UID_Delete(void);   // Hàm này trả về UID_NEW, UID_ADMIN hoặc UID_EXIST
UID_Status_t RC522_UID_Add(void);      // Hàm này trả về UID_NEW, UID_ADMIN hoặc UID_EXIST
UID_Status_t RC522_UID_AddAD(void);    
UID_Status_t RC522_UID_DelAD(void);

UID_Status_t RC522_UID_DeleteByIndex(uint8_t index);
uint8_t RC522_GetCardCount(void);
uint8_t RC522_GetCardUID(uint8_t index, uint8_t *uid_out);
uint8_t RC522_GetAdminCount(void);
uint8_t RC522_GetAdminUID(uint8_t index, uint8_t *uid_out);
UID_Status_t RC522_UID_DeleteAdminByIndex(uint8_t index);
void Print_Card_List_UART(void);
void Print_Selected_Slot_UART(void);
#endif


