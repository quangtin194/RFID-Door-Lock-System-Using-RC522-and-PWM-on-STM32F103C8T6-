    // INCLUDE & DEFINE
    #include "RC522.h"

    #define PICC_REQIDL    0x26   //  Cmd yeu cau the phan hoi 
    #define FLASH_USER_START_ADDR   0x0800FC00
    #define FLASH_XOR_KEY 0x3C5A96F1

    // ---- Bao mat chong copy thong thuong (MIFARE Classic) ----
    // Ma bao mat 16 byte luu o sector 1 (block 4), bao ve bang key rieng.
    #define SEC_SECTOR        1
    #define SEC_BLOCK         (SEC_SECTOR * 4)      // block 4 (data dau cua sector)
    #define SEC_TRAILER       (SEC_SECTOR * 4 + 3)  // block 7 (Key A/B + access bits)
    #define SEC_KEY_SIZE      6

    static const uint8_t SecKeyA[SEC_KEY_SIZE] = {0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5};
    static const uint8_t SecKeyB[SEC_KEY_SIZE] = {0xF5, 0xE4, 0xD3, 0xC2, 0xB1, 0xA0};
    static const uint8_t SecCode[16] = {'F','4','B','K','-','M','E','S','S','I','S','I','U','U','!','!'};
    static const uint8_t DefaultKey[SEC_KEY_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    // Slot dang duoc chon trong danh sach xoa qua UART (0..MAX_CARDS-1 la Card,
    // MAX_CARDS..MAX_CARDS+MAX_ADMINS-1 la Admin). Khai bao extern trong RC522.h.
    uint8_t Selected_Slot = SLOT_NONE;

    // VARIABLE DEFINITIONS
    static SPI_HandleTypeDef *RC522_Handle;
    uint8_t CurrentUID[5];                             // Present UID
    uint8_t AdminUID[4] = {0x53, 0x4F, 0x42, 0x28};    // Admin Card UID
    uint8_t AuthorizedCards[MAX_CARDS][4] = {0};       // Array to store authorized user
    uint8_t CardCount = 0;
    uint8_t AdminUIDs[MAX_ADMINS][4]; 
    uint8_t AdminCount = 0;

    // FUNCTION DEFINITIONS
    void RC522_Init(SPI_HandleTypeDef *spi) {
        RC522_Handle = spi;
        TM_MFRC522_Init();
    }

    static uint8_t Is_Admin_Card(uint8_t *uid) {
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

    // Dua the ve trang thai ACTIVE lai (HALT -> WUPA -> Anticoll -> Select).
    // Dung de thu lai auth bang key khac khi lan auth truoc that bai.
    static TM_MFRC522_Status_t RC522_ReactivateCard(uint8_t *uid) {
        uint8_t respond[2];
        uint8_t serNum[5];
        TM_MFRC522_Status_t st;

        TM_MFRC522_Halt();                              // Bo qua ket qua (the co the da roi truong)
        st = TM_MFRC522_Request(PICC_REQALL, respond);  // WUPA 0x52 danh thuc the dang HALT
        if (st != MI_OK) return st;
        st = TM_MFRC522_Anticoll(serNum);
        if (st != MI_OK) return st;
        memcpy(uid, serNum, 4);                         // Cap nhat lai UID thuc te
        return TM_MFRC522_SelectTag(uid);
    }

    // Ghi ma bao mat va key rieng vao the (goi khi them the AC/AA).
    // Ho tro ca the moi (key mac dinh) va the da nap ma (key rieng).
    static TM_MFRC522_Status_t RC522_WriteSecCode(uint8_t *uid) {
        uint8_t trailer[16];
        TM_MFRC522_Status_t st;

        st = TM_MFRC522_SelectTag(uid);
        if (st != MI_OK) return st;

        // The da duoc nap ma truoc do -> key rieng
        st = TM_MFRC522_Auth(PICC_AUTHENT1A, SEC_BLOCK, (uint8_t *)SecKeyA, uid);
        if (st != MI_OK) {
            // The moi (chua nap) -> key mac dinh FF...
            // Lan auth sai vua roi lam roi Crypto1 cua the,
            // phai dua the ve ACTIVE lai truoc khi thu key mac dinh.
            if (RC522_ReactivateCard(uid) != MI_OK) {
                TM_MFRC522_Halt();
                return MI_ERR;
            }
            st = TM_MFRC522_Auth(PICC_AUTHENT1A, SEC_BLOCK, (uint8_t *)DefaultKey, uid);
            if (st != MI_OK) {
                TM_MFRC522_Halt();
                return st;
            }
        }

        st = TM_MFRC522_Write(SEC_BLOCK, (uint8_t *)SecCode);
        if (st != MI_OK) {
            TM_MFRC522_Halt();
            return st;
        }   

        // Ghi sector trailer: KeyA rieng + access bits (transport FF 07 80 69) + KeyB rieng
        memcpy(&trailer[0], SecKeyA, 6);
        trailer[6] = 0xFF;
        trailer[7] = 0x07;
        trailer[8] = 0x80;
        trailer[9] = 0x69;
        memcpy(&trailer[10], SecKeyB, 6);
        st = TM_MFRC522_Write(SEC_TRAILER, trailer);

        TM_MFRC522_Halt();
        return st;
    }

    // Doc ma bao mat tu the va so sanh. Tra ve MI_OK neu dung, MI_ERR neu sai.
    static TM_MFRC522_Status_t RC522_VerifySecCode(uint8_t *uid) {
        uint8_t buf[16];
        TM_MFRC522_Status_t st;

        st = TM_MFRC522_SelectTag(uid);
        if (st != MI_OK) return st;

        st = TM_MFRC522_Auth(PICC_AUTHENT1A, SEC_BLOCK, (uint8_t *)SecKeyA, uid);
        if (st != MI_OK) {
            TM_MFRC522_Halt();
            return st;
        }

        st = TM_MFRC522_Read(SEC_BLOCK, buf);
        TM_MFRC522_Halt();
        if (st != MI_OK) return st;

        if (memcmp(buf, SecCode, 16) != 0) {
            return MI_ERR;
        }
        return MI_OK;
    }

    void Flash_Save_Cards(void) {
        __disable_irq();            

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
            return;   
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
        __enable_irq();           

        uint32_t *check_ptr = (uint32_t *)FLASH_USER_START_ADDR;
        char dbg[100];
        sprintf(dbg, "[SAVE] counts_raw=0x%08lX  readback=0x%08lX %s\r\n",
                counts, check_ptr[0],
                (check_ptr[0] == counts) ? "OK" : "MISMATCH!!");
    }

    void Flash_Load_Cards(void) {
        memset(AuthorizedCards, 0x00, sizeof(AuthorizedCards));
        memset(AdminUIDs, 0x00, sizeof(AdminUIDs));
        
        uint32_t *flash_ptr = (uint32_t *)FLASH_USER_START_ADDR; 
        uint32_t counts = flash_ptr[0]; 
        
        if (counts == 0xFFFFFFFF) { 
            CardCount = 0; 
            AdminCount = 0;
            return;
        }

        counts ^= FLASH_XOR_KEY;
        
        CardCount = counts & 0xFF;            
        AdminCount = (counts >> 8) & 0xFF;

        if (CardCount > MAX_CARDS) CardCount = 0;
        if (AdminCount > MAX_ADMINS) AdminCount = 0;

        for (int i = 0; i < AdminCount; i++) {
            uint32_t admin_word = flash_ptr[i + 1];
            
            admin_word ^= FLASH_XOR_KEY; 
            
            AdminUIDs[i][0] = (admin_word >> 24) & 0xFF;
            AdminUIDs[i][1] = (admin_word >> 16) & 0xFF;
            AdminUIDs[i][2] = (admin_word >> 8)  & 0xFF;
            AdminUIDs[i][3] = admin_word & 0xFF;
        }

        for (int i = 0; i < CardCount; i++) {
            uint32_t uid_word = flash_ptr[i + 4];
            
            uid_word ^= FLASH_XOR_KEY; 
            
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

    // Kiem tra slot (4 byte UID) co dang trong hay khong.
    static uint8_t UID_IsEmpty(const uint8_t *uid) {
        return (uid[0] == 0x00 && uid[1] == 0x00 &&
                uid[2] == 0x00 && uid[3] == 0x00);
    }

    // Tinh lai CardCount = vi tri slot co du lieu cao nhat + 1.
    static void RecalcCardCount(void) {
        CardCount = 0;
        for (int i = 0; i < MAX_CARDS; i++) {
            if (!UID_IsEmpty(AuthorizedCards[i])) {
                CardCount = (uint8_t)(i + 1);
            }
        }
    }

    UID_Status_t RC522_UID_Add(void) {
        if(Is_Admin_Card(CurrentUID)) {
            return UID_ADMIN;
        }

        // Tim slot trong dau tien (khong don the sang trai).
        int8_t empty = -1;
        for (int i = 0; i < MAX_CARDS; i++) {
            if (UID_IsEmpty(AuthorizedCards[i])) {
                empty = i;
                break;
            }
        }
        if (empty < 0) {
            return UID_INVALID; // Danh sach da day
        }

        for (int i = 0; i < CardCount; i++) {
            if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
                return UID_EXIST; 
            }
        }

        // Ghi ma bao mat vao the truoc khi luu vao danh sach
        if (RC522_WriteSecCode(CurrentUID) != MI_OK) {
            return UID_INVALID; // the khong ghi duoc ma bao mat
        }

        memcpy(AuthorizedCards[empty], CurrentUID, 4);
        RecalcCardCount();
        Flash_Save_Cards();

        return UID_NEW;
    }

    UID_Status_t RC522_UID_Delete(void) {
        if(Is_Admin_Card(CurrentUID)) {
            return UID_ADMIN; // Admin card cannot be deleted
        }
        for (int i = 0; i < MAX_CARDS; i++) {
            if (!UID_IsEmpty(AuthorizedCards[i]) &&
                memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
                // Xoa tai dung slot, khong don cac the phia sau sang trai
                memset(AuthorizedCards[i], 0x00, 4);
                RecalcCardCount();
                Flash_Save_Cards();
                return UID_EXIST;
            }
        }
        return UID_NEW; // Card not found
    }

    RC522_Status_t RC522_UID_Detected(void) {
        uint8_t hardware_version = TM_MFRC522_ReadRegister(0x37); 
        
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
        uint8_t is_master = (memcmp(CurrentUID, AdminUID, 4) == 0);
        uint8_t is_admin = Is_Admin_Card(CurrentUID);
        uint8_t is_valid = 0;

        if (!is_admin) {
            for (int i = 0; i < MAX_CARDS; i++) {
                if (AuthorizedCards[i][0] != 0x00) {
                    if (memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
                        is_valid = 1;
                        break;
                    }
                }
            }
        }

        if (!is_admin && !is_valid) {
            return UID_INVALID;
        }

        // Kiem tra ma bao mat luu tren the.
        // The admin goc (master) duoc mien de tranh bi khoa ngoai;
        // cac the thuong va admin them vao deu phai co ma dung.
        if (!is_master) {
            if (RC522_VerifySecCode(CurrentUID) != MI_OK) {
                return UID_INVALID; // dung UID nhung sai/khong co ma -> khong hop le
            }
        }

        return is_admin ? UID_ADMIN : UID_VALID;
    }

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

        // Ghi ma bao mat vao the truoc khi luu 
        if (RC522_WriteSecCode(CurrentUID) != MI_OK) {
            return UID_INVALID; // the khong ghi duoc ma bao mat
        }

        for (int i = 0; i < MAX_CARDS; i++) {
            if (!UID_IsEmpty(AuthorizedCards[i]) &&
                memcmp(CurrentUID, AuthorizedCards[i], 4) == 0) {
                // Bo the khoi danh sach user tai dung slot (khong don the)
                memset(AuthorizedCards[i], 0x00, 4);
                RecalcCardCount();
                break;
            }
        }

        memcpy(AdminUIDs[AdminCount], CurrentUID, 4);
        AdminCount++;           
        
        Flash_Save_Cards(); 
        return UID_NEW; 
    }

    UID_Status_t RC522_UID_DeleteByIndex(uint8_t index) {
        if (index >= MAX_CARDS) {
            return UID_INVALID; // Vi tri khong hop le
        }

        // Slot dang trong san (chua tung co the / da bi xoa truoc do)
        if (AuthorizedCards[index][0] == 0x00 && AuthorizedCards[index][1] == 0x00 &&
            AuthorizedCards[index][2] == 0x00 && AuthorizedCards[index][3] == 0x00) {
            return UID_NEW;
        }

        memset(AuthorizedCards[index], 0x00, 4);  // Gan UID = 0x00000000

        // Tinh lai CardCount = chi so slot cao nhat con du lieu + 1.
        // Nho do viec them the moi (append o vi tri CardCount) van chay dung
        // ngay ca sau khi xoa slot cuoi cung cua danh sach.
        CardCount = 0;
        for (int i = 0; i < MAX_CARDS; i++) {
            if (AuthorizedCards[i][0] != 0x00 || AuthorizedCards[i][1] != 0x00 ||
                AuthorizedCards[i][2] != 0x00 || AuthorizedCards[i][3] != 0x00) {
                CardCount = (uint8_t)(i + 1);
            }
        }

        Flash_Save_Cards();                       // Luu lai vao flash

        return UID_EXIST; // Xoa thanh cong
    }

    UID_Status_t RC522_UID_DelAD(void) {
        if (memcmp(CurrentUID, AdminUID, 4) == 0) {
            return UID_ADMIN;
        }
        for (int i = 0; i < AdminCount; i++) {
            if (memcmp(CurrentUID, AdminUIDs[i], 4) == 0) {
                for (int j = i; j < AdminCount - 1; j++) {
                    memcpy(AdminUIDs[j], AdminUIDs[j + 1], 4);
                }
                memset(AdminUIDs[AdminCount - 1], 0x00, 4);
                AdminCount--;
                Flash_Save_Cards();
                return UID_EXIST;
            }
        }
        return UID_NEW;
    }

    UID_Status_t RC522_UID_DeleteAdminByIndex(uint8_t index) {
        if (index >= MAX_ADMINS) {
            return UID_INVALID; // Vi tri khong hop le
        }

        // Slot admin dang trong san (chua tung co / da bi xoa truoc do)
        if (AdminUIDs[index][0] == 0x00 && AdminUIDs[index][1] == 0x00 &&
            AdminUIDs[index][2] == 0x00 && AdminUIDs[index][3] == 0x00) {
            return UID_NEW;
        }

        memset(AdminUIDs[index], 0x00, 4); // Gan UID = 0x00000000

        // Tinh lai AdminCount tuong tu nhu CardCount
        AdminCount = 0;
        for (int i = 0; i < MAX_ADMINS; i++) {
            if (AdminUIDs[i][0] != 0x00 || AdminUIDs[i][1] != 0x00 ||
                AdminUIDs[i][2] != 0x00 || AdminUIDs[i][3] != 0x00) {
                AdminCount = (uint8_t)(i + 1);
            }
        }

        Flash_Save_Cards(); // Luu lai vao flash

        return UID_EXIST; // Xoa thanh cong
    }

    uint8_t RC522_GetCardCount(void) {
        return CardCount;
    }

    uint8_t RC522_GetAdminCount(void) {
        return AdminCount;
    }

    // Copy 4 byte UID cua slot "index" vao uid_out. Tra ve 1 neu index hop le,
    // 0 neu khong hop le (khong ghi vao uid_out trong truong hop nay).
    uint8_t RC522_GetCardUID(uint8_t index, uint8_t *uid_out) {
        if (index >= MAX_CARDS || uid_out == NULL) {
            return 0;
        }
        memcpy(uid_out, AuthorizedCards[index], 4);
        return 1;
    }

    uint8_t RC522_GetAdminUID(uint8_t index, uint8_t *uid_out) {
        if (index >= MAX_ADMINS || uid_out == NULL) {
            return 0;
        }
        memcpy(uid_out, AdminUIDs[index], 4);
        return 1;
    }


    // In toan bo danh sach UID hien co ra UART (OLED chi 2 dong nen khong hien het duoc)
    void Print_Card_List_UART(void) {
        char buf[64];
        uint8_t uid[4];
        UART_PC_Print("--- Card List (press number, then * to delete) ---\r\n");

        for (uint8_t i = 0; i < MAX_CARDS; i++) {
            RC522_GetCardUID(i, uid);
            if (uid[0] == 0x00 && uid[1] == 0x00 && uid[2] == 0x00 && uid[3] == 0x00) {
                sprintf(buf, "%d) Card: (empty)\r\n", i + 1);
            } else {
                sprintf(buf, "%d) Card: %02X %02X %02X %02X\r\n", i + 1,
                        uid[0], uid[1], uid[2], uid[3]);
            }
            UART_PC_Print(buf);
        }

        for (uint8_t i = 0; i < MAX_ADMINS; i++) {
            RC522_GetAdminUID(i, uid);
            if (uid[0] == 0x00 && uid[1] == 0x00 && uid[2] == 0x00 && uid[3] == 0x00) {
                sprintf(buf, "%d) Admin: (empty)\r\n", MAX_CARDS + i + 1);
            } else {
                sprintf(buf, "%d) Admin: %02X %02X %02X %02X\r\n", MAX_CARDS + i + 1,
                        uid[0], uid[1], uid[2], uid[3]);
            }
            UART_PC_Print(buf);
        }
    }

    // In UID cua slot vua duoc chon ra UART/Hercules va nhac nguoi dung bam *
    // de xac nhan. OLED KHONG doi noi dung o buoc nay (van giu "Del/in uart")
    // - toan bo chi tiet chon/xac nhan hien thi qua man hinh log UART.
    void Print_Selected_Slot_UART(void) {
        if (Selected_Slot == SLOT_NONE) return;

        uint8_t uid[4];
        char buf[64];
        const char *type;

        if (Selected_Slot < MAX_CARDS) {
            RC522_GetCardUID(Selected_Slot, uid);
            type = "Card";
        } else {
            RC522_GetAdminUID(Selected_Slot - MAX_CARDS, uid);
            type = "Admin";
        }

        if (uid[0] == 0x00 && uid[1] == 0x00 && uid[2] == 0x00 && uid[3] == 0x00) {
            sprintf(buf, "Selected %s %d: (empty) - press * to confirm\r\n", type, Selected_Slot + 1);
        } else {
            sprintf(buf, "Selected %s %d: %02X %02X %02X %02X - press * to confirm\r\n",
                    type, Selected_Slot + 1, uid[0], uid[1], uid[2], uid[3]);
        }
        UART_PC_Print(buf);
    }