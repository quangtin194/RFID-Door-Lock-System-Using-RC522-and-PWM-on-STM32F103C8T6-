// INCLUDE & DEFINE
#include "RC522.h"

#define PICC_REQIDL    0x26
#define MAX_CARDS 4

// VARIABLE DEFINITIONS
static SPI_HandleTypeDef *RC522_Handle;
uint8_t CurrentUID[5]; // Present UID
uint8_t AdminUID[4] = {0x53, 0x4F, 0x42, 0x28};  // Admin Card UID
uint8_t AuthorizedCards[MAX_CARDS][4] = {0}; // Array to store authorized user card UIDs

// FUNCTION DEFINITIONS
void RC522_Init(SPI_HandleTypeDef *spi) {
    RC522_Handle = spi;
    TM_MFRC522_Init();
}

UID_Status_t RC522_UID_Add(void) {
    if(memcmp(CurrentUID, AdminUID, 4) == 0) {
        return UID_ADMIN; 
    }

    for (int i = 0; i < MAX_CARDS; i++) {
        if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
            return UID_EXIST; 
        }
    }

    for (int i = 0; i < MAX_CARDS; i++) {
        if (AuthorizedCards[i][0] == 0x00) {
            memcpy(AuthorizedCards[i], CurrentUID, 4); 
            return UID_NEW; 
        }
    }
    return UID_EXIST; 
}

UID_Status_t RC522_UID_Delete(void) {
    if(memcmp(CurrentUID, AdminUID, 4) == 0) {
        return UID_ADMIN; // Admin card cannot be deleted
    }
    for (int i = 0; i < MAX_CARDS; i++) {
        if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
            memset(AuthorizedCards[i], 0x00, 4); 
            return UID_EXIST;
        }
    }
    return UID_NEW; // Card not found
}

RC522_Status_t RC522_UID_Detected(void) {
    uint8_t hardware_version = TM_MFRC522_ReadRegister(0x37); 
    
    // Nếu đọc ra 0x00 (mất GND/MISO) hoặc 0xFF (treo dây), báo lỗi ngay lập tức
    if (hardware_version == 0x00 || hardware_version == 0xFF) {
        return RC522_ERROR; 
    }
    uint8_t status;
    uint8_t respond[2];
    status = TM_MFRC522_Request(PICC_REQIDL, respond);
    if(status == MI_OK){
        status = TM_MFRC522_Anticoll(CurrentUID);
        if (status == MI_OK) 
            return RC522_OK; 
        else
            return RC522_ERROR;  
    }
    return RC522_NO_CARD;
}

UID_Status_t RC522_UID_Verify(void) {
    if (memcmp(CurrentUID, AdminUID, 4) == 0) {
        return UID_ADMIN;
    }
    for (int i = 0; i < MAX_CARDS; i++) {
        if (AuthorizedCards[i][0] != 0x00) { 
            if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
                return UID_VALID; 
            }
        }
    }
    return UID_INVALID;
}
<<<<<<< HEAD


// Nhung ham moi
UID_Status_t RC522_UID_ChangeAD(void) {
    
}
=======
>>>>>>> origin/main
