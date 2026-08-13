// INCLUDE & DEFINE
#include "UART.h"

// VARIABLE DEFINITIONS
static UART_HandleTypeDef *UART_Handle ;

// FUNCTION DEFINITIONS
void UART_Init(UART_HandleTypeDef *uart) {
    UART_Handle = uart;
}

void UART_PC_Print(const char* message) {
    if (UART_Handle !=NULL){
    HAL_UART_Transmit(UART_Handle,(uint8_t*)message,strlen(message),100);
    }
}