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
 
#include <stdint.h>
#include "hal.h"
#include "ds18b20.h"

#define READ_ROM          0x33
#define SKIP_ROM          0xcc
#define CONVERT_T         0x44
#define READ_SPAD         0xbe
#define WRITE_SPAD        0x4e

/*
* CRC8 look up table for polynormial x^8+x^5+x^4+1
*
*/
static const uint8_t CrcTab[256] = {
0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
};

static uint8_t CalcCrc(void *s, uint16_t size)
{
  uint8_t crc, *p;
  
  crc = 0;
  p = s;
  while (size--)
    crc = CrcTab[crc ^ *p++];
  return crc;
}

static void SetDelay(uint16_t Delay_us) //Delay_us >= 2
{
/*  
  US_TIMER->CNT = 0; //reset counter
  US_TIMER->ARR = Delay_us-1; //set auto-reload register
  US_TIMER->SR = (uint16_t)~TIM_FLAG_Update; //clear update flag
  ENABLE_TIMER(US_TIMER);
*/  
  SetTimeout_us(US_TIMER, Delay_us);
  while (IS_NOT_TIMEOUT(US_TIMER));
  DISABLE_TIMER(US_TIMER);
}

static int InitBus(void)
{
  uint8_t s;
    OneWireBus(0);
    SetDelay(490);    
  MASK_IRQ
    OneWireBus(1);
    SetDelay(62);
    s = ReadOneWireBus;
  UNMASK_IRQ
    SetDelay(428);  
  return !s;
}

static void Write_1(void)
{
  MASK_IRQ
    OneWireBus(0);
    SetDelay(2);
    OneWireBus(1);
  UNMASK_IRQ
    SetDelay(65);
}

static void Write_0(void)
{
  MASK_IRQ
    OneWireBus(0);
    SetDelay(65);
    OneWireBus(1);
  UNMASK_IRQ
    SetDelay(2);
}

static int Read_bit(void)
{
  int ret;
  MASK_IRQ
    OneWireBus(0);
    SetDelay(2);
    OneWireBus(1);
    SetDelay(2);
    ret = ReadOneWireBus;
  UNMASK_IRQ
    SetDelay(62);
  return ret;
}

static void WriteByte(uint8_t b)
{
  int i=8;
  while (i--)
  {
    if (b & 0x01)
      Write_1();
    else
      Write_0();
    b >>= 1;
  }
}

static int ReadScratchPad(uint8_t *Buf)
{
  uint8_t c, i, j;  
  for (i=0; i<9; i++)
  {
    c = 0;
    for (j=0; j<8; j++)
    {    
      if (Read_bit())
        c |= 0x80;
      if (j<7)
        c >>= 1;
    }
    Buf[i] = c;
  }  
  return CalcCrc(Buf, 9)==0;
}
  
int GetTemp(__IO float *f)
{
  uint8_t data[9];
  int16_t t;
  
  if (!InitBus())
    return 0;
  WriteByte(SKIP_ROM);
  WriteByte(READ_SPAD);
  if (!ReadScratchPad(data))
    return 0;
  t = data[1];
  t <<= 8;
  t |= data[0];
  *f = (float)t/16;
  return 1;
}

int ConvertTemp(void)
{
  if (!InitBus())
    return 0;
  WriteByte(SKIP_ROM);
  WriteByte(CONVERT_T);
  return 1;
}































