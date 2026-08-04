// INCLUDE & DEFINE
#include "RC522.h"

// VARIABLE DEFINITIONS
static SPI_HandleTypeDef *RC522_Handle;

// FUNCTION DEFINITIONS
void RC522_Init(SPI_HandleTypeDef *spi) {
    RC522_Handle = spi;
}

RC522_Status_t RC522_UID_Detected(void) {

    return 0;  // Đại đại để compile
}

UID_Status_t RC522_UID_Verify(void) {

    return 0;   // Đại đại để compile
}

UID_Status_t RC522_UID_CheckAorD(void) {
    return 0;   // Đại đại để compile
}