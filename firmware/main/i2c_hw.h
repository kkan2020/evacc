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
 
#ifndef __I2C_HW_H__
#define __I2C_HW_H__

#include "stm32f10x.h"

//#define _M24C02_
//#define _AT24C02_
//#define _AT24C01_
#define _24C64_

#if defined(_AT24C01_)||defined(_AT24C02_)
  #define EEP_PAGE_SIZE           8
#elif defined(_24C32_)||defined(_24C64_)
  #define EEP_PAGE_SIZE           32
#else
  #define EEP_PAGE_SIZE           16
#endif

//I2C
#define IsI2CBusy(x)          (x->Reg->SR2 & 0x02)

typedef __packed struct 
{
#if defined(_24C32_)||defined(_24C64_)
  uint8_t Addr[2];
#else
  uint8_t Addr;
#endif
  uint8_t Data[EEP_PAGE_SIZE];
} TEepromBuf;

typedef struct
{
  I2C_TypeDef *Reg;
  uint8_t SlaveAddr7, DataAddrMSB;
  TIM_TypeDef *Timer;
  uint8_t *TxBuf, *RxBuf;
  uint16_t TxCount, RxCount, TxIdx, RxIdx;
  uint8_t Timeout, DataReady, Transmit, DummyWrite;
} TI2C;


void InitEEP(void);
int EEPWrite(TI2C *I2CPort, uint16_t Addr, void *Buf, uint16_t Size);
int EEPRead(TI2C *I2CPort, uint16_t Addr, void *Buf, uint16_t Size);
void EepromHandler(TI2C *I2CPort);
void EepromTimeoutHandler(TI2C *I2CPort);

#endif

