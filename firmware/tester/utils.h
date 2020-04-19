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
 
#ifndef __UTILS_H__
  #define __UTILS_H__
#include <stdint.h>  
  
#define ENCRYPT_C1			52845
#define ENCRYPT_C2			11719
#define ENCRYPT_KEY 		0xC5A3

typedef __packed union
{
  __packed struct
  {
    uint8_t Lo: 4;
    uint8_t Hi: 4;
  } Bcd;
  uint8_t Byte;
} TBinDigit;

typedef __packed struct
{
  TBinDigit Year, Month, Day, Hour, Min, Sec;
} TDateTime;


typedef __packed struct
{
  TBinDigit Hour, Min;
} TTimeSch;

__inline void DelayLoop(uint32_t Count);
uint8_t ByteToHexStr(char *Dest, uint8_t Value);
uint8_t HexStrToByte(char *Hex);
uint16_t IntToStr(char *Dest, int32_t Value); //return length of resulting string
int32_t StrToInt(char* Src);
uint32_t FloatToStr(char *Buf, float Value, uint32_t DecimalPt);
//void Decrypt(uint8_t *Buf, uint8_t Size);
//void Encrypt(uint8_t *Buf, uint8_t Size);
uint32_t BcdToInt(TBinDigit BinDigit);
TBinDigit IntToBcd(uint32_t Value);
uint32_t DateToStr(char *Buf, TDateTime *DateTimePtr);
uint32_t DateToFullYearStr(char *Buf, TDateTime *DateTimePtr);
uint32_t TimeToStr(char *Buf, TDateTime *DateTimePtr);
uint32_t TimeSchToStr(char *Buf, TTimeSch *SchPtr);
void StrToTimeSch(TTimeSch *SchPtr, char *Text);
uint32_t DateTimeToStr(char *Buf, TDateTime *DateTimePtr);
void StrToDate(TDateTime *DateTimePtr, char *Text);
void StrToTime(TDateTime *DateTimePtr, char *Text);
uint16_t Reverse16(uint16_t Value);
uint32_t Reverse32(uint32_t Value);
void ReadRTC(TDateTime *DateTimePtr);
void WriteRTC(TDateTime *DateTimePtr);

#endif
