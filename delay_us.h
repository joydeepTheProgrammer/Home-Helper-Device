/**
 * @file    delay_us.h
 * @brief   Microsecond delay using ARM DWT (Data Watchpoint & Trace) cycle counter
 *          No timer is consumed — works at any clock frequency.
 *
 * Usage:
 *   DWT_Delay_Init();          // Call once before using delay_us()
 *   delay_us(50);              // Block for 50 microseconds
 */

#ifndef DELAY_US_H
#define DELAY_US_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/**
 * @brief  Enable the DWT cycle counter. Must be called once in system init.
 * @retval 0 on success, -1 if DWT not available (non-Cortex-M3/M4)
 */
int  DWT_Delay_Init(void);

/**
 * @brief  Block for the given number of microseconds.
 *         Accuracy depends on SYSCLK (best at 72 MHz).
 * @param  us  Number of microseconds to delay
 */
void delay_us(uint32_t us);

/**
 * @brief  Block for the given number of milliseconds.
 *         Wraps HAL_Delay() for convenience.
 */
static inline void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

#endif /* DELAY_US_H */
