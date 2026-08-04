// INCLUDE & DEFINE
#include "App.h"

// VARIABLE DEFINITIONS
static volatile AppState_t appState;
static volatile AppState_t previous_State;
static uint32_t Timeout_counter;
static RC522_Status_t rc522Status;
static UID_Status_t uidStatus;

// XU LY NGAT EXTI
// ....

// FUNCTION DEFINITIONS
void App_Init(
    Button_t button,
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
    Button_Init(button);

    // Trang thai ban dau    
    appState = IDLE;
    previous_State = IDLE;
}

void App_Run(void) {
// State Entry
    if (appState != previous_State)
    {
        previous_State = appState;
        switch (appState) {
            case IDLE:
                Button_DisableEXTI();
                Servo_SetAngle(CLOSE_ANGLE);
                Buzzer_off();
                Oled_Display("Scanning!");

                break;
            case VERIFY_UID:

                break;
            case ADMIN_MODE:
                Button_EnableEXTI();
                Servo_SetAngle(OPEN_ANGLE);
                Oled_Display("Hi Boss!");
                Oled_Display("ADD CARD (1) or DELETE CARD (2)");
                UART_PC_Print("Admin mode\n");

                break;
            case ACCESS_ALLOWED:
                Servo_SetAngle(OPEN_ANGLE);
                Oled_Display("Welcome");
                UART_PC_Print("Welcome ID: ...\n");

                break;
            case ACCESS_DENIED:
                Oled_Display("Denied");
                UART_PC_Print("Denied ID: ...\n");
                
                break;
            case ADD_CARD:
                Oled_Display("Scan to add");

                break;
            case DELETE_CARD:
                Oled_Display("Scan to delete");

                break;
            case CARD_ADDED:
                Oled_Display("Card Added");
                UART_PC_Print("Save ID: ...\n");

                break;
            case CARD_EXISTS:
                Oled_Display("Card Exists");

                break;
            case CARD_DELETED:
                Oled_Display("Card Deleted");
                UART_PC_Print("Delete ID: ...\n");

                break;
            case DELETE_DENIED:
                if (uidStatus == UID_NEW) Oled_Display("Not Found");
                else if (uidStatus == UID_ADMIN) Oled_Display("Cannot Delete Admin");

                break;
            case ERROR_STATE:
                Button_DisableEXTI();
                Servo_SetAngle(CLOSE_ANGLE);
                Buzzer_on();
                UART_PC_Print("ERROR\n");
                Oled_Display("Error!!");
                
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
            // Ngắt sẽ thay đổi luồng chương trình, nhớ trong hàm xử lý ngắt có update Timeout_counter nha

            break;
        case ACCESS_ALLOWED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case ACCESS_DENIED:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_S_WAIT) appState = IDLE;

            break;
        case ADD_CARD:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
            else
            {
                uidStatus = RC522_UID_CheckAorD();
                if (uidStatus == UID_EXIST || uidStatus == UID_ADMIN) appState = CARD_EXISTS;
                else appState = CARD_ADDED;
                Timeout_counter = HAL_GetTick();

            }

            break;
        case DELETE_CARD:
            if (HAL_GetTick() - Timeout_counter > TIMEOUT_L_WAIT) appState = IDLE;
            else
            {
                uidStatus = RC522_UID_CheckAorD();
                if (uidStatus == UID_EXIST) appState = CARD_DELETED;
                else appState = DELETE_DENIED;
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
        case ERROR_STATE:
            if (RC522_UID_Detected() == RC522_OK) appState = IDLE;

            break;
        default:
            break;
    }
}


