// INCLUDE & DEFINE
#include "App.h"
#include "Keypad.h"
#include "Oled.h"
#include <stdbool.h>
// VARIABLE DEFINITIONS
static volatile bool confirmPending = false; // đang chờ xác nhận có thẻ hay không? 
static volatile AppState_t appState;
static volatile AppState_t previous_State;
static uint32_t Timeout_counter;
static RC522_Status_t rc522Status;
static UID_Status_t uidStatus;
static Oled_Msg_t oled_status;
static uint8_t selectedIndex;    // STT the dang chon trong che do xoa (1-4), 0 = chua chon

// Hien danh sach 4 the da luu len OLED
static void App_ShowCardList(void)
{
    uint8_t uids[4][4] = {0};
    for (uint8_t i = 0; i < 4; i++) {
        RC522_GetUID(i + 1, uids[i]);
    }
    Oled_ShowCardList(uids);
}

// FUNCTION DEFINITIONS
void App_Init(
    Keypad_t *keypad,
    Buzzer_t *buzzer,
    UART_HandleTypeDef *uart,
    I2C_HandleTypeDef *oled,
    SPI_HandleTypeDef *rc522,
    Servo_t *servo
)
{
    // Init Modules
    Oled_Init(oled);
    UART_Init(uart);
    RC522_Init(rc522);
    Servo_Init(servo);
    Buzzer_Init(buzzer);
    Keypad_Init(keypad);

    // Trang thai ban dau    
    appState = IDLE;
    previous_State = ERROR_STATE;
}

void App_Run(void) {
// State Entry
    if (appState != previous_State)
    {
        previous_State = appState;
        switch (appState) {
            case IDLE:
                Servo_SetAngle(CLOSE_ANGLE);
                Buzzer_off();
                oled_status = OLED_MSG_SCANNING;
                Oled_ShowStatus(oled_status);

                break;
            case VERIFY_UID:

                break;
            case ADMIN_MODE:
                Servo_SetAngle(OPEN_ANGLE);
                oled_status = OLED_MSG_ADMIN_MENU;
                Oled_ShowStatus(OLED_MSG_ADMIN_MENU);
                UART_PC_Print("Admin mode\n");

                break;
            case ACCESS_ALLOWED:
                Servo_SetAngle(OPEN_ANGLE);
                oled_status = OLED_MSG_WELCOME;
                Oled_ShowStatus(OLED_MSG_WELCOME);
                UART_PC_Print("Welcome ID: ");
                UART_Print_UID();

                break;
            case ACCESS_DENIED:
                oled_status = OLED_MSG_DENIED;
                Oled_ShowStatus(OLED_MSG_DENIED);
                Buzzer_on();
                UART_PC_Print("Denied ID: ");
                UART_Print_UID();
                break;
            case ADD_CARD:
                confirmPending = false;   // reset: tranh bi ket o trang thai cho xac nhan cu
                oled_status = OLED_MSG_SCAN_ADD_CARD;
                Oled_ShowStatus(OLED_MSG_SCAN_ADD_CARD);
                UART_PC_Print("Add card\n");
                break;
            case DELETE_MENU:
                confirmPending = false;
                oled_status = OLED_MSG_DELETE_MENU;
                Oled_ShowStatus(OLED_MSG_DELETE_MENU);
                UART_PC_Print("Delete: 1=Scan 2=Index #:back\n");

                break;
            case DELETE_BY_SCAN:
                confirmPending = false;
                selectedIndex = 0;
                oled_status = OLED_MSG_SCAN_DELETE_CARD;
                Oled_ShowStatus(OLED_MSG_SCAN_DELETE_CARD);
                UART_PC_Print("Scan card to delete\n");

                break;
            case DELETE_CARD:
                confirmPending = false;
                selectedIndex = 0;
                App_ShowCardList();
                UART_PC_Print("Delete card (1-4) #:cancel\n");

                break;
            case CARD_ADDED:
                confirmPending = false;
                oled_status = OLED_MSG_CARD_ADDED;
                Oled_ShowStatus(OLED_MSG_CARD_ADDED);
                UART_PC_Print("Save ID: ");
                UART_Print_UID();
                break;
            case CARD_EXISTS:
                confirmPending = false;
                oled_status = OLED_MSG_CARD_EXISTS;
                Oled_ShowStatus(OLED_MSG_CARD_EXISTS);
                UART_PC_Print("Card exists\n");

                break;
            case CARD_DELETED:
                confirmPending = false;
                oled_status = OLED_MSG_CARD_DELETED;
                Oled_ShowStatus(OLED_MSG_CARD_DELETED);
                if (selectedIndex != 0)     // xoa theo STT
                {
                    char idxStr[2] = { (char)('0' + selectedIndex), '\0' };
                    UART_PC_Print("Deleted card: ");
                    UART_PC_Print(idxStr);
                    UART_PC_Print("\n");
                }
                else                        // xoa bang quet the
                {
                    UART_PC_Print("Delete ID: ");
                    UART_Print_UID();
                }
                break;
            case DELETE_DENIED:
                confirmPending = false;
                if (uidStatus == UID_NEW) 
                {
                    oled_status = OLED_MSG_NOT_FOUND;
                    Oled_ShowStatus(OLED_MSG_NOT_FOUND);
                    UART_PC_Print("UID Not found\n");
                }
                else if (uidStatus == UID_ADMIN) 
                {
                    oled_status = OLED_MSG_ADMIN_CARD;
                    Oled_ShowStatus(OLED_MSG_ADMIN_CARD);
                    UART_PC_Print("Cannot delete Admin card\n");
                }
                break;
            case ERROR_STATE:
                Servo_SetAngle(CLOSE_ANGLE);
                Buzzer_on();
                UART_PC_Print("ERROR\n");
                oled_status = OLED_MSG_ERROR;
                Oled_ShowStatus(OLED_MSG_ERROR);
                break;
            default:
                break;
        }
    }
    //_____________________________________________
// State Execution

    switch (appState) {
        case IDLE:
            rc522Status = RC522_UID_Detected();
            if (rc522Status == RC522_OK) appState = VERIFY_UID;
            else if (rc522Status == RC522_ERROR) appState = ERROR_STATE;

            break;
        case VERIFY_UID:
            uidStatus = RC522_UID_Verify();
            if (uidStatus == UID_ADMIN) appState = ADMIN_MODE;
            else if (uidStatus == UID_VALID) appState = ACCESS_ALLOWED;
            else if (uidStatus == UID_INVALID) appState = ACCESS_DENIED;
        
            Timeout_counter = HAL_GetTick();
            break;
        case ADMIN_MODE:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
            else
            {
                // Keypad dieu khien (quet lien tuc moi vong lap, khong can ngat)
                uint8_t key = Keypad_Scan();
                if (key == '1')      { appState = ADD_CARD;     Timeout_counter = HAL_GetTick(); }
                else if (key == '2') { appState = DELETE_MENU;  Timeout_counter = HAL_GetTick(); }
                else if (key == '#') { appState = IDLE;         Timeout_counter = HAL_GetTick(); }
                // Luu y: phai gia han Timeout_counter khi bam phim,
                // khong thi ADMIN_MODE se het 5s va thoat ngay lap tuc.
            }

            break;
        case ACCESS_ALLOWED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;

            break;
        case ACCESS_DENIED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case ADD_CARD:
        {
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT)
            {
                
                appState = IDLE;
                break;
            }
            uint8_t key = Keypad_Scan();        // quet 1 lan moi vong lap
            if (key == '#') { appState = ADMIN_MODE; Timeout_counter = HAL_GetTick(); break; }  // '*' = huy
            if (!confirmPending) {
                if (RC522_UID_Detected() == RC522_OK)
                {
                confirmPending = true;
                Oled_ShowStatus(OLED_CONFIRM);
                Timeout_counter = HAL_GetTick();  // gia han de nguoi dung kip bam '#'
                }
            }
            else {
                if (key == '*'){
                    uidStatus = RC522_UID_Add();
                    if (uidStatus == UID_EXIST || uidStatus == UID_ADMIN) appState = CARD_EXISTS;
                    else appState = CARD_ADDED;
                    Timeout_counter = HAL_GetTick();
                }
            }
            break;
        }
        case DELETE_MENU:
        {
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT)
            {
                appState = IDLE;
                break;
            }
            uint8_t key = Keypad_Scan();        // quet 1 lan moi vong lap
            if (key == '1')      { appState = DELETE_BY_SCAN; Timeout_counter = HAL_GetTick(); }
            else if (key == '2') { appState = DELETE_CARD;     Timeout_counter = HAL_GetTick(); }
            else if (key == '#') { appState = ADMIN_MODE;      Timeout_counter = HAL_GetTick(); }
            break;
        }
        case DELETE_BY_SCAN:
        {
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT)
            {
                appState = IDLE;
                break;
            }
            uint8_t key = Keypad_Scan();        // quet 1 lan moi vong lap
            if (!confirmPending)
            {
                if (key == '#')
                {
                    appState = DELETE_MENU;     // quay ve menu chon cach xoa
                    Timeout_counter = HAL_GetTick();
                }
                else if (RC522_UID_Detected() == RC522_OK)
                {
                    confirmPending = true;
                    Oled_ShowStatus(OLED_CONFIRM);
                    Timeout_counter = HAL_GetTick(); // gia han
                }
            }
            else
            {
                if (key == '*')                 // xac nhan xoa the da quet
                {
                    uidStatus = RC522_UID_Delete();
                    if (uidStatus == UID_EXIST) appState = CARD_DELETED;
                    else appState = DELETE_DENIED;
                    Timeout_counter = HAL_GetTick();
                }
                else if (key == '#')            // bo xac nhan, quay ve trang scan
                {
                    confirmPending = false;
                    Oled_ShowStatus(OLED_MSG_SCAN_DELETE_CARD);
                    Timeout_counter = HAL_GetTick();
                }
            }
            break;
        }
        case DELETE_CARD:
        {
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT)
            {
                appState = IDLE;
                break;
            }
            uint8_t key = Keypad_Scan();        // quet 1 lan moi vong lap

            if (selectedIndex == 0)             // dang xem danh sach the
            {
                if (key >= '1' && key <= '4')
                {
                    selectedIndex = (uint8_t)(key - '0');  // chon the can xoa
                    Oled_ShowStatus(OLED_CONFIRM);
                    Timeout_counter = HAL_GetTick();       // gia han
                }
                else if (key == '#')
                {
                    appState = DELETE_MENU;                // quay ve menu chon cach xoa
                    Timeout_counter = HAL_GetTick();
                }
            }
            else                                // dang cho xac nhan xoa the selectedIndex
            {
                if (key == '*')                 // xac nhan xoa
                {
                    uidStatus = RC522_DeleteByIndex(selectedIndex);
                    if (uidStatus == UID_EXIST) appState = CARD_DELETED;
                    else appState = DELETE_DENIED;
                    Timeout_counter = HAL_GetTick();
                }
                else if (key == '#')            // bo xac nhan, quay ve danh sach
                {
                    selectedIndex = 0;
                    App_ShowCardList();
                    Timeout_counter = HAL_GetTick();
                }
            }
            break;
        }
        case CARD_ADDED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case CARD_EXISTS:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case CARD_DELETED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case DELETE_DENIED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case ERROR_STATE:
            if (RC522_UID_Detected() == RC522_OK) appState = IDLE;

            break;
        default:
            break;
    }
}


