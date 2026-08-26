#include "mfrc522.h"

extern SPI_HandleTypeDef hspi1;
void TM_MFRC522_Init(void) {

	TM_MFRC522_Reset();   // Reset the MFRC522

	TM_MFRC522_WriteRegister(MFRC522_REG_T_MODE, 0x8D);     // Set timmer and prescaler (2000 Hz)
	TM_MFRC522_WriteRegister(MFRC522_REG_T_PRESCALER, 0x3E);  
	TM_MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_L, 30);    // Set the timer reload value to 30 (for 15ms timeout)
	TM_MFRC522_WriteRegister(MFRC522_REG_T_RELOAD_H, 0);
	TM_MFRC522_WriteRegister(MFRC522_REG_RF_CFG, 0x70);   // Set the receiver gain to maximum
	TM_MFRC522_WriteRegister(MFRC522_REG_TX_AUTO, 0x40); // Set the transmitter to automatically adjust the modulation
	TM_MFRC522_WriteRegister(MFRC522_REG_MODE, 0x3D);   
	TM_MFRC522_AntennaOn();		//Open the antenna
}


TM_MFRC522_Status_t TM_MFRC522_Compare(uint8_t* CardID, uint8_t* CompareID) {
	uint8_t i;
	for (i = 0; i < 5; i++) 
		if (CardID[i] != CompareID[i]) 
			return MI_ERR;
	return MI_OK;
}


void TM_MFRC522_WriteRegister(uint8_t addr, uint8_t val) {
    uint8_t txData[2];
    txData[0] = (addr << 1) & 0x7E; // Adress byte and clear the MSB for write operation
    txData[1] = val;                // Data byte

    MFRC522_CS_LOW;
    
    HAL_SPI_Transmit(&hspi1, txData, 2, 100);
    
    MFRC522_CS_HIGH;
}
uint8_t TM_MFRC522_ReadRegister(uint8_t addr) {
    uint8_t txData[2];
    uint8_t rxData[2] = {0, 0};
    
    txData[0] = ((addr << 1) & 0x7E) | 0x80; // Adress byte and set the MSB for read operation
    txData[1] = MFRC522_DUMMY;            
    
    MFRC522_CS_LOW;
    
    HAL_SPI_TransmitReceive(&hspi1, txData, rxData, 2, 100);
    
    MFRC522_CS_HIGH;

    return rxData[1]; 
}

void TM_MFRC522_SetBitMask(uint8_t reg, uint8_t mask) {
	TM_MFRC522_WriteRegister(reg, TM_MFRC522_ReadRegister(reg) | mask);
}

void TM_MFRC522_ClearBitMask(uint8_t reg, uint8_t mask){
	TM_MFRC522_WriteRegister(reg, TM_MFRC522_ReadRegister(reg) & (~mask));
} 

void TM_MFRC522_AntennaOn(void) {
	uint8_t temp;

	temp = TM_MFRC522_ReadRegister(MFRC522_REG_TX_CONTROL);  // TX Control Register have bit 0 and 1 to control the antenna
	if (!(temp & 0x03)) {	// 00000011,  if both bit are 0, turn on the antenna
		TM_MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
	}
}

void TM_MFRC522_AntennaOff(void) {
	TM_MFRC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}

void TM_MFRC522_Reset(void) {
	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_RESETPHASE);
}

TM_MFRC522_Status_t TM_MFRC522_Request(uint8_t reqMode, uint8_t* TagType) {
	TM_MFRC522_Status_t status;  
	uint16_t backBits;			//The received data bits

	TM_MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x07);		

	TagType[0] = reqMode;
	status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

	if ((status != MI_OK) || (backBits != 0x10)) {     // the status is not OK or the number of bits received is not 16 (0x10)
		status = MI_ERR;
	}

	return status;
}

TM_MFRC522_Status_t TM_MFRC522_ToCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint16_t* backLen) {
	TM_MFRC522_Status_t status = MI_ERR;
	uint8_t irqEn = 0x00;
	uint8_t waitIRq = 0x00;
	uint8_t lastBits;
	uint8_t n;
	uint16_t i;

	switch (command) {
		case PCD_AUTHENT: {      // Use for read/write data into the card, don't be used in this project
			irqEn = 0x12;
			waitIRq = 0x10;
			break;
		}
		case PCD_TRANSCEIVE: {
			irqEn = 0x77;   // 01110111b, enable all interrupts 
			waitIRq = 0x30;  // 00110000b, wait for RxIRq and IdleIRq to end
			break;
		}
		default:
			break;
	}

	TM_MFRC522_WriteRegister(MFRC522_REG_COMM_IE_N, irqEn | 0x80); // Enable interrupt request
	TM_MFRC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80);  // Clear all interrupt request bits
	TM_MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80); // Clear the FIFO buffer 

	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, PCD_IDLE); 

	//Writing data to the FIFO
	for (i = 0; i < sendLen; i++) {   
		TM_MFRC522_WriteRegister(MFRC522_REG_FIFO_DATA, sendData[i]);    // Lấy mã lệnh gửi vào FIFO
	}

	//Execute the command
	TM_MFRC522_WriteRegister(MFRC522_REG_COMMAND, command);
	if (command == PCD_TRANSCEIVE) {    
		TM_MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);		//Turn on start send
	}   

	//Waiting to receive data to complete
	i = 2000;	
	do {
		//Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
		n = TM_MFRC522_ReadRegister(MFRC522_REG_COMM_IRQ);
		i--;
	} while ((i!=0) && !(n&0x01) && !(n&waitIRq)); // n&0x01 check the last bit of n, if it is 1, it means the timer has expired
	TM_MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);	  // Turn off start send

	if (i != 0)  {
		if (!(TM_MFRC522_ReadRegister(MFRC522_REG_ERROR) & 0x1B)) {  // 0x1B = 00011011b, include BufferOvfl, ParityErr, ProtocolErr, CollErr
			status = MI_OK;
			if (n & irqEn & 0x01) {    
				status = MI_NOTAGERR;			
			}

			if (command == PCD_TRANSCEIVE) {
				n = TM_MFRC522_ReadRegister(MFRC522_REG_FIFO_LEVEL); // read the number of bytes in the FIFO buffer
				lastBits = TM_MFRC522_ReadRegister(MFRC522_REG_CONTROL) & 0x07;   // take the last 3 bits of the Control Register, which indicate how many bits are in the last byte
				if (lastBits) {   
					*backLen = (n - 1) * 8 + lastBits;   
				} else {   
					*backLen = n * 8;   
				}

				if (n == 0) {   
					n = 1;    
				}
				if (n > MFRC522_MAX_LEN) {   // avoid overflow
					n = MFRC522_MAX_LEN;   
				}

				//Reading the received data in FIFO
				for (i = 0; i < n; i++) {   
					backData[i] = TM_MFRC522_ReadRegister(MFRC522_REG_FIFO_DATA);    
				}
			}
		} else {   
			status = MI_ERR;  
		}
	}

	return status;
}

TM_MFRC522_Status_t TM_MFRC522_Anticoll(uint8_t* serNum) {
	TM_MFRC522_Status_t status;
	uint8_t i;
	uint8_t serNumCheck = 0;
	uint16_t unLen;

	TM_MFRC522_WriteRegister(MFRC522_REG_BIT_FRAMING, 0x00);		

	serNum[0] = PICC_ANTICOLL;
	serNum[1] = 0x20;
	status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

	if (status == MI_OK) {
		//Check card serial number
		for (i = 0; i < 4; i++) {   
			serNumCheck ^= serNum[i];
		}
		if (serNumCheck != serNum[i]) {   
			status = MI_ERR;    
		}
	}
	return status;
} 

