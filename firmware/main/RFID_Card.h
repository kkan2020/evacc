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
 
#ifndef __RFID_CARD_H__
#define __RFID_CARD_H__
#include <stdint.h>
#include "mifare.h"

#define FLOAT_TO_FP(x)                ((int)((x+0.0005)*1000))
#define FP_TO_FLOAT(x)                ((float)(x)/1000+0.0005)
#define BOOL_TRUE                     0x27eb25c9
#define BOOL_FALSE                    0x78d7df40

#define ANTI_COLLISION_COUNT         4

#define UART_TX_OK                   0x01
#define UART_RX_OK                   0x02
#define UART_RX_ERR                  0x04

#define TAG_TYPE_LEN              5
#define TAG_PRIV_CARD_STR         "PRIVA_1.0.0"
#define TAG_PUB_CARD_STR          "PUBLI_1.0.0"
#define TAG_CHARGE_CARD_STR       "CHARG_1.0.0"

//header
#define TAG_CARD_TYPE             '$'
#define TAG_FLD_COUNT             '#'

//Private config
#define TAG_HW_UID                'A'
#define TAG_DEV_UID               'B'
#define TAG_CHR_CURRENT           'C' /* Charging capacity */
#define TAG_CHR_VOLT              'D' /* Charging voltage */
#define TAG_CT_AV                 'E' /* Current transformer conversion factor Ampere per Volt */
#define TAG_PWR_MAN               'F' /* Is power managed */
#define TAG_OPEN_AND_FREE         'G'
#define TAG_3PH_PWR               'H' /* is 3-Phase power? */
#define TAG_AC_PWR                'I' /* is AC POWER? */
#define TAG_DEV_LABEL             'a' /* label to identify device, no need to be unique */
#define TAG_LANGUAGE              'b' /* language to be used for displaying */

//public config
#define TAG_GROUP_UID             'J'
#define TAG_KEY48_PUB             'K'
#define TAG_R_KWH                 'L' /* rate of energy in kWH */
#define TAG_R_PK                  'M' /* rate of parking in hour */
#define TAG_R_PP                  'N' /* rate of parking penality in minute */
#define TAG_R_FP                  'O' /* Free parking after charging in minute */
#define TAG_DH_DEV_TYPE           'P'
#define TAG_DH_NT_ID              'Q'
#define TAG_DH_DEV_REF_JWT        'R'
#define TAG_DH_CLI_REF_JWT        'S'
#define TAG_DH_SERVER_URL         'T'
#define TAG_WIFI_SSID             'c'
#define TAG_WIFI_WPA2_KEY         'd'
#define TAG_WIFI_MODE             'e'

//charging config
#define TAG_USERNAME              'U'
#define TAG_PHONE_NR              'V'
#define TAG_FREE_CARD             'W'
    
#define UID_LEN                      32
#define PHONE_NR_LEN                 20
#define USER_NAME_LEN                40
#define URL_LEN                      80
#define DEV_UID_LEN                  32
#define LABEL_LEN                    40
#define JWT_LEN                      236
#define SSID_LEN                     31
#define WPA2KEY_LEN                  63
#define MAX_FIELD_LEN                (255-2-2)

typedef enum {ctNoCard, ctUnknownCard, ctChargeCard, ctPrivateCard, ctPublicCard} TCardType; 

typedef uint8_t TCardSn[4];
typedef uint8_t TTlv[2+MAX_FIELD_LEN+2];
typedef char TUIdStr[UID_LEN+1]; //append 0
typedef char TDeviceIdStr[DEV_UID_LEN+1]; //append 0
typedef char TLabelStr[LABEL_LEN+1]; //append 0
typedef char TJwtStr[JWT_LEN+1]; //append 0
typedef char TUrlStr[URL_LEN+1]; //append 0
typedef char TOwnerStr[USER_NAME_LEN+1]; //append 0
typedef char TPhoneNrStr[PHONE_NR_LEN+1]; //append 0
typedef char TSsidStr[SSID_LEN+1]; //append 0
typedef char TWpa2KeyStr[WPA2KEY_LEN+1]; //append 0

typedef struct 
{
  uint16_t TxSize, TxPos, RxSize, RxPos, RxState;
  uint8_t *TxBuffer, RxBuffer[RFID_BUF_SIZE];
  uint8_t Tx0, Rx0; 
} TUART_RFID;

typedef struct
{
  uint16_t ChargeCurrentMax;
  uint16_t ChargeVoltage;
  float CTAmperPerVolt;
  uint16_t IsManaged;
} TPower;

typedef struct
{
  float Energy_kWh; 
  float Parking_hr;
  float ParkPenalty_min; 
  uint16_t FreeParking_min;
} TRates;

typedef struct
{
  uint16_t NetworkId, DeviceType;
} TDhDevice;

typedef struct
{
  uint16_t Mode; //Client_Mode=0, AP_Mode = 1;
  TSsidStr SsidStr; //wifi ssid
  TWpa2KeyStr Wpa2KeyStr; //wifi wpa2-key
} TWifiConfig;

typedef struct
{
  TUIdStr GroupUidStr; //an unique id shared with a group of devices; could be used as encryption key
  TKey48 K48Pub; //a mifare Key share with a group of devices to allow reading/writing of card
  TRates Rates;
  TDhDevice DhDevice; //parameters present to DeviceHive server on register 
  TUrlStr ServerUrlStr; //DeviceHive server endpoint url e.g. https://playground.devicehive.com/api/rest
  TJwtStr DevRefreshJwtStr; //json web token for this device
  TJwtStr ClientRefreshJwtStr; //json web token for client device, to be display as QR code for cell phone to scan
  TWifiConfig WifiConfig;
} TPublicConfig;

/*******************************************************************************************/


/*******************************************************************************************
*  Charge card variables
*/
typedef struct 
{
  TOwnerStr NameStr;
  TPhoneNrStr PhoneNrStr;
  int32_t IsFreeCard;
  float Credit;
  int32_t Locked;
} TChargeCard;

/*******************************************************************************************/


/*******************************************************************************************
*  System Config Card definitions
*/
typedef struct
{
  int16_t OpenAndFree; //True: if the device requires no authentication
  int16_t Language; //enum of TLanguage
} TOpMode;

typedef struct
{
  TUIdStr HwUidStr; //an id string uniquely identify this device and is to be compared with PrivateCard's value on processing
  TDeviceIdStr DeviceIdStr; //an unique id string presents to DeviceHive server on register
  TLabelStr LabelStr; //a caption to identify device, no need to be unique
  TPower Power;
  TOpMode OpMode;  
} TPrivateConfig;

/*******************************************************************************************/

typedef struct {
  TCardSn Serial;
  union 
  {
    TChargeCard ChargeCard;
    TPublicConfig Public;
    TPrivateConfig Private;
  } As;
  int8_t Done;
} TCard;

typedef struct
{
  TPrivateConfig Private;
  TPublicConfig Public;
} TConfig;

extern const TKey48 DefKey48;
extern TUART_RFID UartRFID;
extern TCard Card;  
extern TConfig Config;

uint8_t FindCard(TCardSn Serial);
uint8_t RFID_HaltCard(void);
TCardType RFID_ReadCardType(void);
int RFID_ReadFieldCount(void);
int RFID_UnpackInteger(uint8_t *data);
void RFID_SetCurBlock(uint8_t blk);
int RFID_FindAndAuthen(TKey48 Key, TCardSn Serial);
int RFID_ReadValueBlk(uint8_t blkNr, int *pValue);
int RFID_WriteValueBlk(uint8_t blkNr, int value);
uint8_t RFID_DebitValueBlock(float value, int8_t blk);
int RFID_ReadFieldAndSkip(TKey48 Key);
int RFID_WriteFieldAndSkip(TKey48 Key);
int RFID_WriteHeader(char* cardTypeStr, int fieldCount);
uint8_t RFID_LockCard(void);
uint8_t RFID_UnlockCard(void);
uint8_t RFID_DebitCard(float value);
uint8_t RFID_ReadCredit(void);
int TLV_WriteInteger(uint8_t tag, int val, TKey48 key);
int TLV_WriteBool(uint8_t tag, int val, TKey48 key);
int TLV_WriteArray(uint8_t tag, void* src, int size, TKey48 key);
int TLV_WriteString(uint8_t tag, char* val, TKey48 key);
int TLV_WriteFloat(uint8_t tag, float val, TKey48 key);
int TLV_ReadArray(void *dest, int size);
int TLV_ReadInteger(int32_t *dest);
int TLV_ReadBool(int32_t *dest);
int TLV_ReadFloat(float *dest);
int TLV_ReadString(char *dest, int size);
uint8_t TLV_ReadTag(void);

#endif

