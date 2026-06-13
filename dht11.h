/**
 * @file    dht11.h
 * @brief   DHT11 Temperature & Humidity Driver for STM32 HAL
 */
#ifndef DHT11_H
#define DHT11_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t temperature;   /* Integer °C  */
    uint8_t humidity;      /* Integer %RH */
    bool    valid;
} DHT11_Data_t;

/**
 * @brief  Read DHT11 sensor. Blocking ~22 ms.
 * @param  data  Pointer to DHT11_Data_t to fill.
 * @return true on success, false on timing/checksum error.
 */
bool DHT11_Read(DHT11_Data_t *data);

#endif /* DHT11_H */
