// INCLUDE & DEFINE
#include "UART.h"
#include "RC522.h"

// VARIABLE DEFINITIONS
static UART_HandleTypeDef *UART_Handle = NULL;
extern uint8_t CurrentUID[5];


// FUNCTION DEFINITIONS
void UART_Init(UART_HandleTypeDef *uart) {
    UART_Handle = uart;
}

void UART_PC_Print(const char* message) {
    if (UART_Handle != NULL && message != NULL) {
        HAL_UART_Transmit(UART_Handle, (uint8_t*)message, (uint16_t)strlen(message), 100);
    }
}

void UART_Print_UID(void) {
    if (UART_Handle == NULL) {
        return;
    }

    uint8_t status = TM_MFRC522_Anticoll(CurrentUID);

    if (status == MI_OK) { 
        char logStr[120];
        snprintf(logStr, sizeof(logStr), "\r\n %02X %02X %02X %02X\r\n", 
                 CurrentUID[0], CurrentUID[1], CurrentUID[2], CurrentUID[3]);
    
        UART_PC_Print(logStr);
    }
}

