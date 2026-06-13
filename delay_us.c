/**
 * @file    delay_us.c
 * @brief   DWT-based microsecond delay for STM32 Cortex-M3/M4
 *
 * The DWT CYCCNT register counts CPU cycles. At 72 MHz:
 *   1 µs = 72 cycles
 * No timer peripheral is used, so no conflicts with PWM/UART timers.
 */

#include "delay_us.h"
#include "stm32f1xx_hal.h"

/* DWT registers — available on all Cortex-M3/M4/M7 devices */
#define DWT_CONTROL    (*((volatile uint32_t *)0xE0001000U))
#define DWT_CYCCNT     (*((volatile uint32_t *)0xE0001004U))
#define DEM_CR         (*((volatile uint32_t *)0xE000EDFCU))
#define DEM_CR_TRCENA  (1UL << 24U)

/* ─────────────────────────────────────────────────────────────────── */
int DWT_Delay_Init(void) {
    /* Enable trace and debug block (required before DWT can count) */
    DEM_CR |= DEM_CR_TRCENA;

    /* Reset the cycle counter */
    DWT_CYCCNT = 0U;

    /* Enable the cycle counter */
    DWT_CONTROL |= 1U;

    /* Verify it is running */
    if (DWT_CYCCNT == 0U) {
        /* Counter is not incrementing — DWT may not be supported */
        return -1;
    }
    return 0;
}

/* ─────────────────────────────────────────────────────────────────── */
void delay_us(uint32_t us) {
    uint32_t start = DWT_CYCCNT;
    /* cycles_per_us = SystemCoreClock / 1,000,000                     */
    uint32_t cycles = us * (SystemCoreClock / 1000000U);

    /* Wait until the requested number of cycles have elapsed.
     * The subtraction handles 32-bit wraparound correctly.            */
    while ((DWT_CYCCNT - start) < cycles) {
        /* busy-wait */
    }
}
