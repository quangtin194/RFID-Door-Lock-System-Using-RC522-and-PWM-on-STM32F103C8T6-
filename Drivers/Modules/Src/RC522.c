// INCLUDE & DEFINE
#include "RC522.h"
#include "UART.h"      // để dùng huart1
#include <stdio.h>      // sprintf
#include <string.h>     // strlen

#define PICC_REQIDL    0x26
#define FLASH_USER_START_ADDR   0x0800FC00
#define MAX_CARDS 5
#define MAX_ADMINS 3
#define FLASH_XOR_KEY 0x3C5A96F1 // Khóa mã hóa (Bạn có thể đổi tùy ý)

// VARIABLE DEFINITIONS
static SPI_HandleTypeDef *RC522_Handle;
uint8_t CurrentUID[5]; // Present UID
uint8_t AdminUID[4] = {0x53, 0x4F, 0x42, 0x28};  // Admin Card UID
uint8_t AuthorizedCards[MAX_CARDS][4] = {0}; // Array to store authorized user
uint8_t CardCount = 0;
uint8_t AdminUIDs[MAX_ADMINS][4]; 
uint8_t AdminCount = 0;
//  card UIDs

// FUNCTION DEFINITIONS
void RC522_Init(SPI_HandleTypeDef *spi) {
    RC522_Handle = spi;
    TM_MFRC522_Init();
}
uint8_t Is_Admin_Card(uint8_t *uid) {
    if (memcmp(uid, AdminUID, 4) == 0) {
        return 1;
    }
    for (int i = 0; i < AdminCount; i++) {
        if (memcmp(uid, AdminUIDs[i], 4) == 0) {
            return 1;
        }
    }
    return 0; 
}


void Flash_Save_Cards(void) {
    __disable_irq();            // <-- QUAN TRỌNG: khóa toàn bộ ngắt trong lúc ghi Flash

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPERR);

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;
    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
    EraseInitStruct.NbPages     = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return;   // Erase lỗi -> không ghi tiếp, tránh ghi rác
    }

    uint32_t counts = ((AdminCount << 8) | CardCount) ^ FLASH_XOR_KEY;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_START_ADDR, counts);

    for (int i = 0; i < MAX_ADMINS; i++) {
        uint32_t admin_word = 0xFFFFFFFF;
        if (i < AdminCount) {
            admin_word = (AdminUIDs[i][0] << 24) | (AdminUIDs[i][1] << 16) |
                         (AdminUIDs[i][2] << 8)  | AdminUIDs[i][3];
            admin_word ^= FLASH_XOR_KEY;
        }
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_START_ADDR + 4 + (i * 4), admin_word);
    }

    for (int i = 0; i < CardCount; i++) {
        uint32_t uid_word = (AuthorizedCards[i][0] << 24) |
                            (AuthorizedCards[i][1] << 16) |
                            (AuthorizedCards[i][2] << 8)  |
                            (AuthorizedCards[i][3]);
        uid_word ^= FLASH_XOR_KEY;
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_USER_START_ADDR + 16 + (i * 4), uid_word);
    }

    HAL_FLASH_Lock();
    __enable_irq();              // <-- mở lại ngắt SAU khi ghi xong hoàn toàn

    // ---- VERIFY: đọc lại ngay và so sánh, in ra UART để bạn thấy save có đúng không ----
    uint32_t *check_ptr = (uint32_t *)FLASH_USER_START_ADDR;
    char dbg[100];
    sprintf(dbg, "[SAVE] counts_raw=0x%08lX  readback=0x%08lX %s\r\n",
            counts, check_ptr[0],
            (check_ptr[0] == counts) ? "OK" : "MISMATCH!!");
    // Nếu bạn có sẵn huart1 global, có thể transmit trực tiếp; nếu không, gọi Flash_Print_Cards_UART(&huart1) sau đó.
}

void Flash_Load_Cards(void) {
    memset(AuthorizedCards, 0x00, sizeof(AuthorizedCards));
    memset(AdminUIDs, 0x00, sizeof(AdminUIDs));
    
    uint32_t *flash_ptr = (uint32_t *)FLASH_USER_START_ADDR; 
    uint32_t counts = flash_ptr[0]; 
    
    // LƯU Ý QUAN TRỌNG: Phải kiểm tra 0xFFFFFFFF trước khi XOR!
    // Vì nếu Flash trống (0xFFFFFFFF), đem đi XOR sẽ làm mất dấu nhận diện vùng nhớ trống.
    if (counts == 0xFFFFFFFF) { 
        CardCount = 0; 
        AdminCount = 0;
        return;
    }
    
    // Giải mã counts bằng XOR
    counts ^= FLASH_XOR_KEY;
    
    CardCount = counts & 0xFF;            
    AdminCount = (counts >> 8) & 0xFF;

    if (CardCount > MAX_CARDS) CardCount = 0;
    if (AdminCount > MAX_ADMINS) AdminCount = 0;

    // Đọc và giải mã Admin UIDs
    for (int i = 0; i < AdminCount; i++) {
        uint32_t admin_word = flash_ptr[i + 1];
        
        admin_word ^= FLASH_XOR_KEY; // <--- Giải mã XOR lần 2 để lấy lại UID gốc
        
        AdminUIDs[i][0] = (admin_word >> 24) & 0xFF;
        AdminUIDs[i][1] = (admin_word >> 16) & 0xFF;
        AdminUIDs[i][2] = (admin_word >> 8)  & 0xFF;
        AdminUIDs[i][3] = admin_word & 0xFF;
    }

    // Đọc và giải mã Authorized Cards UIDs
    for (int i = 0; i < CardCount; i++) {
        uint32_t uid_word = flash_ptr[i + 4];
        
        uid_word ^= FLASH_XOR_KEY; // <--- Giải mã XOR lần 2 để lấy lại UID gốc
        
        AuthorizedCards[i][0] = (uid_word >> 24) & 0xFF;
        AuthorizedCards[i][1] = (uid_word >> 16) & 0xFF;
        AuthorizedCards[i][2] = (uid_word >> 8)  & 0xFF;
        AuthorizedCards[i][3] = uid_word & 0xFF;
    }
}
void Flash_Print_Cards_UART(void) {
    char buf[100];
    uint32_t *flash_ptr = (uint32_t *)FLASH_USER_START_ADDR;

    UART_PC_Print("=== FLASH DEBUG ===\r\n");

    sprintf(buf, "RAW word0=0x%08lX  AdminCount=%d CardCount=%d\r\n",
            flash_ptr[0], AdminCount, CardCount);
    UART_PC_Print(buf);

    for (int i = 0; i < AdminCount; i++) {
        sprintf(buf, "Admin[%d] RAW=0x%08lX -> %02X %02X %02X %02X\r\n", i,
                flash_ptr[i+1],
                AdminUIDs[i][0], AdminUIDs[i][1], AdminUIDs[i][2], AdminUIDs[i][3]);
        UART_PC_Print(buf);
    }
    for (int i = 0; i < CardCount; i++) {
        sprintf(buf, "Card[%d] RAW=0x%08lX -> %02X %02X %02X %02X\r\n", i,
                flash_ptr[i+4],
                AuthorizedCards[i][0], AuthorizedCards[i][1],
                AuthorizedCards[i][2], AuthorizedCards[i][3]);
        UART_PC_Print(buf);
    }
}
UID_Status_t RC522_UID_Add(void) {
    if (CardCount >= MAX_CARDS) return UID_INVALID;
    if(Is_Admin_Card(CurrentUID)) {
        return UID_ADMIN; 
    }

    for (int i = 0; i < CardCount; i++) {
        if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
            return UID_EXIST; 
        }
    }

    memcpy(AuthorizedCards[CardCount], CurrentUID, 4);
    CardCount++;
    Flash_Save_Cards();

    return UID_NEW;
}

UID_Status_t RC522_UID_Delete(void) {
    if(Is_Admin_Card(CurrentUID)) {
        return UID_ADMIN; // Admin card cannot be deleted
    }
    for (int i = 0; i < CardCount; i++) {
        if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
            for (int j = i; j < CardCount - 1; j++) {
                memcpy(AuthorizedCards[j], AuthorizedCards[j + 1], 4);
            }
            
            memset(AuthorizedCards[CardCount - 1], 0x00, 4); 
            CardCount--;          
            Flash_Save_Cards();   
            
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
    if (Is_Admin_Card(CurrentUID)) {
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

// Nhung ham moi
UID_Status_t RC522_UID_AddAD(void) {
    if (memcmp(CurrentUID, AdminUID, 4) == 0) {
        return UID_ADMIN; 
    }
    for (int i = 0; i < AdminCount; i++) {
        if (memcmp(CurrentUID, AdminUIDs[i], 4) == 0) {
            return UID_EXIST; 
        }
    }
    if (AdminCount >= MAX_ADMINS) {
        return UID_INVALID; 
    }
    for (int i = 0; i < CardCount; i++) {
        if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
            for (int j = i; j < CardCount - 1; j++) {
                memcpy(AuthorizedCards[j], AuthorizedCards[j + 1], 4);
            }
            memset(AuthorizedCards[CardCount - 1], 0x00, 4);
            CardCount--;
            
            break; 
        }
    }

    memcpy(AdminUIDs[AdminCount], CurrentUID, 4);
    AdminCount++;           
    
    Flash_Save_Cards(); 
    return UID_NEW; 
}

// new