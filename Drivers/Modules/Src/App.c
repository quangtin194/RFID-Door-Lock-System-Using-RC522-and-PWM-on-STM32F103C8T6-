// INCLUDE & DEFINE
#include "App.h"

// VARIABLE DEFINITIONS
static volatile AppState_t appState;
static volatile AppState_t previous_State;
static volatile Key_t key; 
static uint8_t Deny_counter = 0;
static uint32_t Deny_start_time = 0;
static uint32_t Timeout_counter;
static RC522_Status_t rc522Status;
static UID_Status_t uidStatus;
static Oled_Msg_t oled_status;
static volatile uint8_t keypad_event;

static uint8_t Lock_Level = 0;        // So lan bi khoa lien tiep
static uint32_t Lock_Duration = 0;    // Thoi gian khoa hien tai (ms)
static uint32_t Last_Lock_Second = 0; // Giay cuoi cung da hien thi (tranh ve OLED lien tuc)

// XU LY NGAT EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{ // Đổi sau 
    if (GPIO_Pin == Keypad_Handle.Col1_Pin||
        GPIO_Pin == Keypad_Handle.Col2_Pin ||
        GPIO_Pin == Keypad_Handle.Col3_Pin)
    {
        keypad_event = 1;
    }
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

    // Trang thai System ban dau    
    appState = IDLE;
    previous_State = ERROR_STATE;

    // Trang thai Keypad ban dau
    key = KEY_NONE;
}

void App_Run(void) {
// State Entry
    if (appState != previous_State)
    {
        previous_State = appState;
        switch (appState) {
            case IDLE:
                Keypad_EnableEXTI();
                Servo_SetAngle(CLOSE_ANGLE);
                Buzzer_off();
                oled_status = OLED_MSG_SCANNING;
                Oled_ShowStatus(oled_status);
                break;

            case VERIFY_UID:
                Keypad_DisableEXTI();
                break;

            case PASSWORD_INPUT:
                Keypad_EnableEXTI();    
                oled_status = OLED_MSG_PASSWORD_INPUT;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Enter password\n");
                break;

            case ADMIN_MODE:
                Keypad_EnableEXTI(); 
                Servo_SetAngle(OPEN_ANGLE);
                oled_status = OLED_MSG_ADMIN_MENU;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Admin mode\n");
                break;

            case ACCESS_ALLOWED:
                Keypad_DisableEXTI();
                Servo_SetAngle(OPEN_ANGLE);
                oled_status = OLED_MSG_WELCOME;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Welcome ID: ");
                UART_Print_UID();
                break;

            case ACCESS_DENIED:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_DENIED;
                Oled_ShowStatus(oled_status);
                Buzzer_on();
                UART_PC_Print("Denied ID: ");
                UART_Print_UID();
                break;

            case LOCKED:
                Keypad_DisableEXTI();
                Servo_SetAngle(CLOSE_ANGLE);
                Buzzer_on();

                // Từng level khóa: lv1 khoa 3s, lv2 khoa 6s, lv3 khoa 12s, lv4 khoa 24s
                Lock_Level++;
                if (Lock_Level > 4) Lock_Level = 4;
                Lock_Duration = LOCK * (1UL << (Lock_Level - 1));
                Timeout_counter = HAL_GetTick();
                Last_Lock_Second = 0;

                Oled_ShowLockCountdown(Lock_Duration / 1000); // Hiển thị CountDown trên OLED
                UART_PC_Print("System locked\n");
                break;

            case ADD_CARD:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_SCAN_ADD_CARD;
                Oled_ShowStatus(OLED_MSG_SCAN_ADD_CARD);
                UART_PC_Print("Add card\n");
                break;

            case DELETE_CARD:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_SCAN_DELETE_CARD;
                Oled_ShowStatus(OLED_MSG_SCAN_DELETE_CARD);
                UART_PC_Print("Delete card\n");
                break;

            case CHANGE_ADMIN_CARD:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_SCAN_NEW_ADMIN;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Scan new admin card\n");
                break;

            case CARD_ADDED:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_CARD_ADDED;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Save ID: ");
                UART_Print_UID();
                break;

            case CARD_EXISTS:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_CARD_EXISTS;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Card exists\n");
                break;

            case CARD_DELETED:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_CARD_DELETED;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Delete ID: ");
                UART_Print_UID();
                break;

            case DELETE_DENIED:
                Keypad_DisableEXTI();
                if (uidStatus == UID_NEW) 
                {
                    oled_status = OLED_MSG_NOT_FOUND;
                    Oled_ShowStatus(oled_status);
                    UART_PC_Print("UID Not found\n");
                }
                else if (uidStatus == UID_ADMIN) 
                {
                    oled_status = OLED_MSG_ADMIN_CARD;
                    Oled_ShowStatus(oled_status);
                    UART_PC_Print("Cannot delete Admin card\n");
                }
                break;

            case ADMIN_CHANGED:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_ADMIN_CHANGED;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Admin card changed\n");
                break;

            case ADMIN_CHANGE_DENIED:
                Keypad_DisableEXTI();
                oled_status = OLED_MSG_ADMIN_CHANGE_DENIED;
                Oled_ShowStatus(oled_status);
                UART_PC_Print("Cannot change admin card\n");
                break;

            case ERROR_STATE:
                Keypad_DisableEXTI();
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
    if (keypad_event)
    {
        keypad_event = 0;
        key = Keypad_Scan();
    }

    switch (appState) {
        case IDLE:
            rc522Status = RC522_UID_Detected();
            if (rc522Status == RC522_OK) appState = VERIFY_UID;
            else if (rc522Status == RC522_ERROR) appState = ERROR_STATE;
            else if (key == KEY_THANG) appState = PASSWORD_INPUT;
            break;

        case VERIFY_UID:
            uidStatus = RC522_UID_Verify();
            if (uidStatus == UID_ADMIN) 
            {
                Deny_counter = 0;
                Lock_Level = 0;
                appState = ADMIN_MODE;
            }
            else if (uidStatus == UID_VALID) 
            {
                Deny_counter = 0;
                Lock_Level = 0;
                appState = ACCESS_ALLOWED;
            }
            else if (uidStatus == UID_INVALID)
            {
                if (Deny_counter == 0) Deny_start_time = HAL_GetTick();
                if (HAL_GetTick() - Deny_start_time > SPAM_TIME) Deny_counter = 0;

                Deny_counter++;

                if (Deny_counter >= MAX_DENY) 
                {
                    Deny_counter = 0;
                    appState = LOCKED;
                }
                else appState = ACCESS_DENIED;
            }
            Timeout_counter = HAL_GetTick();
            break;

        case PASSWORD_INPUT:
            if (key >= KEY_0 && key <= KEY_9)
            {
                Keypad_Password_Append(key);
                Oled_ShowPasswordMask(Keypad_Password_GetLength());
            }
            else if (key == KEY_SAO) 
            {
                Keypad_Password_Del();
                Oled_ShowPasswordMask(Keypad_Password_GetLength());
            }
            else if (key == KEY_THANG)
            {
                if (Keypad_Password_Verify())
                {
                    Lock_Level = 0;
                    appState = ACCESS_ALLOWED;
                }
                else
                {
                    appState = ACCESS_DENIED;
                }

                Keypad_Password_Reset();
                key = KEY_NONE;   // Reset key
                Timeout_counter = HAL_GetTick();
            }
            break;

        case ADMIN_MODE:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
                if (key == KEY_1)
                {
                    appState = ADD_CARD;
                    Timeout_counter = HAL_GetTick();
                }
                else if (key == KEY_2)
                {
                    appState = DELETE_CARD;
                    Timeout_counter = HAL_GetTick();
                }
                else if (key == KEY_3)
                {
                    appState = CHANGE_ADMIN_CARD;
                    Timeout_counter = HAL_GetTick();
                }
            break;

        case ACCESS_ALLOWED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;
            break;

        case ACCESS_DENIED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;
            break;

        case LOCKED:
        {
            uint32_t elapsed = HAL_GetTick() - Timeout_counter;
            if (elapsed >= Lock_Duration) {
                appState = IDLE;
            }
            else {
                // Cap nhat OLED moi giay mot lan
                uint32_t remaining_sec = (Lock_Duration - elapsed + 999) / 1000;
                if (remaining_sec != Last_Lock_Second) {
                    Last_Lock_Second = remaining_sec;
                    Oled_ShowLockCountdown(remaining_sec);
                }
            }
            break;
        }

        case ADD_CARD:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
            else
            {
                rc522Status = RC522_UID_Detected();
                if (rc522Status == RC522_OK)
                {
                    uidStatus = RC522_UID_Add();
                    if (uidStatus == UID_EXIST || uidStatus == UID_ADMIN) appState = CARD_EXISTS;
                    else appState = CARD_ADDED;
                }
                else if (rc522Status == RC522_ERROR) appState = ERROR_STATE;
            }
            break;

        case DELETE_CARD:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
            else
            {
                rc522Status = RC522_UID_Detected();
                if (rc522Status == RC522_OK)
                {
                    uidStatus = RC522_UID_Delete();
                    if (uidStatus == UID_EXIST) appState = CARD_DELETED;
                    else appState = DELETE_DENIED;
                }
                else if (rc522Status == RC522_ERROR) appState = ERROR_STATE;
            }
            break;

        case CHANGE_ADMIN_CARD:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
            else
            {
                rc522Status = RC522_UID_Detected();
                if (rc522Status == RC522_OK) 
                {
                    uidStatus = RC522_UID_AddAD();
                    if (uidStatus == UID_NEW) appState = ADMIN_CHANGED;
                    else appState = ADMIN_CHANGE_DENIED; 
                }
                else if (rc522Status == RC522_ERROR) appState = ERROR_STATE;
                Timeout_counter = HAL_GetTick();
            }
            break;

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

        case ADMIN_CHANGED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;
            break;

        case ADMIN_CHANGE_DENIED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;
            break;

        case ERROR_STATE:
            if (RC522_UID_Detected() != RC522_ERROR) appState = IDLE;
            break;

        default:
            break;
    }

    // Da xu ly phim xong -> reset de khong xu ly lap lai trong vong lap tiep theo
    key = KEY_NONE;
}