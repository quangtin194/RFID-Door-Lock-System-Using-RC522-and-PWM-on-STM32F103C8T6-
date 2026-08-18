# RFID Door Lock System — STM32F103C8T6

> Khóa cửa RFID dùng thẻ Mifare (RC522) + servo mở khóa + OLED SH1106 1.3" hiển thị trạng thái.
> Đồ án nhóm (3 thành viên: QuangTin, DucThinh, ...) — sinh viên Kỹ thuật.
> **File này viết cho AI/người đọc mới**: chỉ cần đọc README này là hiểu toàn bộ dự án.

---

## 1. Tổng quan

- **MCU**: STM32F103C8T6 (Blue Pill, 64 KB Flash, 20 KB RAM, 72 MHz)
- **Chức năng**: quẹt thẻ RFID → nếu thẻ hợp lệ → servo xoay mở khóa + màn hình hiện "Welcome!"; thẻ không hợp lệ → "Denied!" + buzzer kêu. Có thẻ **admin** vào chế độ quản trị để thêm/xóa thẻ qua 2 nút nhấn.
- **Giao diện người dùng**: OLED SH1106 1.3" (I²C) hiển thị trạng thái; USART1 (115200 baud) in log debug ra PC.
- **Nguồn gốc code**: dự án CubeMX (phiên bản 6.18.0) sinh ra khung (Core/, HAL), toàn bộ logic nằm trong 7 module tự viết ở `Drivers/Modules/` + driver MFRC522 ở `Core/Src/mfrc522.c`.
- **Trình biên dịch**: CMake + Ninja + arm-none-eabi-gcc 12.2 (toolchain file `cmake/gcc-arm-none-eabi.cmake`).

---

## 2. Sơ đồ chân (Pin map)

Nguồn chân lý: `PRJ.ioc` + `Core/Src/stm32f1xx_hal_msp.c` + `main.c`.

| Chân | Chức năng | Chi tiết |
|------|-----------|----------|
| PD0 / PD1 | **HSE 8 MHz** (thạch anh ngoài) | Nguồn xung cho PLL |
| PA0 | **TIM2_CH1 → Servo** | PWM 50 Hz, pulse 1.0–2.0 ms |
| PA1 | **EXTI1 → Nút ADD** | Pull-up, kích rising edge (ngắt), chỉ hoạt động ở ADMIN_MODE |
| PA2 | **EXTI2 → Nút DELETE** | Pull-up, kích rising edge (ngắt) |
| PA3 | **GPIO Output → Buzzer** | Active HIGH (1 = kêu) |
| PA5 / PA6 / PA7 | **SPI1 SCK / MISO / MOSI → RC522** | Mode 0 (CPOL=0, CPHA=0), 9 Mbit/s |
| PB0 | **GPIO Output → RC522 CS** | Chip select, active LOW |
| PB6 / PB7 | **I2C1 SCL / SDA → OLED SH1106** | 100 kHz standard mode, AF_OD |
| PA9 / PA10 | **USART1 TX / RX → Debug PC** | 115200, 8N1 |
| PA13 / PA14 | **SWD** (debug/nạp) | NOJTAG (vô hiệu JTAG, giữ SWD) |

---

## 3. Cấu hình xung clock (72 MHz — quan trọng)

`SystemClock_Config()` trong `Core/Src/main.c`:

```
HSE 8 MHz ──PLL ×9──▶ SYSCLK 72 MHz
  ├─ AHB /1  ─▶ HCLK 72 MHz
  ├─ APB1 /2 ─▶ PCLK1 36 MHz  ─▶ TIM2 clock = 72 MHz  (F1: timer = 2×APB1 khi APB1 ≠ 1)
  └─ APB2 /1 ─▶ PCLK2 72 MHz  (TIM1 nếu dùng = 72 MHz)
```

**Hệ quả cho servo (TIM2)**: PSC = 71, ARR = 19999 → tick 1 µs, chu kỳ 20 ms = 50 Hz.
`Servo_SetAngle(angle)`: `CCR = 1000 + (1000 × angle)/90` → 0° = 1.0 ms, 90° = 2.0 ms.

⚠️ Đồ án **dùng HSE** (thạch anh ngoài trên PD0/PD1). Nếu board không hàn thạch anh, phải đổi clock về HSI trong `.ioc` rồi regen.

---

## 4. Cấu trúc thư mục

```
RFID-Door-Lock-System-Using-RC522-and-PWM-on-STM32F103C8T6-/
├── PRJ.ioc                     # Cấu hình CubeMX (nguồn sự thật về pin/clock)
├── CMakeLists.txt              # Build script chính (tên dự án: PRJ)
├── CMakePresets.json           # Preset Debug/Release (binaryDir = build/<preset>)
├── build_and_flash.bat         # Build + nạp 1 cú (xóa build/ trước)
├── startup_stm32f103xb.s       # Vector table (Reset_Handler...)
├── STM32F103xx_FLASH.ld        # Linker script
├── cmake/
│   ├── gcc-arm-none-eabi.cmake # Toolchain file (cross-compiler)
│   └── stm32cubemx/CMakeLists.txt  # CubeMX sinh: nạp Core/* + HAL + CMSIS + startup
├── Core/
│   ├── Inc/  main.h, mfrc522.h, stm32f1xx_hal_conf.h, stm32f1xx_it.h
│   └── Src/
│       ├── main.c              # ★ SystemClock_Config, MX_*_Init, handle toàn cục, main()
│       ├── mfrc522.c           # ★ Driver SPI mức thấp MFRC522 (thư viện kiểu TM)
│       ├── stm32f1xx_it.c      # ★ ISR: EXTI1/EXTI2, TIM2, USART1, SysTick
│       ├── stm32f1xx_hal_msp.c # ★ Cấu hình chân GPIO cho từng ngoại vi (I2C1, SPI1, TIM2, USART1)
│       ├── syscalls.c / sysmem.c / system_stm32f1xx.c   # CubeMX sinh, không sửa
├── Drivers/
│   ├── Modules/                # ★★ 7 module tự viết (phần quan trọng nhất)
│   │   ├── Inc/  App.h Button.h Buzzer.h Oled.h RC522.h Servo.h UART.h
│   │   └── Src/  App.c Button.c Buzzer.c Oled.c RC522.c Servo.c UART.c
│   ├── STM32F1xx_HAL_Driver/   # ST HAL (không sửa). "Inc copy/" là backup thừa, không dùng
│   └── CMSIS/                  # Bulk (đa số không dùng trong build)
└── build/                      # Sinh ra khi build (đã gitignore)
```

---

## 5. Luồng hoạt động chương trình

```
Reset → startup_stm32f103xb.s → main()
  → HAL_Init()
  → SystemClock_Config()            (HSE ×9 = 72 MHz)
  → MX_GPIO_Init / MX_I2C1_Init / MX_SPI1_Init / MX_TIM2_Init / MX_USART1_UART_Init
  → App_Init(&button, &buzzer, &huart1, &hi2c1, &hspi1, &servo)   ← khởi tạo 6 module
  → while(1) { App_Run(); }          ← state machine chạy mãi
```

`App_Init` nạp lần lượt: Oled → UART → RC522 → Servo → Buzzer → Button, rồi đặt trạng thái ban đầu `IDLE`.

### Máy trạng thái (App.c) — pattern Entry/Execute

Mỗi trạng thái có 2 pha:
- **Entry** (chạy 1 lần khi trạng thái đổi, nhờ so sánh `appState != previous_State`): cập nhật OLED/buzzer/servo.
- **Execute** (chạy mỗi vòng lặp): xử lý logic, chuyển trạng thái.

```
                        ┌──────────────────────────────┐
                        ▼                              │ (quẹt lại thẻ OK)
  ┌────────┐  có thẻ   ┌──────────┐  verify UID       ┌────────────┐
  │  IDLE  │──────────▶│VERIFY_UID│──┬───────────────▶│ ERROR_STATE│
  └────────┘           └──────────┘  │ (lỗi HW RC522) └────────────┘
   ▲  ▲       ▲            │         │
   │  │       │            ├─ UID_ADMIN ─▶ ADMIN_MODE ── (nút ADD/DEL) ─▶ ADD_CARD / DELETE_CARD
   │  │       │            ├─ UID_VALID ─▶ ACCESS_ALLOWED  ("Welcome!", servo mở 90°, 1 s)
   │  │       │            └─ UID_INVALID▶ ACCESS_DENIED   ("Denied!", buzzer, 1 s)
   │  │       └─────── timeout (1 s / 5 s) ── quay về IDLE
   └──┴─────── CARD_ADDED / CARD_EXISTS / CARD_DELETED / DELETE_DENIED (hiện 1 s rồi về IDLE)
```

| Trạng thái | Entry làm gì | Execute làm gì | Timeout |
|---|---|---|---|
| `IDLE` | Servo đóng (0°), tắt buzzer, "Scanning!" | Gọi `RC522_UID_Detected()`: có thẻ → VERIFY_UID; lỗi phần cứng → ERROR_STATE | — |
| `VERIFY_UID` | — | `RC522_UID_Verify()` → ADMIN/VALID/INVALID | gán mốc thời gian |
| `ADMIN_MODE` | **Bật EXTI nút**, servo mở, "Hi Boss!" / "1:ADD 2:DELETE" | chờ nút hoặc timeout | 5 s |
| `ACCESS_ALLOWED` | Servo mở, "Welcome!", log UID | chờ timeout | 1 s |
| `ACCESS_DENIED` | "Denied!", buzzer bật, log UID | chờ timeout | 1 s |
| `ADD_CARD` | "Scan to add!" | có thẻ → `RC522_UID_Add()` → CARD_ADDED / CARD_EXISTS | 5 s |
| `DELETE_CARD` | "Scan to delete!" | có thẻ → `RC522_UID_Delete()` → CARD_DELETED / DELETE_DENIED | 5 s |
| `CARD_ADDED` / `CARD_EXISTS` / `CARD_DELETED` / `DELETE_DENIED` | hiện thông báo tương ứng | chờ timeout | 1 s |
| `ERROR_STATE` | Servo đóng, buzzer bật, "Error!" | có thẻ OK → về IDLE | — |

### Xử lý ngắt nút (điểm tinh tế)

`HAL_GPIO_EXTI_Callback` (trong App.c) chỉ chuyển trạng thái khi `appState == ADMIN_MODE`:
- PA1 (ADD) → `ADD_CARD`, PA2 (DEL) → `DELETE_CARD`, kèm gán `Timeout_counter = HAL_GetTick()`.

EXTI chỉ được bật khi vào ADMIN_MODE (`Button_EnableEXTI()`), tắt khi về IDLE/ERROR (`Button_DisableEXTI()`) — chống quẹt phím vô tình. Ghi chú trong code: *"Ngắt sẽ thay đổi luồng chương trình, nhớ trong hàm xử lý ngắt có update Timeout_counter"* — vì nếu không gia hạn timeout, ADMIN_MODE sẽ hết hạn 5 s và thoát ngay khi bấm nút.

---

## 6. Mô tả từng module (`Drivers/Modules/`)

### 6.1 `App` — máy trạng thái chính
- `App_Init(...)` / `App_Run()` — mô tả ở mục 5.
- Enum `AppState_t` (12 trạng thái) định nghĩa trong `App.h`, kèm `CLOSE_ANGLE=0`, `OPEN_ANGLE=90`, `TIMEOUT_S_WAIT=1000 ms`, `TIMEOUT_L_WAIT=5000 ms`.

### 6.2 `RC522` — quản lý danh sách thẻ (tầng logic)
Biến toàn cục quan trọng:
```c
uint8_t CurrentUID[5];                 // UID thẻ đang quẹt (4 byte + 1 byte BCC từ Anticoll)
uint8_t AdminUID[4] = {0x53,0x4F,0x42,0x28};   // thẻ admin hardcode — không thể xóa
uint8_t AuthorizedCards[MAX_CARDS][4]; // MAX_CARDS = 4 — danh sách thẻ hợp lệ (RAM!)
```
- `RC522_UID_Detected()` → `RC522_OK` / `RC522_NO_CARD` / `RC522_ERROR`:
  1. **Tự kiểm tra phần cứng**: đọc register Version (0x37) — `0x00` = mất GND/MISO, `0xFF` = treo dây → trả `RC522_ERROR` ngay (điểm cộng của đồ án, driver gốc không có).
  2. `TM_MFRC522_Request(PICC_REQIDL)` (REQA 0x26) → `TM_MFRC522_Anticoll(CurrentUID)` nạp UID.
- `RC522_UID_Verify()` → `UID_ADMIN` / `UID_VALID` / `UID_INVALID` (so sánh từng dòng `AuthorizedCards`, bỏ qua dòng trống `[i][0]==0x00`).
- `RC522_UID_Add()` → `UID_ADMIN` (không thêm được admin), `UID_EXIST` (đã có), `UID_NEW` (đã thêm), `UID_EXIST` khi bảng đầy.
- `RC522_UID_Delete()` → `UID_ADMIN` (admin không xóa được), `UID_EXIST` (đã xóa — lưu ý tên enum hơi ngược: trả EXIST nghĩa là "tìm thấy và đã xóa"), `UID_NEW` (không tìm thấy).
- **Thuật toán**: duyệt tuyến tính mảng 4 phần tử — hợp lý vì bảng nhỏ cố định; `memcmp` so UID 4 byte.

### 6.3 `mfrc522` (Core/Src) — driver SPI mức thấp (thư viện kiểu Tilen Majerle)
- `TM_MFRC522_WriteRegister/ReadRegister`: SPI 2 byte — byte địa chỉ `(addr<<1)&0x7E` (+ bit 0x80 khi đọc), CS = PB0. SPI1 mode 0, 9 Mbit/s.
- `TM_MFRC522_Init`: Reset → T_MODE 0x8D, T_PRESCALER 0x3E, T_RELOAD 30 (timer 150 kHz cho framing) → RF_CFG 0x70 → TX_AUTO 0x40 → MODE 0x3D → bật antenna.
- `TM_MFRC522_ToCard`: transceive tổng quát — ghi FIFO, chạy lệnh `PCD_TRANSCEIVE`, **chờ IRQ bằng vòng lặp polling** (tối đa 2000 lần đọc, ~25 ms cho thẻ M1), kiểm tra thanh ghi Error (mask 0x1B), đọc FIFO.
- `TM_MFRC522_Request`: REQA → đúng 16 bit trả về (`backBits == 0x10`) mới coi là OK.
- `TM_MFRC522_Anticoll`: gửi lệnh ANTICOLL (0x93, 0x20) → nhận 5 byte (4 UID + 1 BCC) → **kiểm tra XOR checksum**: `serNum[0]^serNum[1]^serNum[2]^serNum[3] == serNum[4]`.
- **Không dùng** authentication/đọc khối (PCD_AUTHENT có sẵn nhưng không gọi) — dự án chỉ lấy UID.

### 6.4 `Servo` — PWM TIM2 CH1 (PA0)
```c
ccr = 1000 + ((1000 * angle) / 90);   // 0° → 1.0 ms, 90° → 2.0 ms, frame 50 Hz
__HAL_TIM_SET_COMPARE(htim, channel, ccr);
```
`Servo_Init` chỉ lưu handle + `HAL_TIM_PWM_Start`. (TIM2 general-purpose, không cần MOE enable.)

### 6.5 `Button` — quản lý EXTI 2 nút
Struct chứa sẵn pin + IRQn; `Button_EnableEXTI/DisableEXTI` bật/tắt NVIC + xóa pending. `Button_Handle` là biến **global** (extern) vì `HAL_GPIO_EXTI_Callback` trong App.c truy cập trực tiếp.

### 6.6 `Buzzer` — PA3 active HIGH (`Buzzer_on` = SET, `Buzzer_off` = RESET).

### 6.7 `UART` — debug
- `UART_PC_Print`: `HAL_UART_Transmit` **blocking**, timeout 100 ms.
- `UART_Print_UID`: gọi lại `TM_MFRC522_Anticoll(CurrentUID)` (đọc lại UID từ thẻ) rồi in hex `%02X %02X %02X %02X`.

### 6.8 `Oled` — SH1106 1.3" (đã phân tích chi tiết ở tài liệu Oled_SH1106_PhanTich_ChiTiet.pdf)
- I²C1 100 kHz, địa chỉ 0x3C (HAL dùng 0x78), framebuffer 1024 byte + `HAL_I2C_Mem_Write_DMA`.
- **Bù cột offset 2** (RAM 132 cột vs panel 128), chuỗi khởi tạo chuẩn SH1106 kèm DC-DC `0xAD/0x8B`.
- API public: `Oled_Init()` + `Oled_ShowStatus(Oled_Msg_t)` — map enum trạng thái → chuỗi (scale 2/1).

---

## 7. Các hằng số / ngưỡng quan trọng

| Hằng số | Giá trị | Nơi định nghĩa | Ý nghĩa |
|---|---|---|---|
| `MAX_CARDS` | 4 | RC522.c | Số thẻ người dùng tối đa |
| `AdminUID` | 53 4F 42 28 | RC522.c | UID thẻ admin (mặc định, hardcode) |
| `CLOSE/OPEN_ANGLE` | 0 / 90 | App.h | Góc servo đóng/mở |
| `TIMEOUT_S/L_WAIT` | 1000 / 5000 ms | App.h | Chờ ngắn (thông báo) / dài (menu admin, quét thẻ) |
| `TIM2 PSC/ARR` | 71 / 19999 | main.c | 1 µs tick, 50 Hz |
| `I2C1 ClockSpeed` | 100000 | main.c | 100 kHz (SH1106 chấp nhận tới 400 kHz) |
| `SPI1 Prescaler` | 8 | main.c | 72/8 = 9 Mbit/s |
| `USART1 Baud` | 115200 | main.c | Debug log |
| NVIC priority | EXTI=1, TIM2=2, USART1=3 | main.c | Ưu tiên ngắt |

---

## 8. Build & nạp firmware

**Cách 1 — CMake thuần (khuyến nghị):**
```bash
cmake --preset Debug            # binaryDir = build/Debug (tự động dùng toolchain file)
cmake --build --preset Debug    # ra PRJ.elf / PRJ.hex / PRJ.bin trong build/Debug
```

**Cách 2 — Script 1 cú (build_and_flash.bat):** xóa `build/`, configure với `-B build -G Ninja`, build, nạp qua ST-Link SWD:
```bat
build_and_flash.bat   :: cần STM32_Programmer_CLI trong PATH; nạp build/PRJ.elf @ 0x08000000
```

**Lưu ý build:**
- Preset dùng `cmake/gcc-arm-none-eabi.cmake`; không cần chỉ toolchain bằng tay.
- `build/` và `build/Debug/` có thể bị xóa bất cứ lúc nào → **luôn reconfigure trước build** (CMakeCache lỗi thời từ máy khác gây lỗi "directory ... is different").
- `.hex`/`.bin` được sinh tự động nhờ POST_BUILD objcopy trong CMakeLists.txt.
- Bản nạp chuẩn: `STM32_Programmer_CLI -c port=SWD -w build/Debug/PRJ.elf 0x08000000 -v -rst`.

---

## 9. Hạn chế & lưu ý đã biết

1. **Danh sách thẻ KHÔNG bền vững**: `AuthorizedCards` nằm trong RAM — reset hoặc mất nguồn là mất hết thẻ đã thêm, chỉ còn thẻ admin. (Hướng mở rộng: lưu vào flash nội, ví dụ nhân bản qua EEPROM emulation hoặc ghi sector flash.)
2. **Chỉ chứa được 4 thẻ người dùng** (`MAX_CARDS=4`).
3. **Admin UID hardcode** trong source — muốn đổi admin phải sửa lại code và nạp lại.
4. **I2C chạy 100 kHz** dù SH1106 hỗ trợ 400 kHz → mỗi lần vẽ full screen ~23 ms. Nếu cần nhanh hơn, tăng `ClockSpeed` lên 400000.
5. **`.ioc` còn rác**: `functionlistsort` vẫn liệt kê `MX_TIM1_Init` nhưng main.c không khai báo/cấu hình TIM1 — khi regen CubeMX cần dọn hoặc bỏ qua (không ảnh hưởng build).
6. **Thư mục `Drivers/STM32F1xx_HAL_Driver/Inc copy/`** là bản backup thừa — không nằm trong include path, nên xóa cho gọn.
7. **`UART_Print_UID` gọi lại Anticoll** — nếu thẻ đã rời khỏi anten giữa chừng, lần đọc lại có thể fail và log UID trống.
8. **Oled DMA fire-and-forget** không kiểm tra BUSY — an toàn với tần suất update theo sự kiện (quẹt thẻ), không dùng trong vòng lặp 1 ms.
9. **Clock phụ thuộc HSE 8 MHz** — board không có thạch anh sẽ không chạy (đổi sang HSI nếu cần).
10. Trạng thái git hiện tại: đang sửa dở `CMakeLists.txt`, `RC522.c` + file `.clangd`, `.gitignore`, `.settings/` chưa commit.

---

## 10. Cách AI tiếp cận khi được giao việc sửa code

1. Đọc `main.c` trước → biết clock thật, handle ngoại vi, thứ tự init.
2. Sửa logic nghiệp vụ → chỉ đụng `Drivers/Modules/` (App.c là nơi hay sửa nhất).
3. Sửa driver mức thấp RC522 → `Core/Src/mfrc522.c` (SPI + thanh ghi), pin CS = PB0.
4. Trước khi patch bất kỳ file nào: **re-read** (file bị sửa ngoài luồng/IDE autosave có thể revert patch).
5. Sau khi sửa: `cmake --preset Debug && cmake --build --preset Debug` — nếu link lỗi `undefined reference to SystemClock_Config/MX_*`, nguyên nhân là main.c bị thay/truncate, không phải CMake.
6. Trả lời người dùng bằng tiếng Việt.
