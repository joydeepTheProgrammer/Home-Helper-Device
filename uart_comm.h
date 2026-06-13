/**
 * @file    uart_comm.h
 * @brief   UART communication helpers — Debug + HC-05 Bluetooth
 */
#ifndef UART_COMM_H
#define UART_COMM_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

void UART_Init(void);                          /* Call after HAL MX init */
void UART_TxChar(char c);
void UART_TxStr(const char *str);
void UART_TxLine(const char *str);             /* str + "\r\n"           */
void UART_TxUInt(uint16_t val);
void UART_TxInt(int16_t val);
void UART_Log(const char *tag, const char *msg);
void UART_LogInt(const char *tag, int16_t val);
bool UART_RxAvailable(void);
char UART_RxChar(void);                        /* Non-blocking, returns 0 if empty */

#endif /* UART_COMM_H */
