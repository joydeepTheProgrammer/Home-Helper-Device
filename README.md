🏠 Home Helper Device — STM32 Edition
A fully autonomous home automation & safety system powered by the STM32F103C8T6 (Blue Pill)
Built with ARM Cortex-M3 @ 72 MHz · STM32 HAL · Embedded C11

📸 Block Diagram
Block Diagram

📋 Table of Contents
Overview
Features
Hardware Requirements
Pin Mapping
Project Structure
System Architecture
Clock & Peripheral Configuration
Building the Project
Flashing to Hardware
Bluetooth Commands
LCD Menu Navigation
AVR → STM32 Migration Notes
Troubleshooting
License
🔍 Overview
The Home Helper Device is an embedded system that monitors your home environment in real-time and automatically controls appliances, fan speed, lighting, and alarms — without any cloud dependency. Everything runs locally on the MCU.

Property	Value
Target MCU	STM32F103C8T6 (Blue Pill)
Architecture	ARM Cortex-M3
Clock Speed	72 MHz (HSE 8 MHz → PLL ×9)
Flash	64 KB
RAM	20 KB
HAL	STM32F1xx HAL (STM32CubeF1)
Standard	C11 (-std=c11)
UART Baud	115200 (Debug + Bluetooth)
Fan PWM Freq	1 kHz (TIM3 CH1)
ADC Resolution	12-bit (0–4095)
✨ Features
#	Feature	Implementation
1	🌡️ Temperature & Humidity	DHT11 via 1-wire · reads every 2 s
2	💨 Auto Fan Speed Control	Linear PWM: 30°C→0%, 35°C→100%
3	🔥 Gas / Smoke Detection	MQ-2 ADC · warning + alarm levels
4	💡 Ambient Light Automation	LDR ADC · relay-controlled room light
5	🚶 Motion Detection	HC-SR501 PIR · occupancy logic
6	🖥️ 5-Page LCD Menu	UP/DOWN/SELECT button navigation
7	📱 Bluetooth Control	HC-05 via USART1 · remote relay/fan
8	🚨 Multi-Level Alarm	Buzzer + LED: Safe / Warning / Alarm
9	🔒 Safety Interlocks	Relay OFF on gas alarm or critical temp
10	🕹️ Manual Override	SELECT button cycles relay override mode
🛠️ Hardware Requirements
Core Components
Component	Model	Qty
Microcontroller	STM32F103C8T6 Blue Pill	1
Temp & Humidity Sensor	DHT11	1
Gas / Smoke Sensor	MQ-2	1
Motion Sensor	HC-SR501 PIR	1
Light Sensor	LDR + 10 kΩ resistor (voltage divider)	1
Display	16×2 HD44780 LCD	1
DC Fan	5V brushless fan + N-MOSFET (e.g. 2N7000)	1
Relay Module	5V relay (active LOW)	1
Buzzer	Passive buzzer 5V	1
Status LED	RGB LED (common cathode) + 3× 220Ω	1
Bluetooth Module	HC-05	1
Tactile Buttons	6mm tactile switches	3
Programmer	ST-Link V2	1
Power Supply

12V DC Adapter
    └─→ LM7805 (TO-220)   → 5V rail  (LCD VDD, Relay, Fan, HC-05)
    └─→ AMS1117-3.3V      → 3.3V rail (STM32 VDD, sensors)
⚠️ Never power the STM32 3.3V pin with 5V — it will damage the MCU.

📌 Pin Mapping
STM32 Pin	GPIO	Direction	Connected To
PA0	ADC1_IN0	Analog IN	MQ-2 Gas Sensor
PA1	ADC1_IN1	Analog IN	LDR Light Sensor
PA6	TIM3_CH1	PWM OUT	Fan MOSFET gate
PA9	USART1_TX	OUT	HC-05 RX / USB-UART TX
PA10	USART1_RX	IN	HC-05 TX / USB-UART RX
PB0	GPIO_OUT	Push-pull	RGB LED — Red
PB1	GPIO_OUT	Push-pull	RGB LED — Green
PB2	GPIO_OUT	Push-pull	RGB LED — Blue
PB3	GPIO_OUT	Push-pull	Relay IN (Active LOW)
PB4	GPIO_OUT	Push-pull	Buzzer
PB6	GPIO_IO	Open-drain	DHT11 Data
PB7	GPIO_IN	No pull	PIR OUT
PB8	GPIO_OUT	Push-pull	LCD RS
PB9	GPIO_OUT	Push-pull	LCD E (Enable)
PB10	GPIO_OUT	Push-pull	LCD D4
PB11	GPIO_OUT	Push-pull	LCD D5
PB12	GPIO_OUT	Push-pull	LCD D6
PB13	GPIO_OUT	Push-pull	LCD D7
PC13	GPIO_IN	Pull-up	Button UP (active LOW)
PC14	GPIO_IN	Pull-up	Button DOWN (active LOW)
PC15	GPIO_IN	Pull-up	Button SELECT (active LOW)
SWDIO	Debug	—	ST-Link DATA
SWCLK	Debug	—	ST-Link CLK
📁 Project Structure

HomeHelperDevice_STM32/
│
├── Core/
│   ├── Inc/
│   │   ├── main.h           # Error_Handler declaration
│   │   ├── config.h         # All pin macros, HAL wrappers, thresholds
│   │   ├── delay_us.h       # DWT µs delay API
│   │   ├── dht11.h          # DHT11 driver API
│   │   ├── lcd.h            # HD44780 4-bit LCD API
│   │   ├── uart_comm.h      # UART helpers API
│   │   └── adc_sensor.h     # ADC read/average API
│   │
│   └── Src/
│       ├── main.c           # HAL init + system clock + main loop
│       ├── delay_us.c       # DWT cycle counter µs delay
│       ├── dht11.c          # 1-wire protocol implementation
│       ├── lcd.c            # HD44780 nibble-mode driver
│       ├── uart_comm.c      # USART1 Tx/Rx communication
│       └── adc_sensor.c     # ADC1 channel switching + averaging
│
├── Drivers/                 # ← Add STM32CubeF1 HAL here (see Setup)
│   ├── STM32F1xx_HAL_Driver/
│   │   ├── Inc/
│   │   └── Src/
│   └── CMSIS/
│       ├── Device/ST/STM32F1xx/Include/
│       └── Include/
│
├── docs/
│   └── stm32_home_helper_block_diagram.png
│
├── STM32F103C8TX_FLASH.ld   # Linker script (from CubeMX/CubeIDE)
├── startup_stm32f103c8tx.s  # Startup file (from CubeMX/CubeIDE)
├── Makefile                 # arm-none-eabi-gcc build system
└── README.md                # This file
🏗️ System Architecture
State Machine

                    ┌─────────┐
           Boot ──► │ NORMAL  │◄──────────────────┐
                    │ 🟢 LED  │                   │
                    └────┬────┘           All safe │
                         │                         │
              Gas≥1200 / Temp≥35°C / Hum≥70%       │
                         ▼                         │
                    ┌─────────┐                    │
                    │ WARNING │────────────────────►┘
                    │ 🟡 LED  │
                    │ Beep ↕  │
                    └────┬────┘
                         │
              Gas≥2400 / Temp≥40°C
                         ▼
                    ┌─────────┐
                    │  ALARM  │
                    │ 🔴 LED  │  Relay FORCED OFF
                    │ Buzzer  │  Fan → 100%
                    │ Fan MAX │
                    └─────────┘
  SELECT ──► OVERRIDE (🔵 LED) ──► SELECT again ──► NORMAL
Control Logic

Every 500 ms:
  gas_adc  = ADC_ReadAvg(CH0, 8 samples)
  ldr_adc  = ADC_ReadAvg(CH1, 4 samples)
  pir      = GPIO_Read(PB7)
Every 2000 ms:
  DHT11_Read() → temperature, humidity
Fan PWM:
  if gas_alarm  → duty = 999 (100%)
  elif temp < 30°C  → duty = 0
  elif temp ≥ 35°C  → duty = 999
  else → duty = (temp - 30) × 999 / 5   [linear]
Relay (Auto mode):
  if gas_alarm OR temp ≥ 40°C → OFF (safety)
  elif dark(ldr<1600) AND motion → ON
  elif bright → OFF
Buzzer:
  if gas_alarm  → continuous ON
  elif warning  → toggle every 500 ms
  else          → OFF
⚙️ Clock & Peripheral Configuration
System Clock Tree

HSE 8 MHz
    └─► PLL ×9 ─────────────────────────► SYSCLK = 72 MHz
                                              │
                    ┌─────────────────────────┴──────────────────┐
                    │                         │                  │
                AHB ÷1                    APB1 ÷2            APB2 ÷1
              = 72 MHz                  = 36 MHz            = 72 MHz
                                    (TIM3 clock             (ADC1 clock
                                    = 72 MHz via ×2)        → ÷6 = 12 MHz)
TIM3 PWM (Fan)

TIM3 Input Clock = 72 MHz
Prescaler (PSC)  = 71     →  Timer clock = 1 MHz
ARR (Period)     = 999    →  PWM freq    = 1 kHz
CCR1 range       = 0–999  →  Duty 0%–100%
ADC1

ADC Clock     = APB2 / 6 = 12 MHz  (within 14 MHz spec)
Resolution    = 12-bit (0–4095)
Sample Time   = 55.5 cycles → ~4.6 µs per sample
Trigger       = Software start
Calibration   = HAL_ADCEx_Calibration_Start() at boot
DWT Microsecond Delay

DWT_CYCCNT increments at 72 MHz (every 13.9 ns)
delay_us(n) = wait until ΔCycles ≥ n × 72
Handles 32-bit wraparound safely via subtraction
No timer peripheral consumed
🔨 Building the Project
Prerequisites
bash

# Install arm-none-eabi toolchain
# Windows (via MSYS2):
pacman -S mingw-w64-x86_64-arm-none-eabi-gcc
# or download: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
# Install OpenOCD (for flashing)
pacman -S mingw-w64-x86_64-openocd
# or: https://openocd.org/pages/getting-openocd.html
# Verify
arm-none-eabi-gcc --version
openocd --version
Get STM32 HAL Library
Option A (Recommended) — STM32CubeMX:

Download STM32CubeMX (free)
Create new project → STM32F103C8T6
Enable: USART1, ADC1, TIM3 CH1, all required GPIO
Generate code → copy the generated Drivers/ folder here
Option B — Direct Download:

bash

git clone --depth 1 https://github.com/STMicroelectronics/STM32CubeF1.git
cp -r STM32CubeF1/Drivers .
Compile
bash

cd HomeHelperDevice_STM32
make
# Output: build/home_helper_stm32.elf  and  .hex
Check Memory Usage
bash

make size
# Example output:
#    text    data     bss     dec     hex
#   18432     256    1024   19712    4D00    build/home_helper_stm32.elf
⚡ Flashing to Hardware
Wiring — ST-Link V2 to Blue Pill

ST-Link V2          Blue Pill
──────────          ─────────
  SWDIO     ──────►  PA13 (SWDIO)
  SWCLK     ──────►  PA14 (SWCLK)
  GND       ──────►  GND
  3.3V      ──────►  3.3V  (or power externally)
Flash Command
bash

make flash
# Uses: openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
#       -c "program build/home_helper_stm32.bin verify reset exit 0x08000000"
STM32CubeIDE (Easier for Beginners)
File → Import → Existing Project → select this folder
Copy Core/Inc/*.h and Core/Src/*.c files in
Click Build ▶ Run (auto-detects ST-Link)
Serial Monitor

Port   : COMx (Windows) or /dev/ttyUSB0 (Linux)
Baud   : 115200
Format : 8N1, no flow control
📱 Bluetooth Commands
Connect any Bluetooth Serial Terminal app (e.g. Serial Bluetooth Terminal on Android) to the HC-05 module (default PIN: 1234).

Command	Action
R	Relay ON (manual override)
r	Relay OFF (manual override)
A	Return to Auto mode
F	Fan → 100%
f	Fan → OFF
S	Print full status dump
?	Show help
Example status dump output:


=== HOME HELPER STM32 STATUS ===
Temp: 28C
Humidity: 55%
Gas ADC: 420
LDR ADC: 980
Motion: NO
Relay: OFF
Fan Duty: 0
State: NORMAL
================================
🖥️ LCD Menu Navigation
Button	Action
UP	Scroll to previous page
DOWN	Scroll to next page
SELECT	Cycle relay: OFF → ON → AUTO
Page	Display Content
0 — Home	Home Helper STM32 / Uptime + state
1 — Temp/Hum	Temperature, humidity, fan %, relay
2 — Gas	Raw ADC value + Safe / WARN! / ALARM!!
3 — Light	LDR ADC + Motion Y/N
4 — Settings	Relay mode: AUTO or MANUAL
🔄 AVR → STM32 Migration Notes
Topic	AVR (ATmega328P)	STM32 (F103C8T6)
GPIO write	PORTB |= (1<<PB3)	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET)
GPIO read	(PINB >> PB7) & 1	HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7)
ms delay	Custom Timer2 ISR	HAL_GetTick() (SysTick built-in)
µs delay	_delay_us()	DWT cycle counter (delay_us())
ADC	10-bit ADC register	12-bit HAL_ADC_GetValue()
ADC thresholds	GAS_ALARM = 600	GAS_ALARM = 2400 (×4 scale)
UART TX	UDR0 = byte	HAL_UART_Transmit()
PWM duty	0–255 (OCR0B)	0–999 (TIM3 CCR1)
Interrupt safety	cli/sei	Not needed (HAL_GetTick is atomic)
Fan PWM timer	Timer0 OC0B	TIM3 CH1 (PA6)
Clock init	Fuse bits (external)	SystemClock_Config() in code
Flash tool	avrdude	OpenOCD + ST-Link V2
🛠️ Troubleshooting
Symptom	Likely Cause	Fix
LCD shows nothing	V0 (contrast) not set	Adjust potentiometer on LCD
LCD shows blocks only	Initialization failed	Check 5V on LCD VDD/A pins; verify PB8–PB13 wiring
DHT11 read errors	Timing / noise	Add 4.7 kΩ pull-up from PB6 to 3.3V; keep wire short (<20 cm)
Fan not spinning	MOSFET gate not driven	Check PA6 → MOSFET gate connection; verify make flash succeeded
Relay clicks randomly	Floating PIR / LDR	PIR needs 5V supply; LDR divider needs correct resistor
No UART output	Wrong baud rate	Set terminal to 115200 baud, 8N1
Bluetooth not pairing	HC-05 not in AT mode	Default PIN is 1234; ensure HC-05 RX connected to PA9 via 3.3V divider
Build fails: HAL not found	Missing Drivers/ folder	Download STM32CubeF1 HAL (see 
Building
)
OpenOCD: no device found	ST-Link not detected	Check USB; install ST-Link drivers (from ST website)
STM32 gets hot	5V on 3.3V VDD pin	Remove 5V immediately — use AMS1117-3.3 or ST-Link 3.3V only
📊 Resource Usage (Estimated)
Resource	Used	Available	%
Flash	~18 KB	64 KB	~28%
RAM	~1.3 KB	20 KB	~7%
GPIO pins	20	37	—
Timers	TIM3 (PWM)	TIM1,2,3,4	—
UART	USART1	1,2,3	—
ADC channels	2	10	—
 
