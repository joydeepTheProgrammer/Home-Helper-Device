
                                                               🏠 Home Helper Device — STM32 Edition

A Fully Autonomous Smart Home Automation & Safety System
Powered by the STM32F103C8T6 (Blue Pill) using ARM Cortex-M3, STM32 HAL, and Embedded C11.
Designed for real-time environmental monitoring, appliance automation, and safety protection without requiring cloud connectivity. 


📖 Overview
Home Helper Device is an intelligent embedded system that continuously monitors temperature, humidity, gas leakage, ambient light, and human presence to automatically control household appliances and trigger safety responses when hazardous conditions are detected.

All processing occurs locally on the STM32 microcontroller, ensuring:

No internet dependency

Fast response time

High reliability

Enhanced privacy

Low power consumption

The system integrates multiple sensors and actuators into a single platform capable of operating autonomously while also supporting Bluetooth-based remote control. 


✨ Key Features
Environmental Monitoring
Real-time temperature and humidity measurement using DHT11

Gas and smoke detection using MQ-2

Ambient light sensing using LDR

Occupancy detection using PIR motion sensor

Smart Automation
Automatic fan speed control based on temperature

Automatic lighting based on motion and room brightness

Appliance control through relay outputs

Manual override functionality

Safety Protection
Multi-level warning and alarm system

Gas leak detection with emergency response

Over-temperature protection

Automatic relay shutdown during hazardous conditions

Audible and visual alerts

User Interface
16×2 LCD with multi-page menu system

Three-button navigation interface

Bluetooth control through HC-05

Serial debugging interface

Embedded Design
Bare-metal STM32 HAL implementation

No RTOS required

Lightweight memory footprint

Fully offline operation

📊 System Specifications
Parameter	Value
MCU	STM32F103C8T6
Core	ARM Cortex-M3
Clock Frequency	72 MHz
Flash Memory	64 KB
SRAM	20 KB
Programming Language	Embedded C11
Framework	STM32 HAL
ADC Resolution	12-bit
UART Baud Rate	115200
Fan PWM Frequency	1 kHz
🏗 System Architecture
The firmware operates as an event-driven state machine with four operating modes:

NORMAL State
Safe environmental conditions

Green status LED

Automatic appliance control enabled

WARNING State
Triggered when:

Gas concentration exceeds warning threshold

Temperature exceeds 35°C

Humidity exceeds 70%

Actions:

Yellow LED indication

Periodic buzzer warning

User notification

ALARM State
Triggered when:

Gas concentration reaches alarm level

Temperature exceeds 40°C

Actions:

Red LED indication

Continuous buzzer activation

Relay shutdown

Fan forced to maximum speed

OVERRIDE State
Activated manually through the SELECT button or Bluetooth commands.

Actions:

Manual appliance control

Blue LED indication

Automation temporarily disabled

🔧 Hardware Components
Core Controller
Component	Quantity
STM32F103C8T6 Blue Pill	1
Sensors
Component	Purpose
DHT11	Temperature & Humidity
MQ-2	Gas & Smoke Detection
HC-SR501 PIR	Motion Detection
LDR	Ambient Light Detection
Output Devices
Component	Purpose
16×2 LCD	User Interface
DC Fan	Cooling Control
Relay Module	Appliance Switching
Passive Buzzer	Audible Alerts
RGB LED	Status Indication
Communication
Component	Purpose
HC-05 Bluetooth Module	Wireless Control
ST-Link V2	Programming & Debugging
📌 GPIO Mapping
STM32 Pin	Function
PA0	MQ-2 Gas Sensor
PA1	LDR Sensor
PA6	Fan PWM Output
PA9	UART TX
PA10	UART RX
PB0	RGB LED Red
PB1	RGB LED Green
PB2	RGB LED Blue
PB3	Relay Control
PB4	Buzzer
PB6	DHT11 Data
PB7	PIR Motion Input
PB8-PB13	LCD Interface
PC13	UP Button
PC14	DOWN Button
PC15	SELECT Button
⚙️ Control Algorithms
Automatic Fan Control
Temperature	Fan Speed
Below 30°C	0%
30–35°C	Linear PWM Scaling
Above 35°C	100%
Automatic Lighting
Light is enabled only when:

Room is Dark
AND
Motion is Detected
Safety Interlock
The relay is automatically disabled when:

Gas Alarm Active
OR
Temperature ≥ 40°C
📱 Bluetooth Commands
Command	Action
R	Relay ON
r	Relay OFF
A	Auto Mode
F	Fan 100%
f	Fan OFF
S	Status Report
?	Help Menu
Example
=== HOME HELPER STM32 STATUS ===

Temp      : 28°C
Humidity  : 55%
Gas ADC   : 420
LDR ADC   : 980
Motion    : NO
Relay     : OFF
Fan Duty  : 0%
State     : NORMAL
🖥 LCD User Interface
Page 0 – Dashboard
Displays:

System status

Uptime

Current operating mode

Page 1 – Environment
Displays:

Temperature

Humidity

Fan speed

Relay status

Page 2 – Gas Monitoring
Displays:

MQ-2 ADC reading

Safety level

Page 3 – Lighting
Displays:

LDR value

Motion detection status

Page 4 – Settings
Displays:

Relay operating mode

Override status

📁 Block diagram
<img width="1536" height="1024" alt="image" src="https://github.com/user-attachments/assets/37546b86-327b-4040-bdb8-206f1a5717a0" />

🚀 Building the Project
Requirements
ARM GNU Embedded Toolchain

STM32CubeF1 HAL Drivers

GNU Make

OpenOCD

Build
make
Generated files:

build/home_helper_stm32.elf
build/home_helper_stm32.hex
Memory Usage
Resource	Usage
Flash	~18 KB
RAM	~1.3 KB
⚡ Flashing the Firmware
ST-Link Wiring
ST-Link      STM32
------------------------
SWDIO    ->  PA13
SWCLK    ->  PA14
GND      ->  GND
3.3V     ->  3.3V
Flash
make flash
🔄 AVR to STM32 Migration Highlights
AVR	STM32 HAL
PORTB	= (1<<PB3)
ADC 10-bit	ADC 12-bit
OCR0 PWM	TIM3 PWM
_delay_us()	DWT Delay
UDR0	HAL_UART_Transmit()
avrdude	OpenOCD
🛠 Troubleshooting
LCD Blank
Verify LCD power supply

Adjust contrast potentiometer

Check LCD data wiring

DHT11 Errors
Add 4.7kΩ pull-up resistor

Keep wiring short

Ensure 3.3V operation

Fan Not Running
Verify MOSFET gate connection

Check PWM output on PA6

Bluetooth Not Pairing
Confirm HC-05 power supply

Default PIN: 1234

Verify UART connections

No Serial Output
Baud rate must be 115200

Configure terminal as 8N1

📈 Resource Utilization
Resource	Used	Available
Flash	~18 KB	64 KB
RAM	~1.3 KB	20 KB
GPIO	20	37
UART	USART1	USART1/2/3
ADC Channels	2	10
Timers	TIM3	TIM1–TIM4
🔮 Future Enhancements
Wi-Fi Connectivity (ESP8266/ESP32)

Mobile Application

MQTT Integration

Data Logging to SD Card

OTA Firmware Updates

TFT Graphical Interface

Voice Assistant Support

Energy Consumption Monitoring
