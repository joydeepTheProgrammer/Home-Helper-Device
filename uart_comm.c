/**
 * @file    uart_comm.c
 * @brief   UART communication for STM32 HAL (USART1)
 *          Used for Serial Debug + HC-05 Bluetooth (shared USART1)
 */

#include "uart_comm.h"
#include "config.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ─────────────────────────────────────────────────────────────────── */
void UART_Init(void) {
    /* USART1 is initialised by MX_USART1_UART_Init() in main.c.
     * This function exists as an explicit API entry point and can be
     * used to send a startup banner.                                  */
    UART_TxLine("\r\n==============================");
    UART_TxLine(" Home Helper Device v2.0 STM32");
    UART_TxLine(" STM32F103C8T6 @ 72 MHz");
    UART_TxLine("==============================");
    UART_TxLine("Type '?' for Bluetooth commands");
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_TxChar(char c) {
    HAL_UART_Transmit(&huart1, (uint8_t *)&c, 1U, 100U);
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_TxStr(const char *str) {
    if (!str) return;
    while (*str) UART_TxChar(*str++);
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_TxLine(const char *str) {
    UART_TxStr(str);
    UART_TxChar('\r');
    UART_TxChar('\n');
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_TxUInt(uint16_t val) {
    char buf[6];
    uint8_t idx = 0;

    if (val == 0U) { UART_TxChar('0'); return; }
    while (val > 0U) {
        buf[idx++] = (char)('0' + (val % 10U));
        val /= 10U;
    }
    while (idx > 0U) UART_TxChar(buf[--idx]);
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_TxInt(int16_t val) {
    if (val < 0) { UART_TxChar('-'); val = -val; }
    UART_TxUInt((uint16_t)val);
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_Log(const char *tag, const char *msg) {
    UART_TxChar('[');
    UART_TxStr(tag);
    UART_TxStr("] ");
    UART_TxLine(msg);
}

/* ─────────────────────────────────────────────────────────────────── */
void UART_LogInt(const char *tag, int16_t val) {
    UART_TxChar('[');
    UART_TxStr(tag);
    UART_TxStr("] ");
    UART_TxInt(val);
    UART_TxChar('\r');
    UART_TxChar('\n');
}

/* ─────────────────────────────────────────────────────────────────── */
bool UART_RxAvailable(void) {
    /* Check if RXNE (Read Data Register Not Empty) flag is set        */
    return (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) == SET);
}

/* ─────────────────────────────────────────────────────────────────── */
char UART_RxChar(void) {
    if (!UART_RxAvailable()) return '\0';   /* Non-blocking */
    uint8_t byte = 0;
    HAL_UART_Receive(&huart1, &byte, 1U, 1U);
    return (char)byte;
}
