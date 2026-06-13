/**
 * @file    config.h
 * @brief   Home Helper Device — STM32F103C8T6 Configuration
 * @target  STM32F103C8T6 "Blue Pill"
 * @toolchain arm-none-eabi-gcc + STM32 HAL
 * @version 2.0.0
 *
 * CLOCK:
 *   HSE = 8 MHz  →  PLL ×9  →  SYSCLK = 72 MHz
 *   AHB = 72 MHz, APB1 = 36 MHz, APB2 = 72 MHz
 *
 * PIN MAPPING:
 * ─────────────────────────────────────────────────────────────────
 *  PA0  → ADC1_IN0   — MQ-2  Gas Sensor (analog)
 *  PA1  → ADC1_IN1   — LDR   Light Sensor (analog)
 *  PA6  → TIM3_CH1   — FAN   PWM output
 *  PA9  → USART1_TX  — Debug / HC-05 Bluetooth TX
 *  PA10 → USART1_RX  — Debug / HC-05 Bluetooth RX
 *  PB0  → GPIO_OUT   — RGB LED Red
 *  PB1  → GPIO_OUT   — RGB LED Green
 *  PB2  → GPIO_OUT   — RGB LED Blue
 *  PB3  → GPIO_OUT   — RELAY  (Active LOW)
 *  PB4  → GPIO_OUT   — BUZZER
 *  PB6  → GPIO_IO    — DHT11  Data (1-wire, open-drain)
 *  PB7  → GPIO_IN    — PIR    Motion Sensor
 *  PB8  → GPIO_OUT   — LCD RS
 *  PB9  → GPIO_OUT   — LCD E
 *  PB10 → GPIO_OUT   — LCD D4
 *  PB11 → GPIO_OUT   — LCD D5
 *  PB12 → GPIO_OUT   — LCD D6
 *  PB13 → GPIO_OUT   — LCD D7
 *  PC13 → GPIO_IN    — Button UP   (pull-up, active-low)
 *  PC14 → GPIO_IN    — Button DOWN (pull-up, active-low)
 *  PC15 → GPIO_IN    — Button SEL  (pull-up, active-low)
 * ─────────────────────────────────────────────────────────────────
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ─────────────────────────── UART ──────────────────────────────── */
#define UART_BAUD_RATE       115200U

/* ─────────────────────────── TIM3 PWM (FAN) ────────────────────── */
/* TIM3_CH1 on PA6, APB1 Timer clock = 72 MHz (with ×2 multiplier)  */
/* ARR = 999 → PWM freq = 72,000,000 / (72 × 1000) = 1 kHz          */
#define FAN_TIM_PRESCALER    71U     /* Divide 72 MHz → 1 MHz        */
#define FAN_TIM_PERIOD       999U    /* 1 MHz / 1000 = 1 kHz PWM     */
/* CCR1 = 0 (off) … 999 (100% duty) */
#define FAN_DUTY(pct)        ((uint32_t)((pct) * (FAN_TIM_PERIOD + 1U) / 100U))
#define FAN_OFF_DUTY         0U
#define FAN_MAX_DUTY         FAN_TIM_PERIOD

/* ─────────────────────────── GPIO HELPERS ──────────────────────── */
/* Relay — Active LOW */
#define RELAY_GPIO_PORT      GPIOB
#define RELAY_GPIO_PIN       GPIO_PIN_3
#define RELAY_ON()           HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, GPIO_PIN_RESET)
#define RELAY_OFF()          HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, GPIO_PIN_SET)
#define RELAY_STATE()        (HAL_GPIO_ReadPin(RELAY_GPIO_PORT, RELAY_GPIO_PIN) == GPIO_PIN_RESET)

/* Buzzer */
#define BUZZER_GPIO_PORT     GPIOB
#define BUZZER_GPIO_PIN      GPIO_PIN_4
#define BUZZER_ON()          HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_SET)
#define BUZZER_OFF()         HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET)

/* PIR Motion */
#define PIR_GPIO_PORT        GPIOB
#define PIR_GPIO_PIN         GPIO_PIN_7
#define PIR_READ()           (HAL_GPIO_ReadPin(PIR_GPIO_PORT, PIR_GPIO_PIN) == GPIO_PIN_SET)

/* DHT11 */
#define DHT11_GPIO_PORT      GPIOB
#define DHT11_GPIO_PIN       GPIO_PIN_6

/* RGB LED */
#define LED_R_PORT           GPIOB
#define LED_R_PIN            GPIO_PIN_0
#define LED_G_PORT           GPIOB
#define LED_G_PIN            GPIO_PIN_1
#define LED_B_PORT           GPIOB
#define LED_B_PIN            GPIO_PIN_2

#define LED_RED()    do { HAL_GPIO_WritePin(LED_R_PORT,LED_R_PIN,GPIO_PIN_SET);   \
                          HAL_GPIO_WritePin(LED_G_PORT,LED_G_PIN,GPIO_PIN_RESET); \
                          HAL_GPIO_WritePin(LED_B_PORT,LED_B_PIN,GPIO_PIN_RESET); } while(0)
#define LED_GREEN()  do { HAL_GPIO_WritePin(LED_R_PORT,LED_R_PIN,GPIO_PIN_RESET); \
                          HAL_GPIO_WritePin(LED_G_PORT,LED_G_PIN,GPIO_PIN_SET);   \
                          HAL_GPIO_WritePin(LED_B_PORT,LED_B_PIN,GPIO_PIN_RESET); } while(0)
#define LED_BLUE()   do { HAL_GPIO_WritePin(LED_R_PORT,LED_R_PIN,GPIO_PIN_RESET); \
                          HAL_GPIO_WritePin(LED_G_PORT,LED_G_PIN,GPIO_PIN_RESET); \
                          HAL_GPIO_WritePin(LED_B_PORT,LED_B_PIN,GPIO_PIN_SET);   } while(0)
#define LED_YELLOW() do { HAL_GPIO_WritePin(LED_R_PORT,LED_R_PIN,GPIO_PIN_SET);   \
                          HAL_GPIO_WritePin(LED_G_PORT,LED_G_PIN,GPIO_PIN_SET);   \
                          HAL_GPIO_WritePin(LED_B_PORT,LED_B_PIN,GPIO_PIN_RESET); } while(0)
#define LED_OFF_ALL() do { HAL_GPIO_WritePin(LED_R_PORT,LED_R_PIN,GPIO_PIN_RESET); \
                           HAL_GPIO_WritePin(LED_G_PORT,LED_G_PIN,GPIO_PIN_RESET); \
                           HAL_GPIO_WritePin(LED_B_PORT,LED_B_PIN,GPIO_PIN_RESET); } while(0)

/* Buttons (active LOW, internal pull-up) */
#define BTN_GPIO_PORT        GPIOC
#define BTN_UP_PIN           GPIO_PIN_13
#define BTN_DOWN_PIN         GPIO_PIN_14
#define BTN_SEL_PIN          GPIO_PIN_15
#define BTN_UP_PRESSED()     (HAL_GPIO_ReadPin(BTN_GPIO_PORT, BTN_UP_PIN)   == GPIO_PIN_RESET)
#define BTN_DOWN_PRESSED()   (HAL_GPIO_ReadPin(BTN_GPIO_PORT, BTN_DOWN_PIN) == GPIO_PIN_RESET)
#define BTN_SEL_PRESSED()    (HAL_GPIO_ReadPin(BTN_GPIO_PORT, BTN_SEL_PIN)  == GPIO_PIN_RESET)

/* ─────────────────────────── ADC CHANNELS ──────────────────────── */
/* 12-bit ADC (0–4095), VREF = 3.3 V                                 */
#define ADC_CH_GAS           ADC_CHANNEL_0    /* PA0 — MQ-2          */
#define ADC_CH_LDR           ADC_CHANNEL_1    /* PA1 — LDR           */

/* ─────────────────────────── THRESHOLDS ───────────────────────── */
/* Temperature (DHT11 output is uint8_t °C — same as AVR version)    */
#define TEMP_WARN_C          30U
#define TEMP_HIGH_C          35U
#define TEMP_CRITICAL_C      40U
#define HUMIDITY_HIGH        70U

/* Gas — scaled to 12-bit ADC (4× the 10-bit AVR values)            */
#define GAS_WARN_ADC         1200U   /* ~300 × 4 */
#define GAS_ALARM_ADC        2400U   /* ~600 × 4 */

/* LDR — scaled to 12-bit                                            */
#define LDR_DARK_THRESHOLD   1600U   /* ~400 × 4 */

/* ─────────────────────────── TIMING (ms) ──────────────────────── */
#define DHT11_READ_INTERVAL_MS    2000U
#define SENSOR_POLL_INTERVAL_MS   500U
#define DISPLAY_REFRESH_MS        1000U
#define DEBOUNCE_MS               50U

/* ─────────────────────────── SYSTEM STATES ────────────────────── */
typedef enum {
    STATE_NORMAL   = 0,
    STATE_WARNING  = 1,
    STATE_ALARM    = 2,
    STATE_OVERRIDE = 3
} SystemState_t;

typedef enum {
    PAGE_HOME      = 0,
    PAGE_TEMP_HUM  = 1,
    PAGE_GAS       = 2,
    PAGE_LIGHT     = 3,
    PAGE_SETTINGS  = 4,
    PAGE_COUNT     = 5
} DisplayPage_t;

/* ─────────────────────────── EXTERN HANDLES ────────────────────── */
/* Defined in main.c, used across modules                             */
extern UART_HandleTypeDef  huart1;
extern TIM_HandleTypeDef   htim3;
extern ADC_HandleTypeDef   hadc1;

#endif /* CONFIG_H */
