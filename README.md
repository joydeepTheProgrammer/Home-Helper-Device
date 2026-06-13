# 🏠 Home Helper Device — STM32 Edition

> **A Fully Autonomous Smart Home Automation & Safety System**
>
> Powered by the **STM32F103C8T6 (Blue Pill)** using **ARM Cortex-M3**, **STM32 HAL**, and **Embedded C11**.
>
> Designed for real-time environmental monitoring, intelligent automation, and household safety without cloud dependency.

---

## 📖 Overview

Home Helper Device is an intelligent embedded system that continuously monitors environmental conditions and automatically controls household appliances while providing advanced safety protection.

The system integrates multiple sensors and actuators into a single platform capable of:

- Monitoring temperature and humidity
- Detecting gas leaks and smoke
- Detecting room occupancy
- Measuring ambient light levels
- Controlling fans and lighting automatically
- Triggering alarms during hazardous conditions
- Supporting Bluetooth-based remote control

All processing is performed locally on the STM32 microcontroller, ensuring:

- No internet dependency
- Fast response times
- High reliability
- Enhanced privacy
- Low power consumption

---

## ✨ Features

### 🌡 Environmental Monitoring

- DHT11 temperature and humidity sensing
- MQ-2 gas and smoke detection
- Ambient light monitoring using LDR
- PIR-based motion detection

### 💡 Smart Automation

- Automatic fan speed control
- Intelligent lighting control
- Relay-based appliance switching
- Occupancy-aware automation
- Manual override mode

### 🚨 Safety Features

- Multi-level alarm system
- Gas leak detection
- Over-temperature protection
- Automatic emergency shutdown
- Audible and visual warnings

### 📱 Connectivity

- HC-05 Bluetooth remote control
- UART debugging interface
- Real-time status reporting

### 🖥 User Interface

- 16×2 LCD display
- Five-page menu system
- Three-button navigation
- Manual override controls

---

## 📊 System Specifications

| Parameter | Value |
|------------|--------|
| MCU | STM32F103C8T6 |
| Architecture | ARM Cortex-M3 |
| Clock Frequency | 72 MHz |
| Flash Memory | 64 KB |
| SRAM | 20 KB |
| Development Framework | STM32 HAL |
| Programming Language | Embedded C11 |
| ADC Resolution | 12-bit |
| UART Baud Rate | 115200 |
| Fan PWM Frequency | 1 kHz |

---

## 🏗 System Architecture

The firmware is implemented as a finite-state machine.

### 🟢 NORMAL State

Safe operating conditions.

**Actions**

- Green status LED
- Automatic control enabled
- Buzzer OFF
- Relay operates normally

---

### 🟡 WARNING State

Triggered when:

- Gas exceeds warning threshold
- Temperature exceeds 35°C
- Humidity exceeds 70%

**Actions**

- Yellow LED indication
- Periodic buzzer warning
- User notification

---

### 🔴 ALARM State

Triggered when:

- Gas exceeds alarm threshold
- Temperature exceeds 40°C

**Actions**

- Red LED indication
- Continuous buzzer activation
- Fan forced to 100%
- Relay forced OFF

---

### 🔵 OVERRIDE State

Activated manually through:

- SELECT button
- Bluetooth command

**Actions**

- Manual appliance control
- Automation disabled
- Blue LED indication

---

## 🔧 Hardware Components

### Core Controller

| Component | Quantity |
|------------|----------|
| STM32F103C8T6 Blue Pill | 1 |

---

### Sensors

| Component | Function |
|------------|----------|
| DHT11 | Temperature & Humidity |
| MQ-2 | Gas & Smoke Detection |
| HC-SR501 PIR | Motion Detection |
| LDR | Ambient Light Detection |

---

### Output Devices

| Component | Function |
|------------|----------|
| 16×2 LCD | User Interface |
| Relay Module | Appliance Control |
| DC Fan | Cooling System |
| Passive Buzzer | Alarm Notification |
| RGB LED | Status Indication |

---

### Communication

| Component | Function |
|------------|----------|
| HC-05 | Bluetooth Control |
| ST-Link V2 | Programming & Debugging |

---

## 📌 GPIO Mapping

| STM32 Pin | Function |
|------------|----------|
| PA0 | MQ-2 Gas Sensor |
| PA1 | LDR Sensor |
| PA6 | Fan PWM Output |
| PA9 | USART1 TX |
| PA10 | USART1 RX |
| PB0 | RGB LED Red |
| PB1 | RGB LED Green |
| PB2 | RGB LED Blue |
| PB3 | Relay Control |
| PB4 | Buzzer |
| PB6 | DHT11 Data |
| PB7 | PIR Motion Input |
| PB8 | LCD RS |
| PB9 | LCD Enable |
| PB10 | LCD D4 |
| PB11 | LCD D5 |
| PB12 | LCD D6 |
| PB13 | LCD D7 |
| PC13 | UP Button |
| PC14 | DOWN Button |
| PC15 | SELECT Button |

---

## ⚙️ Control Logic

### Fan Control

| Temperature | Fan Speed |
|-------------|------------|
| Below 30°C | 0% |
| 30°C – 35°C | Linear PWM Control |
| Above 35°C | 100% |

#### Formula

```text
Duty = (Temperature - 30) × 999 / 5
