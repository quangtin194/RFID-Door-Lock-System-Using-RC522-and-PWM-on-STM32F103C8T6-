// INCLUDE & DEFINE
#include "Oled.h"
#include "string.h"
#define OLED_CMD_MODE   0x00   /* control byte: next byte(s) = command */
#define OLED_DATA_MODE  0x40   /* control byte: next byte(s) = data    */
#define OLED_FONT_COUNT   (sizeof(Oled_Font) / sizeof(Oled_Font[0]))
#define GLYPH_W       5   /* glyph width in source pixels           */
#define GLYPH_GAP     1   /* 1px gap between characters, unscaled   */
 
// VARIABLE DEFINITIONS
static I2C_HandleTypeDef *Oled_Handle;

// FUNCTION DEFINITIONS
static uint8_t Oled_Buffer[OLED_WIDTH * OLED_PAGES]; // nhân bản dữ liệu gửi qua OLED 
static void Oled_WriteCommand(uint8_t cmd)
{
    HAL_I2C_Mem_Write(Oled_Handle, OLED_I2C_ADDR, OLED_CMD_MODE,
                       I2C_MEMADD_SIZE_8BIT, &cmd, 1,HAL_MAX_DELAY);
} //lệnh điều chỉnh cưởng độ sáng .....
static void Oled_WriteData(const uint8_t *data, uint16_t size)
{
    HAL_I2C_Mem_Write(Oled_Handle, OLED_I2C_ADDR, OLED_DATA_MODE,
                       I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, size,HAL_MAX_DELAY);
} //lệnh bật tắt pixel, ghi thẳng vào RAM
static void Oled_UpdateScreen(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; page++) {
        Oled_WriteCommand(0xB0 + page);                            /* set page address    */
        Oled_WriteCommand(0x00 + (OLED_COL_OFFSET & 0x0F));         /* lower column nibble */
        Oled_WriteCommand(0x10 + (OLED_COL_OFFSET >> 4));           /* higher column nibble*/
        Oled_WriteData(&Oled_Buffer[page * OLED_WIDTH], OLED_WIDTH);
    }
}
void Oled_Init(I2C_HandleTypeDef *i2c) {
    Oled_Handle = i2c;
    /* Standard SH1106 1.3" power-on sequence. */
    Oled_WriteCommand(0xAE); /* display off                      */
    Oled_WriteCommand(0x40); /* start line address = 0           */
    Oled_WriteCommand(0xB0); /* page start address               */
    Oled_WriteCommand(0xC8); /* COM output scan direction, remap */
    Oled_WriteCommand(0x00 + (OLED_COL_OFFSET & 0x0F)); /* low column addr  */
    Oled_WriteCommand(0x10 + (OLED_COL_OFFSET >> 4));   /* high column addr */
    Oled_WriteCommand(0x81); /* contrast control                 */
    Oled_WriteCommand(0x80);
    Oled_WriteCommand(0xA1); /* segment re-map                   */
    Oled_WriteCommand(0xA6); /* normal (not inverted) display    */
    Oled_WriteCommand(0xA8); /* multiplex ratio                  */
    Oled_WriteCommand(0x3F); /* -> 1/64 duty (64-row panel)       */
    Oled_WriteCommand(0xA4); /* resume RAM content display       */
    Oled_WriteCommand(0xD3); /* display offset                   */
    Oled_WriteCommand(0x00);
    Oled_WriteCommand(0xD5); /* display clock divide ratio       */
    Oled_WriteCommand(0x80);
    Oled_WriteCommand(0xD9); /* pre-charge period                */
    Oled_WriteCommand(0x22);
    Oled_WriteCommand(0xDA); /* COM pins hardware configuration  */
    Oled_WriteCommand(0x12);
    Oled_WriteCommand(0xDB); /* VCOMH deselect level              */
    Oled_WriteCommand(0x40);
    Oled_WriteCommand(0xAD); /* DC-DC control (SH1106 charge pump)*/
    Oled_WriteCommand(0x8B); /* -> enable internal DC-DC          */
    Oled_WriteCommand(0xAF); /* display ON                       */
    Oled_Clear();
}

void Oled_Clear(void)
{
    memset(Oled_Buffer, 0x00, sizeof(Oled_Buffer));//void *memset(void *ptr, int value, size_t num);
    Oled_UpdateScreen();
}
 
typedef struct {
    char ch;
    uint8_t cols[5];
} Oled_Glyph_t;
 
static const Oled_Glyph_t Oled_Font[] = {
    { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { '!', { 0x00, 0x00, 0x5F, 0x00, 0x00 } },
    { '*', { 0x14, 0x08, 0x3E, 0x08, 0x14 } },
    { '/', { 0x40, 0x30, 0x08, 0x06, 0x01 } },
    { '1', { 0x00, 0x42, 0x7F, 0x40, 0x00 } },
    { '2', { 0x42, 0x61, 0x51, 0x49, 0x46 } },
    { ':', { 0x00, 0x00, 0x14, 0x00, 0x00 } },
    { 'A', { 0x7C, 0x12, 0x11, 0x12, 0x7C } },
    { 'B', { 0x7F, 0x49, 0x49, 0x49, 0x36 } },
    { 'C', { 0x3E, 0x41, 0x41, 0x41, 0x41 } },
    { 'D', { 0x7F, 0x41, 0x41, 0x41, 0x3E } },
    { 'E', { 0x7F, 0x49, 0x49, 0x49, 0x41 } },
    { 'F', { 0x7F, 0x09, 0x09, 0x09, 0x01 } },
    { 'H', { 0x7F, 0x08, 0x08, 0x08, 0x7F } },
    { 'N', { 0x7F, 0x02, 0x04, 0x08, 0x7F } },
    { 'L', { 0x7F, 0x40, 0x40, 0x40, 0x00 } },
    { 'S', { 0x46, 0x49, 0x49, 0x49, 0x31 } },
    { 'T', { 0x01, 0x01, 0x7F, 0x01, 0x01 } },
    { 'W', { 0x7F, 0x20, 0x18, 0x20, 0x7F } },
    { 'a', { 0x30, 0x4A, 0x4A, 0x2A, 0x7C } },
    { 'c', { 0x3C, 0x42, 0x42, 0x42, 0x00 } },
    { 'd', { 0x38, 0x44, 0x44, 0x44, 0x7F } },
    { 'e', { 0x3C, 0x4A, 0x4A, 0x4A, 0x0C } },
    { 'f', { 0x04, 0x7E, 0x05, 0x05, 0x00 } },
    { 'g', { 0x0C, 0x52, 0x52, 0x52, 0x3E } },
    { 'i', { 0x00, 0x44, 0x7D, 0x40, 0x00 } },
    { 'l', { 0x00, 0x41, 0x7F, 0x40, 0x00 } },
    { 'm', { 0x7C, 0x04, 0x38, 0x04, 0x78 } },
    { 'n', { 0x7C, 0x08, 0x04, 0x04, 0x78 } },
    { 'o', { 0x38, 0x44, 0x44, 0x44, 0x38 } },
    { 'r', { 0x7C, 0x08, 0x04, 0x04, 0x08 } },
    { 's', { 0x44, 0x4A, 0x4A, 0x4A, 0x30 } },
    { 't', { 0x00, 0x02, 0x3F, 0x42, 0x40 } },
    { 'u', { 0x3C, 0x40, 0x40, 0x20, 0x7C } },
    { '3', { 0x21, 0x41, 0x45, 0x4B, 0x31 } },
    { '0', { 0x3E, 0x51, 0x49, 0x45, 0x3E } },
    { '4', { 0x08, 0x14, 0x22, 0x7F, 0x08 } },
    { '5', { 0x47, 0x45, 0x45, 0x45, 0x39 } },
    { '6', { 0x3E, 0x49, 0x49, 0x49, 0x32 } },
    { '7', { 0x01, 0x01, 0x79, 0x05, 0x03 } },
    { '8', { 0x36, 0x49, 0x49, 0x49, 0x36 } },
    { '9', { 0x06, 0x49, 0x49, 0x29, 0x1E } },
    { 'x', { 0x44, 0x28, 0x10, 0x28, 0x44 } },
};

static const Oled_Glyph_t *Oled_FindGlyph(char ch)
{
    for (uint32_t i = 0; i < OLED_FONT_COUNT; i++) {
        if (Oled_Font[i].ch == ch) {
            return &Oled_Font[i];
        }
    }
    return &Oled_Font[0]; // nếu không có thì trả khoảng trắng
}

static void Oled_SetPixel(int16_t x, int16_t y)
{
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) {
        return;
    }
    Oled_Buffer[x + (y / 8) * OLED_WIDTH] |= (1 << (y % 8));
} /* y/8 -> số trang - byte nào trong cột từ trên xuống theo cột
     y%8 -> bit nào trong byte
     1 << (y % 8) -> bật bit mà không làm ảnh hưởng các bit khác */

static void Oled_DrawChar(int16_t x0, int16_t y0, char ch, uint8_t scale)
{
    const Oled_Glyph_t *g = Oled_FindGlyph(ch);
    for (uint8_t col = 0; col < GLYPH_W; col++) {
        uint8_t bits = g->cols[col];
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                for (uint8_t sy = 0; sy < scale; sy++) {
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        Oled_SetPixel(x0 + col * scale + sx, y0 + row * scale + sy);
                    }
                }
            }
        }
    }
}/* tìm ch , vẽ trên oled với scale*/
 
static uint16_t Oled_TextWidth(const char *str, uint8_t scale)
{
    uint16_t len = (uint16_t)strlen(str);
    if (len == 0) return 0;
    return len * (GLYPH_W + GLYPH_GAP) * scale - GLYPH_GAP * scale;// khoảng cách chuỗi, chữ cuối không cần khoảng trống nên trừ ra
}

static void Oled_DrawLineCentered(const char *str, int16_t y0, uint8_t scale)
{
    uint16_t w = Oled_TextWidth(str, scale);
    int16_t x = (OLED_WIDTH - (int16_t)w) / 2;
    if (x < 0) x = 0;
 
    for (const char *p = str; *p != '\0'; p++) {
        Oled_DrawChar(x, y0, *p, scale);
        x += (GLYPH_W + GLYPH_GAP) * scale;
    }
}// căn giữa

static void Oled_ShowLines(const char *line1, uint8_t scale1,
                            const char *line2, uint8_t scale2)
{
    memset(Oled_Buffer, 0x00, sizeof(Oled_Buffer));
 
    uint8_t h1 = 7 * scale1;
    uint8_t h2 = (line2 != NULL) ? 7 * scale2 : 0;
    uint8_t gap = (line2 != NULL) ? 4 : 0;
    int16_t blockHeight = h1 + gap + h2;
    int16_t y1 = (OLED_HEIGHT - blockHeight) / 2;
 
    Oled_DrawLineCentered(line1, y1, scale1);
    if (line2 != NULL) {
        Oled_DrawLineCentered(line2, y1 + h1 + gap, scale2);
    }
 
    Oled_UpdateScreen();
}
 
void Oled_ShowStatus(Oled_Msg_t msg)
{
    switch (msg) {
        case OLED_MSG_SCANNING:
            Oled_ShowLines("Scanning!", 2, NULL, 0);
            break;
        case OLED_MSG_WELCOME:
            Oled_ShowLines("Welcome!", 2, NULL, 0);
            break;
        case OLED_MSG_DENIED:
            Oled_ShowLines("Denied!", 2, NULL, 0);
            break;
        case OLED_MSG_ADMIN_MENU:
            Oled_ShowLines("Hi Boss!", 2, "1:ADD 2:DEL 3:CA", 1);
            break;
        case OLED_MSG_SCAN_ADD_CARD:
            Oled_ShowLines("Scan", 2, "to add!", 2);
            break;
        case OLED_MSG_ERROR:
            Oled_ShowLines("Error!", 2, NULL, 0);
            break;
        case OLED_MSG_SCAN_DELETE_CARD:
            Oled_ShowLines("Scan", 2, "to delete!", 2);
            break;
        case OLED_MSG_CARD_EXISTS:
            Oled_ShowLines("Card", 2, "Exists!", 2);
            break;
        case OLED_MSG_CARD_ADDED:
            Oled_ShowLines("Card", 2, "Added!", 2);
            break;
        case OLED_MSG_CARD_DELETED:
            Oled_ShowLines("Card", 2, "Deleted!", 2);
            break;
        case OLED_MSG_NOT_FOUND:
            Oled_ShowLines("Not Found!", 2, NULL, 0);
            break;
        case OLED_MSG_ADMIN_CARD:
            Oled_ShowLines("No Delete", 2, "Admin!", 2);
            break;
        case OLED_MSG_PASSWORD_INPUT:
            Oled_ShowLines("Enter Code", 2, NULL, 0);
            break;
        case OLED_MSG_LOCKED:
            Oled_ShowLines("Secure!", 2, NULL, 0);
            break;
        case OLED_MSG_SCAN_NEW_ADMIN:
            Oled_ShowLines("Scan", 2, "Admin!", 2);
            break;
        case OLED_MSG_ADMIN_CHANGED:
            Oled_ShowLines("Admin", 2, "Done!", 2);
            break;
        case OLED_MSG_ADMIN_CHANGE_DENIED:
            Oled_ShowLines("Admin", 2, "Denied!", 2);
            break;
        default:
            Oled_ShowLines("Scanning!", 2, NULL, 0);
            break;
    }
}


// Nhung ham moi
void Oled_ShowPasswordMask(uint8_t length) {
    char mask[9]; // Toi da 8 ky tu mat khau
    if (length > 8) length = 8;

    for (uint8_t i = 0; i < length; i++) {
        mask[i] = '*';
    }
    mask[length] = '\0';

    Oled_ShowLines(mask, 2, NULL, 0);
<<<<<<< HEAD
=======
}

static void Oled_UintToStr(uint32_t value, char *buf)
{
    char tmp[11];
    uint8_t len = 0;

    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value > 0 && len < 10) {
            tmp[len++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    uint8_t i = 0;
    while (len > 0) {
        buf[i++] = tmp[--len];
    }
    buf[i] = '\0';
}

void Oled_ShowLockCountdown(uint32_t seconds) {
    char secStr[12];
    Oled_UintToStr(seconds, secStr);
    Oled_ShowLines("Secure!", 2, secStr, 2);
>>>>>>> f8e46b1c0b53cc08b60c7a3038903fc4de54fee1
}

static void Oled_UintToStr(uint32_t value, char *buf)
{
    char tmp[11];
    uint8_t len = 0;

    if (value == 0) {
        tmp[len++] = '0';
    } else {
        while (value > 0 && len < 10) {
            tmp[len++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }

    uint8_t i = 0;
    while (len > 0) {
        buf[i++] = tmp[--len];
    }
    buf[i] = '\0';
}

void Oled_ShowLockCountdown(uint32_t seconds) {
    char secStr[12];
    Oled_UintToStr(seconds, secStr);
    Oled_ShowLines("Secure!", 2, secStr, 2);
}