// INCLUDE & DEFINE
#include "UART.h"

// VARIABLE DEFINITIONS
static UART_HandleTypeDef *UART_Handle;

// FUNCTION DEFINITIONS
void UART_Init(UART_HandleTypeDef *uart) {
    UART_Handle = uart;
}

void UART_PC_Print(const char* message) {

}
