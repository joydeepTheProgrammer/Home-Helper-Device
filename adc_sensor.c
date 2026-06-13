/**
 * @file    adc_sensor.c
 * @brief   ADC1 single-channel read helper for STM32 HAL
 *
 * ADC1 must be initialised by MX_ADC1_Init() in main.c before use.
 * We reconfigure the channel selection (SQR3) per read call so that
 * both GA_CHANNEL_0 and LDR_CHANNEL_1 can share ADC1.
 */

#include "adc_sensor.h"
#include "config.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────────────────── */
uint16_t ADC_ReadChannel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;   /* Enough for MQ-2/LDR */

    /* Reconfigure the channel for next conversion */
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0U;
    }

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return 0U;
    }

    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;   /* 12-bit result: 0–4095 */
}

/* ─────────────────────────────────────────────────────────────────── */
uint16_t ADC_ReadAvg(uint32_t channel, uint8_t samples) {
    if (samples < 1U) samples = 1U;
    if (samples > 16U) samples = 16U;

    uint32_t sum = 0U;
    for (uint8_t i = 0; i < samples; i++) {
        sum += ADC_ReadChannel(channel);
    }
    return (uint16_t)(sum / (uint32_t)samples);
}
