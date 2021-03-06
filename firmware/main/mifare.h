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
 
#ifndef __MYFARE_H__
#define __MYFARE_H__
#include <stdint.h>

/*
KEY DEFINITIONS:
KeyA, KeyB, Key48 -- 48-bit(6 bytes) key defined in mifare protocol
Key-Def -- a predefined 48-bit(6 bytes) random number for mifare protocol, known by all machine
Key-Pri -- an unique 48-bit(6 bytes) random number generated by charger
Key-Pub -- an unique 48-bit(6 bytes) random number generated by management software
Key-Man -- an unique 48-bit(6 bytes) random number generated by management software
Key128 -- an unique 128-bit(16 bytes) random number generated by charger for encrypt/decrypt network message

CARD TYPES:
PrivateCard -- a contactless card containing the hardware configuration of the charger
RateCard -- a contactless card containing the software configuration of the charger
ChargeCard -- a contactless card for authentication of the right of using chargers

DATA FORMAT OF CONTACTLESS CARD:

GENERAL FORMAT:
-----------------------------------------------------------------------------------------
sector 0 Blk1   <TAG_CARD_TYPE><5+2><?> ; type ?=TAG_KEY|TAG_PUB_CARD_STR|TAG_CHARGE_CARD_STR + crc(2 bytes)
         Blk2   <TAG_FLD_COUNT><4+2><?>; int32_t(4 bytes) + crc(2 bytes)(count excludes value fields)
         Blk3     KeyA=Key-Def, KeyB=?
sector 1        <T><L><V> ; TLV data field + crc(2 bytes)
   .   .
sector n        <T><L><V>   
-----------------------------------------------------------------------------------------


System Config Card: generated by charger at startup in debug mode, will wait 5 seconds for checking RFID card
-----------------------------------------------------------------------------------------
sector 0 Blk1     <TAG_CARD_TYPE><5><?> 
         Blk2     <TAG_FLD_COUNT><4+2><?>; int32_t(4 bytes) + crc(2 bytes)
         Blk3     KeyA=KeyB=Key-Def
sector 1 Blk0     <TAG_HW_ID><16+2><Key128>; key128+crc, 
         Blk1     data...
         Blk2     <TAG_KEY128><16+2><Key128>; key128+crc,
         Blk3     KeyA=Key-Def, KeyB=Key-Def
Sector 2 Blk0     data...
         Blk1     <TAG_KEY48_PUB><6+2><Key48-Pub>;key+crc, written by management software 
         Blk2     ...
         Blk3     KeyA=Key-Def, KeyB=Key-Def
-----------------------------------------------------------------------------------------


Rate Card:
-----------------------------------------------------------------------------------------
sector 0 Blk1     <TAG_CARD_TYPE><5><?>
         Blk2     <TAG_FLD_COUNT><4+2><?>; int32_t(4 bytes) + crc(2 bytes)
         Blk3     KeyA=Key-Def, KeyB=Key-Man
sector 1 Blk0     <TLV>
         Blk1     ...
         Blk2     ...
         Blk3     KeyA=Key-Pub, KeyB=Key-Man
   .
   .
   .
-----------------------------------------------------------------------------------------
   
Charge Card:
-----------------------------------------------------------------------------------------
sector 0 Blk1     <TAG_CARD_TYPE><5><?>
         Blk2     <TAG_FLD_COUNT><4+2><?>; int32_t(4 bytes) + crc(2 bytes)
         Blk3     KeyA=Key-Def, KeyB=Key-Man
sector 1 Blk0     <TAG_OWNER><N+2><values> ;card owner's name+crc
         Blk1     <TAG_PAY_MODE><4+3> ;integer, pay mode of this user
         Blk2     <TAG_PHONE_NR><N+2><values> ;phone#+crc
         Blk3     KeyA=Key-Pub, KeyB=Key-Man
sector 2 Blk0     data...
         Blk1     data...
         Blk2     ...
         Blk3     KeyA=Key-Pub, KeyB=Key-Man
   .
   .
   .
sector15 Blk0     value field1 ;money balance
         Blk1     value field3 ;lock marker, 0=unlocked, 1=locked
				 BLK2			
         Blk3     KeyA=Key-Pub(RO, Dec/O), KeyB=Key-Man(RW, Inc/Dec)
-----------------------------------------------------------------------------------------

*/
#define KEY48_LEN                     6

#define RFID_STX1                     0xAA
#define RFID_STX2                     0xBB
#define RFID_BLK_SIZE                 16
#define RFID_BUF_SIZE                 64
#define RFID_RO                       02
#define RFID_RW                       01

typedef __packed struct
{
  int32_t Value, nValue, bValue;
  uint8_t Adr, nAdr, bAdr, nbAdr;
} TValueBlk;

typedef uint8_t TKey48[KEY48_LEN];

typedef __packed struct
{
  uint8_t KeyA[KEY48_LEN];
  uint8_t Access[4];
  uint8_t KeyB[KEY48_LEN];
} TTrailerBlk;

typedef __packed union
{
  uint8_t Data[RFID_BLK_SIZE];
  TValueBlk Value;
  TTrailerBlk Trailer;  
} TMifareBlk;

uint16_t RFID_BufSize(uint8_t *Buf);
uint8_t* RFID_RequestMsg(void);
uint8_t* RFID_AntiCollisionMsg(void);
uint8_t* RFID_SelectMsg(uint8_t *CardNr);
uint8_t* RFID_AuthMsg(uint8_t Blk, uint8_t *Key);
uint8_t* RFID_ReadBlkMsg(uint8_t Blk);
uint8_t* RFID_WriteBlkMsg(uint8_t Blk, uint8_t *Data);
uint8_t* RFID_BeepMsg(uint8_t Duration);
uint8_t* RFID_InitValueMsg(uint8_t Blk, int32_t Value);
uint8_t* RFID_ReadValueMsg(uint8_t Blk);
uint8_t* RFID_CreditMsg(uint8_t Blk, uint32_t Value);
uint8_t* RFID_DebitMsg(uint8_t Blk, uint32_t Value);
uint8_t* RFID_HaltMsg(void);
void RFID_MakeTrailer(TTrailerBlk *Trailer, uint8_t *KeyA, uint8_t *KeyB, 
              uint8_t ABits0, uint8_t ABits1, uint8_t ABits2, uint8_t ABits3);
//uint8_t *RFID_SetModeMsg(uint8_t mode);
//uint8_t* RFID_WriteBlkQuickMsg(uint8_t *Key, uint8_t Blk, uint8_t *Data);
#endif


