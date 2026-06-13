/**
 * @file    dht11.c
 * @brief   DHT11 driver for STM32 HAL
 *
 * GPIO strategy (STM32 HAL):
 *   - Set pin as OUTPUT_OD (open-drain) to drive LOW.
 *   - Set pin as INPUT (floating) to release the bus.
 *   - STM32 HAL does not support run-time GPIO mode switch via a
 *     simple toggle, so we reconfigure the GPIO_InitTypeDef each time.
 *
 * Timing is provided by DWT microsecond delay.
 */

#include "dht11.h"
#include "config.h"
#include "delay_us.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ── Internal: switch DHT11 pin between output (drive) and input ── */
static void dht11_set_output(void) {
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT11_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;   /* Open-drain: pull LOW, release = float */
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static void dht11_set_input(void) {
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = DHT11_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;            /* Internal pull-up while listening */
    HAL_GPIO_Init(DHT11_GPIO_PORT, &gpio);
}

static inline void dht11_pin_low(void) {
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_RESET);
}

static inline void dht11_pin_high(void) {
    HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_GPIO_PIN, GPIO_PIN_SET);
}

static inline uint8_t dht11_pin_read(void) {
    return (HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_GPIO_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

/* ── Wait for pin to reach 'state' within timeout_us ────────────── */
static bool dht11_wait_for(uint8_t state, uint32_t timeout_us) {
    uint32_t t = timeout_us;
    while (t--) {
        if (dht11_pin_read() == state) return true;
        delay_us(1);
    }
    return false;
}

/* ─────────────────────────────────────────────────────────────────── */
bool DHT11_Read(DHT11_Data_t *data) {
    uint8_t bits[5] = {0, 0, 0, 0, 0};

    if (!data) return false;
    data->valid = false;

    /* ── Step 1: MCU sends start signal (LOW ≥ 18 ms) ─────────────── */
    dht11_set_output();
    dht11_pin_low();
    delay_ms(20);                       /* 20 ms start pulse           */

    /* ── Step 2: MCU releases bus, switches to input ───────────────── */
    dht11_pin_high();
    dht11_set_input();
    delay_us(40);                       /* Let bus rise, wait for DHT  */

    /* ── Step 3: DHT11 response — expect LOW 80 µs then HIGH 80 µs ── */
    if (!dht11_wait_for(0U, 100U)) return false;  /* DHT pulls LOW     */
    if (!dht11_wait_for(1U, 100U)) return false;  /* DHT pulls HIGH    */
    if (!dht11_wait_for(0U, 100U)) return false;  /* HIGH ends → data  */

    /* ── Step 4: Read 40 bits (5 bytes) ────────────────────────────── */
    for (uint8_t byte_i = 0; byte_i < 5U; byte_i++) {
        for (int8_t bit_i = 7; bit_i >= 0; bit_i--) {
            /* Each bit starts with ~50 µs LOW */
            if (!dht11_wait_for(1U, 70U)) return false;

            /* Sample at 40 µs: if still HIGH → '1', else '0' */
            delay_us(40);
            if (dht11_pin_read() == 1U) {
                bits[byte_i] |= (1U << (uint8_t)bit_i);
            }

            /* Wait for this bit's HIGH to end */
            if (!dht11_wait_for(0U, 100U)) return false;
        }
    }

    /* ── Step 5: Verify checksum (sum of first 4 bytes = byte 5) ───── */
    uint8_t checksum = (uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]);
    if (checksum != bits[4]) {
        return false;
    }

    /* ── Step 6: Populate output ────────────────────────────────────── */
    data->humidity    = bits[0];    /* Integer humidity  (bits[1] = decimal, ignored) */
    data->temperature = bits[2];    /* Integer temp      (bits[3] = decimal, ignored) */
    data->valid       = true;

    /* Return pin to output-idle (HIGH) state */
    dht11_set_output();
    dht11_pin_high();

    return true;
}
