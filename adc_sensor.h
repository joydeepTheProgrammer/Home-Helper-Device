/**
 * @file    adc_sensor.h
 * @brief   ADC sensor read helpers for STM32 HAL (ADC1)
 */
#ifndef ADC_SENSOR_H
#define ADC_SENSOR_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/**
 * @brief  Read a single ADC channel. Returns 12-bit value (0–4095).
 * @param  channel  ADC_CHANNEL_x constant (e.g. ADC_CHANNEL_0)
 */
uint16_t ADC_ReadChannel(uint32_t channel);

/**
 * @brief  Read an ADC channel and return averaged result.
 * @param  channel  ADC_CHANNEL_x constant
 * @param  samples  Number of samples to average (1–16)
 */
uint16_t ADC_ReadAvg(uint32_t channel, uint8_t samples);

#endif /* ADC_SENSOR_H */
