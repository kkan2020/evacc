/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Ken W.T. Kan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
#ifndef __WIFI_THREAD_H__
#define __WIFI_THREAD_H__

#include <stdint.h>
#include "cmsis_os2.h"
#include "stm32f10x.h"

#define PKT_PLAYLOAD_SIZE           264
#define UART_BUF_SIZE						    (PKT_PLAYLOAD_SIZE+4)

typedef struct 
{
  USART_TypeDef* Uart;
  uint16_t TxSize, TxPos, RxPos;
  uint8_t TxBuffer[UART_BUF_SIZE], RxBuffer[UART_BUF_SIZE];
  uint64_t lastSentTickCount;
} TSPort;

typedef struct
{
  TSPort* Port;
  uint16_t len;
  uint8_t payload[UART_BUF_SIZE];
} TPacket;

extern const osThreadAttr_t dhThread_attr;
extern TSPort WiFiPort;
extern osMessageQueueId_t SPortMsgQ;

void dhThread(void *arg);

#endif

