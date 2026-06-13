/**
 * @file    lcd.c
 * @brief   HD44780 16×2 LCD Driver — 4-bit mode for STM32 HAL
 *
 * All 6 control/data pins are on GPIOB (PB8–PB13).
 * Timing follows HD44780 datasheet requirements.
 */

#include "lcd.h"
#include "delay_us.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

/* ── Internal helpers ─────────────────────────────────────────────── */

static inline void lcd_rs_set(void)   { HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_RS_PIN, GPIO_PIN_SET);   }
static inline void lcd_rs_clear(void) { HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_RS_PIN, GPIO_PIN_RESET); }
static inline void lcd_e_set(void)    { HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_E_PIN,  GPIO_PIN_SET);   }
static inline void lcd_e_clear(void)  { HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_E_PIN,  GPIO_PIN_RESET); }

/** Pulse Enable pin to latch the current nibble */
static void lcd_pulse_enable(void) {
    lcd_e_set();
    delay_us(1);
    lcd_e_clear();
    delay_us(1);
}

/** Write lower 4 bits of 'nibble' to D4–D7 */
static void lcd_write_nibble(uint8_t nibble) {
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_D4_PIN,
        (nibble & 0x01U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_D5_PIN,
        (nibble & 0x02U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_D6_PIN,
        (nibble & 0x04U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_GPIO_PORT, LCD_D7_PIN,
        (nibble & 0x08U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    lcd_pulse_enable();
}

/**
 * @brief  Write a full byte (command or data) in two nibbles MSN first.
 * @param  byte     Byte to send
 * @param  is_data  true = data register, false = instruction register
 */
static void lcd_write_byte(uint8_t byte, int is_data) {
    if (is_data) lcd_rs_set();
    else          lcd_rs_clear();

    lcd_write_nibble(byte >> 4U);     /* Upper nibble */
    lcd_write_nibble(byte & 0x0FU);   /* Lower nibble */

    delay_us(50);  /* Most commands take ~37 µs */
}

/* ── Public API ───────────────────────────────────────────────────── */

void LCD_Init(void) {
    /* GPIO pins must already be configured as output push-pull by
     * MX_GPIO_Init() in main.c (PB8–PB13 OUTPUT PP, no pull)          */

    /* Ensure all pins start LOW */
    lcd_rs_clear();
    lcd_e_clear();
    HAL_GPIO_WritePin(LCD_GPIO_PORT,
        LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN,
        GPIO_PIN_RESET);

    delay_ms(50);   /* Power-on delay ≥ 40 ms */

    /* 3× 8-bit function-set reset (per HD44780 init sequence) */
    lcd_rs_clear();
    lcd_write_nibble(0x03U);
    delay_ms(5);
    lcd_write_nibble(0x03U);
    delay_ms(1);
    lcd_write_nibble(0x03U);
    delay_ms(1);

    /* Switch to 4-bit mode */
    lcd_write_nibble(0x02U);
    delay_ms(1);

    /* Full commands in 4-bit mode */
    lcd_write_byte(LCD_CMD_FUNC_SET, 0);   /* 4-bit, 2-line, 5×8     */
    lcd_write_byte(0x08U, 0);              /* Display OFF             */
    lcd_write_byte(LCD_CMD_CLEAR, 0);      /* Clear display           */
    delay_ms(2);
    lcd_write_byte(LCD_CMD_ENTRY, 0);      /* Entry mode: inc, no shift*/
    lcd_write_byte(LCD_CMD_DISP_ON, 0);    /* Display ON, cursor OFF  */
}

void LCD_Clear(void) {
    lcd_write_byte(LCD_CMD_CLEAR, 0);
    delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0U) ?
        (LCD_ROW0_ADDR + (col & 0x0FU)) :
        (LCD_ROW1_ADDR + (col & 0x0FU));
    lcd_write_byte(addr, 0);
}

void LCD_PutChar(char c) {
    lcd_write_byte((uint8_t)c, 1);
}

void LCD_PutStr(const char *str) {
    if (!str) return;
    while (*str) LCD_PutChar(*str++);
}

void LCD_PutUInt(uint16_t val) {
    char buf[6];
    uint8_t idx = 0;

    if (val == 0U) { LCD_PutChar('0'); return; }

    while (val > 0U) {
        buf[idx++] = (char)('0' + (val % 10U));
        val /= 10U;
    }
    while (idx > 0U) LCD_PutChar(buf[--idx]);
}

void LCD_PutInt(int16_t val) {
    if (val < 0) { LCD_PutChar('-'); val = -val; }
    LCD_PutUInt((uint16_t)val);
}

void LCD_PrintRow(uint8_t row, const char *str) {
    LCD_SetCursor(row, 0);
    uint8_t len = 0;
    while (str[len] && len < 16U) {
        LCD_PutChar(str[len++]);
    }
    while (len < 16U) { LCD_PutChar(' '); len++; }
}
