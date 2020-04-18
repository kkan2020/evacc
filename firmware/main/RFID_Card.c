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
 
#include <string.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal.h"
#include "crc16.h"
#include "mifare.h"
#include "RFID_Card.h"

#define LOCK_BLK					61
#define CREDIT_BLK				60

const TKey48 DefKey48 = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
TUART_RFID UartRFID;
TCard Card;  
TConfig Config;

static uint8_t CurBlk;
int8_t LastSector = -1;
static TTlv Tlv;

int RFID_UnpackInteger(uint8_t *data)
{
  int val = data[3];
  val <<= 8;
  val |= data[2];
  val <<= 8;
  val |= data[1];
  val <<= 8;
  val |= data[0];
  return val;
}

uint8_t TLV_ReadTag(void)
{
  return Tlv[0];
}

void RFID_SetCurBlock(uint8_t blk)
{
  CurBlk = blk;
}

uint8_t* RFID_Send(uint8_t* Buf)
{
  int32_t flag;
  uint16_t count, i;
  uint8_t s;
  
  if (!Buf)
    return 0;
  memset(&UartRFID, 0, sizeof(UartRFID));
  UartRFID.TxBuffer = Buf;
  UartRFID.TxSize = RFID_BufSize(Buf);
  USART_ITConfig(RFID_UART, USART_IT_RXNE, ENABLE);    
  USART_ITConfig(RFID_UART, USART_IT_TXE, ENABLE);
  //wait for transmit to complete
	flag = osThreadFlagsWait(UART_TX_OK, osFlagsWaitAny, MsToOSTicks(500)); 
  if (flag<0) //Timeout
    goto _err;
  //wait for response
  flag = osThreadFlagsWait(UART_RX_OK|UART_RX_ERR, osFlagsWaitAny, MsToOSTicks(500)); 
  if ((flag<0)||(flag&UART_RX_ERR)) //Timeout or error
    goto _err;
  //data received
  USART_ITConfig(RFID_UART, USART_IT_TXE|USART_IT_RXNE, DISABLE);
  USART_ClearITPendingBit(RFID_UART, USART_IT_TC|USART_IT_RXNE);
  //calc checksum
  count = RFID_BufSize(UartRFID.RxBuffer);
  s = 0;
  for (i=4; i<count; i++)
    s ^= UartRFID.RxBuffer[i];
  if (s==0)
		return UartRFID.RxBuffer;
_err:    
  USART_ITConfig(RFID_UART, USART_IT_TXE|USART_IT_RXNE, DISABLE);
  USART_ClearITPendingBit(RFID_UART, USART_IT_TC|USART_IT_RXNE);
  return 0;
}

/*
uint8_t SetCardReaderMode(uint8_t mode)
{
  uint8_t *response;
  RFID_Send(RFID_SetModeMsg(mode));
  if (!response || response[8]!=0) //negative
    return 0;
  return 1;
}
*/
//force card request and authen
/*
void ResetCurSector(void)
{
  LastSector = -1;
}
*/

//find the specified card with *pSerial if pSerial!=null or find a new card 
//card serial with be written into Card.Serial if found
uint8_t FindCard(TCardSn Serial)
{
  uint8_t *response;
  int n, found=0;

  LastSector = -1;
  n = ANTI_COLLISION_COUNT;  
  while (n-- && !found)
  {
    response = RFID_Send(RFID_RequestMsg());
    if (!response || response[8]!=0) //negative
      return 0;
    response = RFID_Send(RFID_AntiCollisionMsg());
    if (!response || response[8]!=0) //negative
      return 0;
    if (!Serial) //new card
      found = 1;
    else if ((memcmp(Serial, &response[9], sizeof(TCardSn))==0)) //card is found
      found = 1;
  }
  if (!found)
    return 0;
  memcpy(Card.Serial, &response[9], sizeof(Card.Serial));
  response = RFID_Send(RFID_SelectMsg(Card.Serial));
  if (!response || response[8]!=0) //negative
    return 0;
  return 1;
}

uint8_t AuthCard(uint8_t blkNr, TKey48 Key)
{
  uint8_t *response;

  response = RFID_Send(RFID_AuthMsg(blkNr, Key));
  if (!response || response[8]!=0) //negative
  {
    LastSector = -1;
    return 0;   
  }
  LastSector = blkNr>>2;
  return 1;
}

uint8_t RFID_HaltCard(void)
{
  uint8_t *response;

  response = RFID_Send(RFID_HaltMsg());
  if (!response || response[8]!=0) //negative
    return 0;
  return 1;
}
  

int RFID_FindAndAuthen(TKey48 Key, TCardSn Serial)
{
  int8_t targetSector = CurBlk>>2;
  if ((LastSector<0)||(LastSector!=targetSector))
  {
    if (!FindCard(Serial))
      return 0;
    if (!AuthCard(CurBlk, Key))
      return 0;
  }
  return 1;
}

uint8_t* ReadBlk(TKey48 Key)
{
  uint8_t *response;    
  if (!RFID_FindAndAuthen(Key, Card.Serial))
    return 0;
  response = RFID_Send(RFID_ReadBlkMsg(CurBlk)); 
  if (response && response[8]==0) //positive
    return &response[9]; //16 bytes data
  return 0;
}

uint8_t WriteBlk(TKey48 Key, uint8_t *Data)
{
  uint8_t *response;    
  if (!RFID_FindAndAuthen(Key, Card.Serial))
    return 0;
  response = RFID_Send(RFID_WriteBlkMsg(CurBlk, Data)); 
  if (response && response[8]==0) //positive
    return 1;    
  return 0;
}

void SkipBlk(void) 
{
  CurBlk++;
  if (CurBlk % 4 == 3) //sector trailer
    CurBlk++;  
}

//read TLV field of RFID card to Tlv global variabl and validate CRC
int RFID_ReadFieldAndSkip(TKey48 Key)
{
  uint8_t *blk, unreadLen, readLen;
  int desPos;
  blk = ReadBlk(Key); 
  if (!blk)
    return 0;
  SkipBlk();
  unreadLen = 2+blk[1]; //2+field_length
  if (unreadLen > sizeof(Tlv)) //field too long
    return 0;
//  readLen = 0;
  desPos = 0;
  while (unreadLen) 
  {
    readLen = MIN(RFID_BLK_SIZE, unreadLen);
    memcpy(&Tlv[desPos], blk, readLen);
    unreadLen -= readLen;
    desPos += readLen;
    if (unreadLen)
    {
      blk = ReadBlk(Key);
      if (!blk)
        return 0;
      SkipBlk();
    }
  }
  if (ValidateCrcBlk(Tlv, 2+Tlv[1]))
		return 1;
	return 0;
}

//write TLV field of RFID card from Tlv global variable
int RFID_WriteFieldAndSkip(TKey48 Key)
{
  int srcPos, unwriteLen, writeLen;
  if (Tlv[1] > sizeof(Tlv)-2) //Tlv buffer overflow
    return 0;
  CalcCrcBlk(Tlv, 2+Tlv[1]);
  srcPos = writeLen = 0;
  unwriteLen = 2 + Tlv[1];
  while (unwriteLen>0)
  {
    writeLen = MIN(RFID_BLK_SIZE, unwriteLen);
    if (!WriteBlk(Key, &Tlv[srcPos]))
      return 0;
    SkipBlk();
    srcPos += writeLen;
    unwriteLen -= writeLen;
  }
  return 1;
}

int RFID_ReadValueBlk(uint8_t blkNr, int32_t *pValue) 
{
  uint8_t *response;
  CurBlk = blkNr;
  if (!RFID_FindAndAuthen(Config.Public.K48Pub, Card.Serial))
    return 0;
  response = RFID_Send(RFID_ReadValueMsg(CurBlk));
  if (!response || response[8]!=0) //negative
    return 0;
  *pValue = RFID_UnpackInteger(&response[9]);
  return 1;
}

int RFID_WriteValueBlk(uint8_t blkNr, int32_t value) 
{
  uint8_t *response;
  CurBlk = blkNr;
  LastSector = -1;
  if (!RFID_FindAndAuthen(Config.Public.K48Pub, Card.Serial))
    return 0;
  response = RFID_Send(RFID_InitValueMsg(CurBlk, value));
  if (!response || response[8]!=0) //negative
    return 0;
  return 1;
}

int RFID_ReadBoolBlk(uint8_t blkNr, int *pValue) 
{
  int32_t t;
  if (RFID_ReadValueBlk(blkNr, &t))
  {
    if (t==BOOL_TRUE)
      *pValue = 1;
    else if (t==BOOL_FALSE)
      *pValue = 0;
    else
      return 0;
    return 1;
  }
  return 0;
}

int RFID_WriteBoolBlk(uint8_t blkNr, int value) 
{  return RFID_WriteValueBlk(blkNr, value!=0? BOOL_TRUE: BOOL_FALSE); 
}

uint8_t RFID_DebitValueBlock(float value, int8_t blk)
{
  uint8_t *response;
  LastSector = -1;
  CurBlk = blk;
  if (!RFID_FindAndAuthen(Config.Public.K48Pub, Card.Serial))
    return 0;
  response = RFID_Send(RFID_DebitMsg(blk, FLOAT_TO_FP(value)));
  if (!response || response[8]!=0) //negative
    return 0;
  return 1;
}

int TLV_ReadArray(void *dest, int size)
{
  if (Tlv[1] != size+2)
    return 0;
  memcpy(dest, &Tlv[2], size);
  return 1;
}
int TLV_WriteArray(uint8_t tag, void* src, int size, TKey48 key)
{
  Tlv[0] = tag;
  Tlv[1] = size + 2; //add 2 bytes for crc  
  memcpy(&Tlv[2], src, MIN(MAX_FIELD_LEN, size));
  return RFID_WriteFieldAndSkip(key);
}

int TLV_ReadInteger(int32_t *dest)
{
  if (Tlv[1] != 6) //size mismatch; sizof(int)+ sizeof(crc)
    return 0;
  int32_t t = RFID_UnpackInteger(&Tlv[2]);
  *dest = t;
  return 1;
}
int TLV_WriteInteger(uint8_t tag, int val, TKey48 key)
{
  Tlv[0] = tag;
  Tlv[1] = 4 + 2; //sizeof(int32_t)+2

  Tlv[2] = val & 0xff;
  val >>= 8;
  Tlv[3] = val & 0xff;
  val >>= 8;
  Tlv[4] = val & 0xff;
  val >>= 8;
  Tlv[5] = val & 0xff;
  return RFID_WriteFieldAndSkip(key);
}

int TLV_ReadBool(int32_t *dest)
{
  if (!TLV_ReadInteger(dest))
    return 0;
  if (*dest == BOOL_TRUE)
    *dest = 1;
  else if (*dest == BOOL_FALSE)
    *dest = 0;
  else
    return 0;
  return 1;
}
int TLV_WriteBool(uint8_t tag, int val, TKey48 key)
{
  val = val? BOOL_TRUE: BOOL_FALSE;
  return TLV_WriteInteger(tag, val, key);
}

int TLV_ReadFloat(float *dest)
{
  int32_t t;
  if (!TLV_ReadInteger(&t))
    return 0;
  *dest = FP_TO_FLOAT(t);
  return 1;
}
int TLV_WriteFloat(uint8_t tag, float val, TKey48 key)
{
  return TLV_WriteInteger(tag, FLOAT_TO_FP(val), key);
}
  
int TLV_ReadString(char dest[], int size)
{
  int len = Tlv[1]-2;
  size -= 1;
  if (len > size)
    return 0;
  memcpy(dest, &Tlv[2], len);
  dest[len] = 0;  
  return 1;
}
int TLV_WriteString(uint8_t tag, char* val, TKey48 key)
{
  return TLV_WriteArray(tag, val, strlen(val), key);
}

TCardType RFID_ReadCardType(void)
{
  uint8_t *pValue;
  
  CurBlk = 1;  
  LastSector = -1;
  if (!RFID_FindAndAuthen((uint8_t*)DefKey48, NULL))
    return ctNoCard;
  if (!RFID_ReadFieldAndSkip((uint8_t*)DefKey48)) //read error
    return ctUnknownCard;
  if (TLV_ReadTag() != TAG_CARD_TYPE) //invalide tag
    return ctUnknownCard;
  pValue = &Tlv[2];
  if (memcmp(TAG_CHARGE_CARD_STR, pValue, TAG_TYPE_LEN)==0)
    return ctChargeCard;  
  else if (memcmp(TAG_PUB_CARD_STR, pValue, TAG_TYPE_LEN)==0)
    return ctPublicCard;
  else if (memcmp(TAG_PRIV_CARD_STR, pValue, TAG_TYPE_LEN)==0)
    return ctPrivateCard;
  else 
    return ctUnknownCard;  
}

int RFID_ReadFieldCount(void)
{
  int32_t t;
  CurBlk = 2;
  if (!RFID_ReadFieldAndSkip((uint8_t*)DefKey48))
    return ctNoCard;
  if (TLV_ReadTag() != TAG_FLD_COUNT) //invalide field
    return 0;
  if (!TLV_ReadInteger(&t))
    return 0;
  return t; 
}
  
int RFID_WriteHeader(char* cardTypeStr, int fieldCount)
{
  RFID_SetCurBlock(1);
  if (!TLV_WriteString(TAG_CARD_TYPE, cardTypeStr, (uint8_t*)DefKey48))
    return 0;
  if (!TLV_WriteInteger(TAG_FLD_COUNT, fieldCount, (uint8_t*)DefKey48))
    return 0;  
  return 1;
}


uint8_t RFID_LockCard(void)
{
  if (!RFID_WriteBoolBlk(LOCK_BLK, 1))
    return 0;
  Card.As.ChargeCard.Locked = BOOL_TRUE;
  return 1;
}

uint8_t RFID_UnlockCard(void)
{
  if (!RFID_WriteBoolBlk(LOCK_BLK, 0))
    return 0;
  Card.As.ChargeCard.Locked = BOOL_FALSE;
  return 1;
}

uint8_t RFID_DebitCard(float value)
{
  if (!RFID_DebitValueBlock(value, CREDIT_BLK))
    return 0;
	Card.As.ChargeCard.Credit -= value;
	return 1;
}

uint8_t RFID_ReadCredit(void)
{
  int32_t t;
  int locked;
  //read balance
  if (!RFID_ReadValueBlk(CREDIT_BLK, &t))
    return 0;
  Card.As.ChargeCard.Credit = FP_TO_FLOAT(t);
  //read lock marker
  if (!RFID_ReadBoolBlk(LOCK_BLK, &locked))
    return 0;
  Card.As.ChargeCard.Locked = locked;
  return 1;
}

