/**
 * @file    lcd.h
 * @brief   HD44780 16×2 LCD Driver — 4-bit mode for STM32 HAL
 *
 * Pin wiring:
 *   RS → PB8    E  → PB9
 *   D4 → PB10  D5 → PB11
 *   D6 → PB12  D7 → PB13
 *   R/W tied to GND (write-only)
 *   VSS=GND, VDD=5V, V0=potentiometer, A=5V via 220Ω, K=GND
 *
 * NOTE: LCD operates at 5V but accepts 3.3V logic for control lines
 *       (most HD44780 compatible displays). For strict 5V logic,
 *       use a level shifter on RS, E, D4–D7.
 */
#ifndef LCD_H
#define LCD_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ── LCD GPIO Port (all control + data pins on GPIOB) ─────────── */
#define LCD_GPIO_PORT   GPIOB
#define LCD_RS_PIN      GPIO_PIN_8
#define LCD_E_PIN       GPIO_PIN_9
#define LCD_D4_PIN      GPIO_PIN_10
#define LCD_D5_PIN      GPIO_PIN_11
#define LCD_D6_PIN      GPIO_PIN_12
#define LCD_D7_PIN      GPIO_PIN_13

/* ── HD44780 Commands ──────────────────────────────────────────── */
#define LCD_CMD_CLEAR       0x01U
#define LCD_CMD_HOME        0x02U
#define LCD_CMD_ENTRY       0x06U
#define LCD_CMD_DISP_ON     0x0CU
#define LCD_CMD_FUNC_SET    0x28U   /* 4-bit, 2-line, 5×8 */
#define LCD_ROW0_ADDR       0x80U
#define LCD_ROW1_ADDR       0xC0U

/* ── API ───────────────────────────────────────────────────────── */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_PutChar(char c);
void LCD_PutStr(const char *str);
void LCD_PutUInt(uint16_t val);
void LCD_PutInt(int16_t val);
void LCD_PrintRow(uint8_t row, const char *str);   /* Pad to 16 chars */

#endif /* LCD_H */
