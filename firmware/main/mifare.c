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
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "mifare.h"


uint8_t QueryBuf[RFID_BUF_SIZE];

uint16_t RFID_BufSize(uint8_t *Buf)
{
  uint16_t n;

  n = ((uint16_t)Buf[3])<<8;
  n |= Buf[2];
  return n+4;
}

static void AppendCheckSum()
{
  uint16_t n, i;
  uint8_t s=0;
  
  n = RFID_BufSize(QueryBuf) - 1;
  for (i=4; i<n; i++)
    s ^= QueryBuf[i];
  QueryBuf[n] = s;
}

uint8_t* RFID_RequestMsg(void)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x06;
  //command
  QueryBuf[6] = 0x01;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = 0x52;
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_AntiCollisionMsg(void)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x06;
  //command
  QueryBuf[6] = 0x02;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = 0x04;
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_SelectMsg(uint8_t *CardNr)
{
  uint8_t i;

  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x09;
  //command
  QueryBuf[6] = 0x03;
  QueryBuf[7] = 0x02;
  //data
  for (i=0; i<4; i++)  
    QueryBuf[8+i] = CardNr[i];
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_AuthMsg(uint8_t blkNr, TKey48 Key)
{
  uint8_t i;

  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x0D;
  //command
  QueryBuf[6] = 0x07;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = 0x60;
  QueryBuf[9] = blkNr;
  for (i=0; i<sizeof(TKey48); i++)  
    QueryBuf[10+i] = Key[i];
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_ReadBlkMsg(uint8_t Blk)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x06;
  //command
  QueryBuf[6] = 0x08;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = Blk;
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_WriteBlkMsg(uint8_t Blk, uint8_t *Data)
{
  uint8_t i;

  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x16;
  //command
  QueryBuf[6] = 0x09;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = Blk;
  for (i=0; i<16; i++)  
    QueryBuf[9+i] = Data[i];  
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_BeepMsg(uint8_t Duration)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x06;
  //command
  QueryBuf[6] = 0x06;
  QueryBuf[7] = 0x01;
  //data
  QueryBuf[8] = Duration;
  AppendCheckSum();
  return QueryBuf;
}

uint8_t* RFID_InitValueMsg(uint8_t Blk, int32_t Value)
{
  uint8_t *pU8;
  int i;
  
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x0A;
  //command
  QueryBuf[6] = 0x0A;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = Blk;
  pU8 = (uint8_t*)&Value;
  for (i=0; i<4; i++)  
    QueryBuf[9+i] = pU8[i];      
  AppendCheckSum();
  return QueryBuf;  
}

uint8_t* RFID_ReadValueMsg(uint8_t Blk)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x06;
  //command
  QueryBuf[6] = 0x0B;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = Blk;
  AppendCheckSum();
  return QueryBuf;  
}

uint8_t* RFID_CreditMsg(uint8_t Blk, uint32_t Value)
{
  uint8_t *pU8;
  int i;
  
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x0A;
  //command
  QueryBuf[6] = 0x0D;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = Blk;
  pU8 = (uint8_t*)&Value;
  for (i=0; i<4; i++)  
    QueryBuf[9+i] = pU8[i];    
  AppendCheckSum();
  return QueryBuf;  
}

uint8_t* RFID_DebitMsg(uint8_t Blk, uint32_t Value)
{
  uint8_t *pU8;
  int i;
  
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x0A;
  //command
  QueryBuf[6] = 0x0C;
  QueryBuf[7] = 0x02;
  //data
  QueryBuf[8] = Blk;
  pU8 = (uint8_t*)&Value;
  for (i=0; i<4; i++)  
    QueryBuf[9+i] = pU8[i];  
  AppendCheckSum();
  return QueryBuf;  
}

uint8_t *RFID_HaltMsg(void)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x05;
  //command
  QueryBuf[6] = 0x04;
  QueryBuf[7] = 0x02;
  AppendCheckSum();
  return QueryBuf;  
}

void RFID_MakeTrailer(TTrailerBlk *Trailer, uint8_t *KeyA, uint8_t *KeyB, 
              uint8_t ABits0, uint8_t ABits1, uint8_t ABits2, uint8_t ABits3)
{
  uint8_t c1, c2, c3;

  memcpy(Trailer->KeyA, KeyA, sizeof(Trailer->KeyA));
  memcpy(Trailer->KeyB, KeyB, sizeof(Trailer->KeyB));
  
  c2 = ((ABits3 & 0x02)<<2)|((ABits2 & 0x02)<<1)| (ABits1 & 0x02)    |((ABits0 & 0x02)>>1);
  c1 = ((ABits3 & 0x04)<<1)| (ABits2 & 0x04)    |((ABits1 & 0x04)>>1)|((ABits0 & 0x04)>>2);
  c3 = ((ABits3 & 0x01)<<3)|((ABits2 & 0x01)<<2)|((ABits1 & 0x01)<<1)| (ABits0 & 0x01);
  
  Trailer->Access[0] = ~((c2<<4)|c1);
  Trailer->Access[1] = (c1<<4)|((~c3)&0x0f);
  Trailer->Access[2] = (c3<<4)|c2;
  Trailer->Access[3] = 0; 
}

/*
uint8_t *RFID_SetModeMsg(uint8_t mode)
{
  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x06;
  //command
  QueryBuf[6] = 0x01;
  QueryBuf[7] = 0x06;
  //data
  QueryBuf[8] = mode;
  AppendCheckSum();
  return QueryBuf;  
}
*/

/*
uint8_t* RFID_WriteBlkQuickMsg(uint8_t *Key, uint8_t Blk, uint8_t *Data)
{
  uint8_t i;

  memset(QueryBuf, 0, sizeof(QueryBuf));
  QueryBuf[0] = RFID_STX1;
  QueryBuf[1] = RFID_STX2;
  //length
  QueryBuf[2] = 0x1D;
  //command
  QueryBuf[6] = 0x09;
  QueryBuf[7] = 0x06;
  QueryBuf[8] = 0x60;
  QueryBuf[9] = Blk;
  //key
  for (i=0; i<6; i++)  
    QueryBuf[10+i] = Key[i];  
  //data
  for (i=0; i<16; i++)  
    QueryBuf[16+i] = Data[i];  
  AppendCheckSum();
  return QueryBuf;
}
*/




