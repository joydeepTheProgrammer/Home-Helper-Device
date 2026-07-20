# 🏠 Home Helper Device

<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F103C8T6-03234B?logo=stmicroelectronics&logoColor=white" />
  <img src="https://img.shields.io/badge/Core-ARM%20Cortex--M3-0091BD?logo=arm&logoColor=white" />
  <img src="https://img.shields.io/badge/Language-Embedded%20C11-A8B9CC?logo=c&logoColor=black" />
  <img src="https://img.shields.io/badge/HAL-STM32F1xx-blue" />
  <img src="https://img.shields.io/badge/Clock-72%20MHz-brightgreen" />
  <img src="https://img.shields.io/badge/License-MIT-green" />
</p>

<p align="center">
  <b>A fully autonomous smart home automation &amp; safety system — no cloud, no dependency, no compromise.</b><br/>
  Runs entirely on the <b>STM32F103C8T6 Blue Pill</b>, monitoring your environment in real-time and reacting in under 500 ms.
</p>

---

## 📖 What It Does

Home Helper Device is a standalone embedded firmware that transforms a Blue Pill dev board into a smart home controller. It reads multiple sensors every 500 ms, drives actuators intelligently, and keeps the user informed through a 16×2 LCD and a UART/Bluetooth serial interface — all without any internet connection.

```
Sensors ──► STM32F103C8T6 ──► Actuators
  DHT11  (Temp/Hum)              DC Fan   (PWM speed)
  MQ-2   (Gas/Smoke)    ────►   Relay    (Light/appliance)
  LDR    (Light level)           Buzzer   (Alarm tones)
  PIR    (Motion)                RGB LED  (Status colour)
                                 LCD 16×2 (5-page UI)
                         ◄────  HC-05    (Bluetooth control)
```

---

## ✨ Feature Highlights

| Category | Features |
|---|---|
| 🌡️ **Sensing** | Temperature & humidity (DHT11) · Gas/smoke (MQ-2 · 12-bit ADC) · Ambient light (LDR) · PIR motion |
| 💡 **Automation** | Fan PWM speed from temperature · Light relay when dark + motion · Manual override mode |
| 🚨 **Safety** | 3-level FSM alarm: NORMAL → WARNING → ALARM · Gas-triggered relay cut-off · Over-temp protection |
| 📱 **Connectivity** | HC-05 Bluetooth commands over USART1 · Serial debug at 115200 baud |
| 🖥️ **UI** | HD44780 16×2 LCD · 5-page menu · 3-button navigation (UP/DOWN/SELECT) · 50 ms debounce |
| ⚡ **Efficiency** | Non-blocking main loop · DWT µs delay (no timer wasted) · `--specs=nano.specs` binary |

---

## Circuit Diagram

<img width="1024" height="1024" alt="circuit-full" src="https://github.com/user-attachments/assets/ae76bbcd-a5c4-4e23-ac50-8401c024d284" />


<img width="1024" height="1024" alt="circuit-drivers" src="https://github.com/user-attachments/assets/b5a60b26-e7df-4974-abb8-6642212e31b5" />


---

## 🏗️ System Architecture

### Finite State Machine

```
┌──────────────────────────────────────────────────────────────────────────────────────────────┐
│                                                                                              │
│      Gas < 1200 AND Temp < 35°C AND Hum < 70%                                                │
│             │                                                                                │
│             ▼                                                                                │
│      ┌──────────────┐                                                                        │
│      │    NORMAL    │────────────── Gas ≥ 1200 OR Temp ≥ 35°C OR Hum ≥ 70% ───────────┐      │
│      └──────┬───────┘                                                                 │      │
│             ▲                                                                         ▼      │
│             │                                                                 ┌────────────┐ │
│             │                                                                 │  WARNING   │ │ 
│             │◄──────── Gas < 1200 AND Temp < 35°C AND Hum < 70% ─────────────┤|            │ │
│             │                                                                 └─────┬──────┘ │
│             │                                                                       │        │
│             │                                            Gas ≥ 2400 OR Temp ≥ 40°C │         │ 
│             │                                                                       ▼        │
│             │                                                                ┌────────────┐  │
│             │                                                                │   ALARM    │  │
│             │                                                                │ Fan=100%   │  │
│             │                                                                │ Relay=OFF  │  │
│             │                                                                │ Buzzer=ON  │  │
│             │                                                                └─────┬──────┘  │
│             │                                                                      │         │
│             │                     Gas < 2400 AND Temp < 40°C                       │         │ 
│             └──────────────────────────────────────────────────────────────────────┘         │
│                                                                                              │
│                                                                                              │
│      SELECT Button / Bluetooth Command                                                       │
│                    │                                                                         │
│                    ▼                                                                         │
│             ┌────────────────┐                                                               │
│             │    OVERRIDE    │                                                               │
│             │  (No Alarms)   │                                                               │
│             └───────┬────────┘                                                               │
│                     │                                                                        │
│      SELECT Button / Bluetooth Command                                                       │
│                     ▼                                                                        │
│                  NORMAL                                                                      │
│                                                                                              │
└──────────────────────────────────────────────────────────────────────────────────────────────┘s
```

### Main Loop (Non-Blocking)

```
for(;;)
 ├─ handle_bluetooth()          Every iteration — poll USART1 RXNE flag
 ├─ handle_buttons()            Every iteration — 50 ms debounce gate
 ├─ [every 2000 ms]  DHT11_Read()
 ├─ [every  500 ms]  ADC_ReadAvg(GAS, 8 samples)
 │                   ADC_ReadAvg(LDR, 4 samples)
 │                   PIR_READ()
 │                   control_fan()   → TIM3 CCR1
 │                   control_relay() → PB3 active-LOW
 │                   control_buzzer()→ PB4
 │                   update_led_state() → PB0/1/2
 ├─ [every 1000 ms]  display_update()  → LCD 5-page
 └─ [every 1000 ms]  uptime_s++
```

---

## ⚙️ Control Logic

### Fan Speed (TIM3 CH1 · PA6 · 1 kHz PWM · ARR = 999)

| Temperature | Fan Speed | CCR Value |
|---|---|---|
| < 30°C (`TEMP_WARN_C`) | 0% — Off | 0 |
| 30°C – 34°C | Linear interpolation | `(temp − 30) × 999 / 5` |
| ≥ 35°C (`TEMP_HIGH_C`) | 100% — Full speed | 999 |
| Gas ≥ 2400 (ALARM) | 100% — Forced ventilation | 999 |

### Relay / Light Automation

```
IF   gas_adc ≥ 2400   →  Relay OFF  (hard override — gas safety)
ELIF temp    ≥ 40°C   →  Relay OFF  (hard override — thermal safety)
ELIF dark AND motion  →  Relay ON   (auto light)
ELIF bright           →  Relay OFF  (daytime)
ELSE                  →  Hold current state  (dark + no motion)
```
> **Note:** Relay is **Active LOW** — `GPIO_PIN_RESET` = coil energised.

### Buzzer Behaviour

| Condition | Buzzer |
|---|---|
| Gas ≥ 2400 OR Temp ≥ 40°C | Continuous ON |
| Gas ≥ 1200 OR Temp ≥ 35°C | Intermittent (toggles every 500 ms poll) |
| All clear | OFF |

### RGB LED Colour Map

| State | Colour | RGB Pins |
|---|---|---|
| NORMAL | 🟢 Green | PB1 = HIGH |
| WARNING | 🟡 Yellow | PB0 + PB1 = HIGH |
| ALARM | 🔴 Red | PB0 = HIGH |
| OVERRIDE | 🔵 Blue | PB2 = HIGH |
| Error_Handler | 🔴 Red + Buzzer | PB0 = HIGH, PB4 = HIGH |

---

## 📌 GPIO Pin Map

| Pin | Peripheral Mode | Signal | Notes |
|---|---|---|---|
| **PA0** | ADC1_IN0 · Analog | MQ-2 Gas Sensor | 12-bit · 0–4095 |
| **PA1** | ADC1_IN1 · Analog | LDR Light Sensor | 12-bit · 0–4095 |
| **PA6** | TIM3_CH1 · AF_PP | Fan PWM | 1 kHz · duty 0–999 |
| **PA9** | USART1_TX · AF_PP | Debug / HC-05 TX | 115200 8N1 |
| **PA10** | USART1_RX · Input | Debug / HC-05 RX | 115200 8N1 |
| **PB0** | GPIO Output PP | RGB LED Red | |
| **PB1** | GPIO Output PP | RGB LED Green | |
| **PB2** | GPIO Output PP | RGB LED Blue | |
| **PB3** | GPIO Output PP | Relay Control | **Active LOW** |
| **PB4** | GPIO Output PP | Buzzer | Active HIGH |
| **PB6** | GPIO Output OD | DHT11 Data | 1-wire · open-drain |
| **PB7** | GPIO Input (no pull) | PIR Motion | External pull handled by module |
| **PB8** | GPIO Output PP | LCD RS | |
| **PB9** | GPIO Output PP | LCD Enable | |
| **PB10** | GPIO Output PP | LCD D4 | 4-bit mode |
| **PB11** | GPIO Output PP | LCD D5 | 4-bit mode |
| **PB12** | GPIO Output PP | LCD D6 | 4-bit mode |
| **PB13** | GPIO Output PP | LCD D7 | 4-bit mode |
| **PC13** | GPIO Input Pull-Up | Button UP | Active LOW |
| **PC14** | GPIO Input Pull-Up | Button DOWN | Active LOW |
| **PC15** | GPIO Input Pull-Up | Button SELECT | Active LOW |

---

## 🖥️ LCD Menu Pages

Navigate with **UP** / **DOWN**. Press **SELECT** on any page to cycle the override.

| Page | Key | Row 0 (chars 1–16) | Row 1 (chars 1–16) |
|---|---|---|---|
| 0 · Home | `HOME` | `Home Helper STM32` | `Up:HH:MM:SS STA` ¹ |
| 1 · Temp & Hum | `TEMP` | `T:25°C  H:60%  ` | `Fan:050% RLY:ON` |
| 2 · Gas Sensor | `GAS` | `Gas/Smoke Sensor` | `Val:1250 WARN!  ` ² |
| 3 · Light & Motion | `LGT` | `Light & Motion  ` | `LDR:1420 MOT:Y ` |
| 4 · Settings | `SET` | `Settings/Overrid` ³ | `RLY:AUTO        ` |

> ¹ STA = `OK ` / `WRN` / `ALM`  
> ² Status = `Safe   ` / `WARN!  ` / `ALARM!!`  
> ³ The 17-char literal is clipped to 16 by the LCD driver — displays as `Settings/Overrid`

---

## 📱 Bluetooth Commands

Connect any serial terminal or Bluetooth app to **HC-05** at **115200 baud** and send single ASCII characters:

| Char | Action | UART Response |
|---|---|---|
| `R` | Relay ON · enter override mode | `[BT] Relay ON (override)` |
| `r` | Relay OFF · enter override mode | `[BT] Relay OFF (override)` |
| `A` | Exit override · return to auto | `[BT] Auto mode restored` |
| `F` | Fan to 100% immediately | `[BT] Fan MAX` |
| `f` | Fan OFF immediately | `[BT] Fan OFF` |
| `S` / `s` | Print full status report | See below |
| `?` | Print command help | Two-line help text |

### Status Report (`S` command)

```
=== HOME HELPER STM32 STATUS ===
Temp: 27C
Humidity: 55%
Gas ADC: 843
LDR ADC: 1200
Motion: YES
Relay: ON
Fan Duty: 399
State: NORMAL
================================
```

---

## 📊 Threshold Reference

All constants are in [`config.h`](config.h):

| Constant | Value | Meaning |
|---|---|---|
| `TEMP_WARN_C` | 30 °C | Fan starts; WARNING if humidity also high |
| `TEMP_HIGH_C` | 35 °C | Fan at 100%; triggers WARNING state |
| `TEMP_CRITICAL_C` | 40 °C | ALARM state; relay forced OFF |
| `HUMIDITY_HIGH` | 70 % | Triggers WARNING state |
| `GAS_WARN_ADC` | 1200 | ~1.0 V on 3.3 V ref → WARNING state |
| `GAS_ALARM_ADC` | 2400 | ~1.9 V on 3.3 V ref → ALARM state |
| `LDR_DARK_THRESHOLD` | 1600 | Below this = dark → relay light automation |
| `DHT11_READ_INTERVAL_MS` | 2000 | DHT11 poll period |
| `SENSOR_POLL_INTERVAL_MS` | 500 | MQ-2, LDR, PIR poll period |
| `DISPLAY_REFRESH_MS` | 1000 | LCD refresh period |
| `DEBOUNCE_MS` | 50 | Button debounce window |

---

## 📦 Hardware Bill of Materials

| Ref | Component | Qty | Notes |
|---|---|---|---|
| U1 | STM32F103C8T6 Blue Pill | 1 | 72 MHz, 64 KB Flash, 20 KB SRAM |
| S1 | HC-SR501 PIR Sensor | 1 | Adjustable sensitivity & delay |
| U2 | DHT11 | 1 | ±2 °C / ±5% RH |
| U3 | MQ-2 Gas / Smoke Sensor | 1 | 5V supply, analog out to PA0 |
| U4 | HC-05 Bluetooth Module | 1 | Pre-paired to 115200 baud |
| LCD1 | 16×2 HD44780 LCD | 1 | 5V with 10kΩ contrast pot |
| RL1 | 5V Relay Module | 1 | Active LOW trigger |
| M1 | DC Fan (5V or 12V) | 1 | Driven via MOSFET |
| BZ1 | Active Buzzer (5V) | 1 | Direct GPIO or transistor driver |
| LED1 | RGB LED (common cathode) | 1 | 330Ω series on each channel |
| R1–R3 | 330 Ω resistors | 3 | RGB LED current limit (~10 mA each) |
| R4 | 10 kΩ resistor | 1 | LDR voltage divider |
| J1 | ST-Link V2 | 1 | SWD programming & GDB debug |

---

## 🗂️ Source File Layout

```
Home-Helper-Device/
├── main.c          HAL init · FSM · main loop · all MX_xxx_Init() functions
├── main.h          Error_Handler() forward declaration
├── config.h        Pin defs · thresholds · timing · SystemState_t · DisplayPage_t
│                   extern handles: huart1, htim3, hadc1
├── dht11.c/.h      DHT11 1-wire driver (open-drain + GPIO mode reconfigure)
├── lcd.c/.h        HD44780 4-bit LCD driver (PB8–PB13) + LCD_PrintRow()
├── uart_comm.c/.h  USART1 TX/RX helpers · boot banner · BT command RX
├── adc_sensor.c/.h ADC1 single-channel read + averaged read (1–16 samples)
├── delay_us.c/.h   DWT CYCCNT µs busy-wait + delay_ms() → HAL_Delay()
└── Makefile        arm-none-eabi-gcc · flash via OpenOCD/ST-Link · GDB debug
```

---

## 🛠️ Clock & Peripheral Configuration

```
HSE  8 MHz
  └─► PLL ×9 ──► SYSCLK = 72 MHz
                   ├─ AHB  DIV1 = 72 MHz  (GPIO, DMA, DWT)
                   ├─ APB1 DIV2 = 36 MHz  → Timer clock = 72 MHz (×2 rule)
                   │    └─ TIM3  PSC=71, ARR=999 → 1 kHz PWM on PA6
                   └─ APB2 DIV1 = 72 MHz
                        ├─ USART1  → 115200 baud
                        └─ ADC1    DIV6 → 12 MHz ADC clock (≤14 MHz spec)
```

**ADC calibration** (`HAL_ADCEx_Calibration_Start`) runs once after power-on to compensate internal offset.

---

## 🔨 Building & Flashing

### Prerequisites

```bash
arm-none-eabi-gcc --version   # GCC 10+ recommended
openocd --version              # For ST-Link flash & GDB
```

### Makefile Targets

```bash
make              # Compile → build/home_helper_stm32.elf + .hex
make flash        # Flash .bin via ST-Link + OpenOCD
make size         # Print flash/SRAM usage (berkeley format)
make debug        # Launch OpenOCD + arm-none-eabi-gdb session
make clean        # Remove build/ directory
```

> **📁 Project Layout Note:** The Makefile targets the STM32CubeIDE directory structure (`Core/Src/*.c`). If using the flat repository layout (all `.c` in root), change one line in the Makefile:
> ```makefile
> APP_SRCS = $(wildcard *.c)   # flat layout
> ```
> You will also need `stm32f1xx_it.c` (provides `SysTick_Handler → HAL_IncTick`) and the linker script `STM32F103C8TX_FLASH.ld` — both generated by STM32CubeIDE.

### Flashing with STM32CubeIDE (Recommended)

1. **Import** this project into STM32CubeIDE (File → Import → Existing Project).
2. Ensure `Drivers/STM32F1xx_HAL_Driver/` is present.
3. **Build** (Ctrl+B) → **Run** (F11) — ST-Link flashes automatically.

### Manual OpenOCD

```bash
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg \
  -c "program build/home_helper_stm32.bin verify reset exit 0x08000000"
```

---

## 📊 System Specifications

| Parameter | Value |
|------------|--------|
| MCU | STM32F103C8T6 |
| Architecture | ARM Cortex-M3 |
| Clock | 72 MHz (HSE 8 MHz + PLL ×9) |
| Flash | 64 KB (some C8T6 batches report 128 KB) |
| SRAM | 20 KB |
| HAL | STM32F1xx HAL (STM32CubeF1) |
| Language | Embedded C11 (`-std=c11`) |
| Compiler flags | `-Os -Wall -Wextra -Wshadow -Wstrict-prototypes` |
| ADC Resolution | 12-bit (0–4095, VREF = 3.3 V) |
| UART Baud Rate | 115200 8N1 |
| PWM Frequency | 1 kHz (TIM3 CH1, ARR = 999) |
| Sensor poll | 500 ms (ADC/PIR) · 2000 ms (DHT11) |
| LCD refresh | 1000 ms |
| Button debounce | 50 ms |

---

## ⚠️ Known Limitations

- **DHT11 read is blocking** (~22 ms including start pulse). The main loop pauses during each DHT11 read every 2 seconds. For latency-critical applications, consider moving DHT11 reads to a low-priority interrupt or RTOS task.
- **USART1 is shared** between the HC-05 Bluetooth module and the serial debug port. They cannot be used independently at runtime.
- **No RTOS.** The firmware is a bare-metal cooperative loop. Tasks are time-sliced by the `timer_elapsed()` helper using `HAL_GetTick()`.
- **MQ-2 warm-up time.** The MQ-2 sensor requires ~60 seconds of preheat after power-on for stable readings.

---

## 📄 MIT License

Copyright (c) 2026 Joydeep Majumdar

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

---
