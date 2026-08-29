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
	TM_MFRC522_WriteRegister(MFRC522_REG_MODE, 0x25);   // Tat CRC phan cung (TxCRCEn/RxCRCEn) - dung CRC mem tranh noi CRC 2 lan
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

/* ISO14443A CRC_A (poly 0x1021, init 0x6363, reflected 0x8408) */
uint16_t TM_MFRC522_CalcCRC(uint8_t *data, uint8_t len) {
	uint16_t crc = 0x6363;
	uint8_t i, bit;
	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (bit = 0; bit < 8; bit++) {
			if (crc & 0x0001) {
				crc = (crc >> 1) ^ 0x8408;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

/* Chon the (SELECT) bang UID da nhan tu Anticoll */
TM_MFRC522_Status_t TM_MFRC522_SelectTag(uint8_t *serNum) {
	uint8_t buf[9];
	uint8_t i;
	uint8_t bcc = 0;
	uint16_t unLen;
	uint16_t crc;

	buf[0] = PICC_SElECTTAG; /* 0x93 */
	buf[1] = 0x70;           /* 7 bytes: SEL + NVB + UID(4) + BCC */
	for (i = 0; i < 4; i++) {
		buf[2 + i] = serNum[i];
		bcc ^= serNum[i];
	}
	buf[6] = bcc;
	crc = TM_MFRC522_CalcCRC(buf, 7);
	buf[7] = crc & 0xFF;
	buf[8] = crc >> 8;

	return TM_MFRC522_ToCard(PCD_TRANSCEIVE, buf, 9, buf, &unLen);
}

/* Xac thuc sector (PCD_AUTHENT), khong dung CRC */
TM_MFRC522_Status_t TM_MFRC522_Auth(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid) {
	uint8_t sendData[12];
	uint8_t i;
	uint16_t backLen;
	TM_MFRC522_Status_t status;

	sendData[0] = authMode;
	sendData[1] = blockAddr;
	for (i = 0; i < 6; i++) {
		sendData[2 + i] = key[i];
	}
	for (i = 0; i < 4; i++) {
		sendData[8 + i] = uid[i];
	}

	status = TM_MFRC522_ToCard(PCD_AUTHENT, sendData, 12, sendData, &backLen);
	if (status != MI_OK) {
		return status;
	}

	// Auth chi thanh cong khi Crypto1 duoc bat (MFCrypto1On = Status2Reg bit 2).
	// Neu key sai, the gui NACK/khong phan hoi => MFRC522 tat Crypto1 => bit nay = 0.
	// (ToCard cho PCD_AUTHENT khong tra ve loi khi key sai nen phai kiem tra bit nay.)
	if (!(TM_MFRC522_ReadRegister(MFRC522_REG_STATUS2) & 0x04)) {
		return MI_ERR;
	}
	return MI_OK;
}

/* Doc 16 byte du lieu cua mot block */
TM_MFRC522_Status_t TM_MFRC522_Read(uint8_t blockAddr, uint8_t *recvData) {
	uint8_t buf[4];
	uint16_t unLen;
	uint16_t crc;

	buf[0] = PICC_READ; /* 0x30 */
	buf[1] = blockAddr;
	crc = TM_MFRC522_CalcCRC(buf, 2);
	buf[2] = crc & 0xFF;
	buf[3] = crc >> 8;

	return TM_MFRC522_ToCard(PCD_TRANSCEIVE, buf, 4, recvData, &unLen);
}

/* Ghi 16 byte du lieu vao mot block */
TM_MFRC522_Status_t TM_MFRC522_Write(uint8_t blockAddr, uint8_t *writeData) {
	uint8_t buf[18];
	uint8_t i;
	uint16_t unLen;
	uint16_t crc;
	TM_MFRC522_Status_t status;

	/* Gui lenh WRITE + dia chi block + CRC */
	buf[0] = PICC_WRITE; /* 0xA0 */
	buf[1] = blockAddr;
	crc = TM_MFRC522_CalcCRC(buf, 2);
	buf[2] = crc & 0xFF;
	buf[3] = crc >> 8;
	status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, buf, 4, buf, &unLen);
	if (status != MI_OK) {
		return status;
	}
	/* The phai tra ve ACK 4 bit = 0x0A */
	if (unLen != 4 || (buf[0] & 0x0F) != 0x0A) {
		return MI_ERR;
	}

	/* Gui 16 byte du lieu + CRC */
	for (i = 0; i < 16; i++) {
		buf[i] = writeData[i];
	}
	crc = TM_MFRC522_CalcCRC(writeData, 16);
	buf[16] = crc & 0xFF;
	buf[17] = crc >> 8;
	status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, buf, 18, buf, &unLen);
	if (status != MI_OK) {
		return status;
	}
	if (unLen != 4 || (buf[0] & 0x0F) != 0x0A) {
		return MI_ERR;
	}

	return MI_OK;
}

/* Dua the ve trang thai HALT */
TM_MFRC522_Status_t TM_MFRC522_Halt(void) {
	uint8_t buf[4];
	uint16_t unLen;
	uint16_t crc;

	buf[0] = PICC_HALT; /* 0x50 */
	buf[1] = 0x00;
	crc = TM_MFRC522_CalcCRC(buf, 2);
	buf[2] = crc & 0xFF;
	buf[3] = crc >> 8;

	return TM_MFRC522_ToCard(PCD_TRANSCEIVE, buf, 4, buf, &unLen);
} 

