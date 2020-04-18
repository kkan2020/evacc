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
#include "RFIDThread.h"
#include "PilotThread.h"
#include "CT_Thread.h"
#include "UIThread.h"
#include "CT_Thread.h"
#include "stm32f10x_it.h"
#include "trans.h"
#include "lcg-par.h" 

#define ABITS_TRANS_TR      0x01  /* tailer bits: transport mode */
#define ABITS_TR            0x03  /* trailer bits: KeyA read access bits only; KeyB WO KeyA/B and R/W access bits */
#define ABITS_DATA          0x04  /* data block bits: KeyA RO; KeyB is R/W */
#define ABITS_VALUE         0x06  /* value block bits: KeyA RO+Dec_O; KeyB R/W+Inc/Dec */

#define RFID_THREAD_DELAY_MS          250
#define MIN_BALANCE                   0

osRtxThread_t RFIDThread_tcb;
uint64_t RFIDThreadStk[128]; 
const osThreadAttr_t RFIDThread_attr = 
{ 
  .cb_mem = &RFIDThread_tcb,
  .cb_size = sizeof(RFIDThread_tcb),
  .stack_mem = RFIDThreadStk,  
  .stack_size = sizeof(RFIDThreadStk),
  .priority = osPriorityNormal,
};

static TTransState TrState;
  
static uint8_t ReadPrivateCard(void)
{
  int fCount;
  int32_t t;

//  ResetCurSector();
  fCount = RFID_ReadFieldCount();
  if (fCount==0)
    return 0;  
  Card.As.Private = Config.Private;
  while (fCount--)
  {    
    if (!RFID_ReadFieldAndSkip((uint8_t*)DefKey48))
      return 0;
    switch(TLV_ReadTag())
    {
      case TAG_HW_UID:
        if (!TLV_ReadString(Card.As.Private.HwUidStr, sizeof(Card.As.Private.HwUidStr)))
          return 0;       
        break;
      
      case TAG_DEV_UID:
        if (!TLV_ReadString(Card.As.Private.DeviceIdStr, sizeof(Card.As.Private.DeviceIdStr)))
          return 0;
        break;
        
      case TAG_DEV_LABEL:
        if (!TLV_ReadString(Card.As.Private.LabelStr, sizeof(Card.As.Private.LabelStr)))
          return 0;
        break;        
      
      case TAG_CHR_CURRENT: //charging current in A
        if (!TLV_ReadInteger(&t))
          return 0;
        if (t>CHARGE_CURRENT_MAX)
          t = CHARGE_CURRENT_MAX;
        else if (t<CHARGE_CURRENT_MIN)
          t = CHARGE_CURRENT_MIN;
        Card.As.Private.Power.ChargeCurrentMax = t;
        break;

      case TAG_CHR_VOLT: //charging voltage in V
        if (!TLV_ReadInteger(&t))
          return 0;
        if (t > CHARGE_VOLTAGE_MAX)
          t = CHARGE_VOLTAGE_MAX;
        else if (t < CHARGE_VOLTAGE_MIN)
          t = CHARGE_VOLTAGE_MIN;
        Card.As.Private.Power.ChargeVoltage = t;
        break;

      case TAG_CT_AV: //current transformer transconductance(A/V)
        if (!TLV_ReadFloat(&Card.As.Private.Power.CTAmperPerVolt))
          return 0;
        break;

      case TAG_PWR_MAN:        
        if (!TLV_ReadBool(&t))
          return 0;        
        Card.As.Private.Power.IsManaged = t;
        break;
      
      case TAG_OPEN_AND_FREE:
        if (!TLV_ReadBool(&t))
          return 0;
        Card.As.Private.OpMode.OpenAndFree = t;
        break;
      
      case TAG_LANGUAGE:
        if (!TLV_ReadInteger(&t))
          return 0;
        Card.As.Private.OpMode.Language = t;
        break;
      
      default:
        break;
    }
  }
  if (strlen(Config.Private.HwUidStr)==0)
    return  1;
  if (strcmp(Config.Private.HwUidStr, Card.As.Private.HwUidStr)==0) //id matched
    return 1;
  return 0; //id unmatched
}

static int SavePrivateConfig(void)
{
  if (!EEP_WriteStringBlk(EEP_HW_UID_ADDR, Card.As.Private.HwUidStr, sizeof(TUIdStr), Config.Private.HwUidStr))
    return 0;
  if (!EEP_WriteStringBlk(EEP_DH_DEV_ID_ADDR, Card.As.Private.DeviceIdStr, sizeof(TDeviceIdStr), Config.Private.DeviceIdStr))
    return 0;
  if (!EEP_WriteStringBlk(EEP_LABEL_ADDR, Card.As.Private.LabelStr, sizeof(TLabelStr), Config.Private.LabelStr))
    return 0;
  if (!EEP_WriteBlk(EEP_POWER_ADDR, &Card.As.Private.Power, sizeof(Card.As.Private.Power), &Config.Private.Power, RangeCheck_Power))    
    return 0;  
  else
  {
    osThreadFlagsSet(CTThread_id, CT_UPD_VARS);
    osThreadFlagsSet(PilotThread_id, PILOT_UPD_VARS);      
  }
  if (!EEP_WriteBlk(EEP_OP_MODE_ADDR, &Card.As.Private.OpMode, sizeof(Card.As.Private.OpMode), &Config.Private.OpMode, NULL))
    return 0;
  return 1;
}

static uint8_t CreatePrivateCard(void)
{
//  TTrailerBlk trailer;
  TCardType cardType;  
  cardType = RFID_ReadCardType();
  if (cardType == ctNoCard)
		return 0;
  else if (cardType==ctPrivateCard)
  {    
    if (!ReadPrivateCard()) //not this key Card 
      return 0;
  } 
  else if (cardType!=ctUnknownCard)//not new card
    return 0;
                                                                                                                                                                                        
/*  
  // Initialize sector_0 trailer 
  RFID_SetCurBlock(3);
  if (!RFID_FindAndAuthen(System.System.K48Def, 0))
    return 0;
  RFID_MakeTrailer(&trailer, System.System.K48Def, System.System.K48Def, 
      0x00, ABITS_DATA, ABITS_DATA, ABITS_TRANS_TR);
  if (!WriteBlk(System.System.K48Def, (uint8_t*)&trailer))
    return 0;
*/    
  if (!RFID_WriteHeader(TAG_PRIV_CARD_STR, 11))
    return 0;
  
/*  
  // init sector1 trailer
  RFID_SetCurBlock(7);
  RFID_MakeTrailer(&trailer, System.System.K48Def, System.System.K48Def, 
      ABITS_DATA, ABITS_DATA, ABITS_DATA, ABITS_TR);   
  if (!WriteBlk(System.System.K48Def, (uint8_t*)&trailer))
    return 0;
*/  
  if (!TLV_WriteString(TAG_HW_UID, Config.Private.HwUidStr, (uint8_t*)DefKey48))
    return 0;
  
  if (!TLV_WriteString(TAG_DEV_UID, Config.Private.DeviceIdStr, (uint8_t*)DefKey48))
    return 0;
  
  if (!TLV_WriteString(TAG_DEV_LABEL, Config.Private.LabelStr, (uint8_t*)DefKey48))
    return 0;

  if (!TLV_WriteInteger(TAG_CHR_CURRENT, Config.Private.Power.ChargeCurrentMax, (uint8_t*)DefKey48))
    return 0;

  if (!TLV_WriteInteger(TAG_CHR_VOLT, Config.Private.Power.ChargeVoltage, (uint8_t*)DefKey48))
    return 0;

  if (!TLV_WriteFloat(TAG_CT_AV, Config.Private.Power.CTAmperPerVolt, (uint8_t*)DefKey48))
    return 0;

  if (!TLV_WriteBool(TAG_PWR_MAN, Config.Private.Power.IsManaged, (uint8_t*)DefKey48))
    return 0;
  
  if (!TLV_WriteBool(TAG_OPEN_AND_FREE, Config.Private.OpMode.OpenAndFree, (uint8_t*)DefKey48))
    return 0;
  
  if (!TLV_WriteInteger(TAG_LANGUAGE, Config.Private.OpMode.Language, (uint8_t*)DefKey48))
    return 0;  

  if (!TLV_WriteBool(TAG_3PH_PWR, IS_3PHASE_POWER, (uint8_t*)DefKey48))
    return 0;
  
  if (!TLV_WriteBool(TAG_AC_PWR, 1, (uint8_t*)DefKey48))
    return 0;
  
  return 1;
}

static uint8_t ReadPublicCard(void)
{
  int16_t fCount, fRead;
  int32_t t;
  
//  ResetCurSector();
  fCount = RFID_ReadFieldCount();
  if (fCount==0)
    return 0;
  Card.As.Public = Config.Public;
  fRead = 0;
  while (fCount--)
  {
    if (!RFID_ReadFieldAndSkip(Config.Public.K48Pub))
      return 0;
    switch(TLV_ReadTag()) //tag
    {
      case TAG_GROUP_UID:
        if (!TLV_ReadString(Card.As.Public.GroupUidStr, sizeof(Card.As.Public.GroupUidStr)))
          return 0;       
        fRead++;
        break;
      
      case TAG_KEY48_PUB:
        if (!TLV_ReadArray(Card.As.Public.K48Pub, sizeof(Card.As.Public.K48Pub)))
          return 0;       
        fRead++;
        break;
        
      case TAG_R_KWH: //rate of energy per kWH
        if (!TLV_ReadFloat(&Card.As.Public.Rates.Energy_kWh))
          return 0;
        fRead++;
        break;

      case TAG_R_PK: //rate of parking per hour
        if (!TLV_ReadFloat(&Card.As.Public.Rates.Parking_hr))
          return 0;
        fRead++;
        break;
        
      case TAG_R_PP: //rate of parking penality per minute
        if (!TLV_ReadFloat(&Card.As.Public.Rates.ParkPenalty_min))
          return 0;
        fRead++;
        break;

      case TAG_R_FP: //free parking time in minutes       
        if (!TLV_ReadInteger(&t)) 
          return 0;
        Card.As.Public.Rates.FreeParking_min = t;
        fRead++;
        break;

      case TAG_DH_DEV_TYPE:
        if (!TLV_ReadInteger(&t))
          return 0;
        Card.As.Public.DhDevice.DeviceType = t;
        fRead++;
        break;
                
      case TAG_DH_NT_ID:
        if (!TLV_ReadInteger(&t))
          return 0;
        Card.As.Public.DhDevice.NetworkId = t;
        fRead++;
        break;

      case TAG_DH_SERVER_URL:
        if (!TLV_ReadString(Card.As.Public.ServerUrlStr, sizeof(Card.As.Public.ServerUrlStr)))
          return 0;
        fRead++;
        break;
      
      case TAG_DH_DEV_REF_JWT:
        if (!TLV_ReadString(Card.As.Public.DevRefreshJwtStr, sizeof(Card.As.Public.DevRefreshJwtStr)))
          return 0;
        fRead++;
        break;
      
      case TAG_DH_CLI_REF_JWT:
        if (!TLV_ReadString(Card.As.Public.ClientRefreshJwtStr, sizeof(Card.As.Public.ClientRefreshJwtStr)))
          return 0;
        fRead++;
        break;

      case TAG_WIFI_MODE:
        if (!TLV_ReadInteger(&t)) 
          return 0;
        Card.As.Public.WifiConfig.Mode = t;
        fRead++;
        break;

      case TAG_WIFI_SSID:
        if (!TLV_ReadString(Card.As.Public.WifiConfig.SsidStr, sizeof(Card.As.Public.WifiConfig.SsidStr)))
          return 0;
        fRead++;
        break;

      case TAG_WIFI_WPA2_KEY:
        if (!TLV_ReadString(Card.As.Public.WifiConfig.Wpa2KeyStr, sizeof(Card.As.Public.WifiConfig.Wpa2KeyStr)))
          return 0;
        fRead++;
        break;
        
      default:
        break;
    }
  }  
  return fRead>0; /* return success if some fields were read */
}

int SavePublicConfig(void)
{
  if (!EEP_WriteBlk(EEP_KEY48_PUB_ADDR, Card.As.Public.K48Pub, sizeof(Card.As.Public.K48Pub), Config.Public.K48Pub, NULL))
    return 0;  
  if (!EEP_WriteStringBlk(EEP_GROUP_ID_ADDR, Card.As.Public.GroupUidStr, sizeof(TUIdStr), Config.Public.GroupUidStr))
    return 0;
  if (!EEP_WriteBlk(EEP_RATES_ADDR, &Card.As.Public.Rates, sizeof(Config.Public.Rates), &Config.Public.Rates, NULL))
    return 0;
  if (!EEP_WriteBlk(EEP_DH_DEV_ADDR, &Card.As.Public.DhDevice, sizeof(Config.Public.DhDevice), &Config.Public.DhDevice, NULL))
    return 0;  
  if (!EEP_WriteStringBlk(EEP_SERVER_URL_ADDR, Card.As.Public.ServerUrlStr, sizeof(Card.As.Public.ServerUrlStr), Config.Public.ServerUrlStr))
    return 0;
  if (!EEP_WriteStringBlk(EEP_DEVICE_JWT_ADDR, Card.As.Public.DevRefreshJwtStr, sizeof(Card.As.Public.DevRefreshJwtStr), Config.Public.DevRefreshJwtStr))
    return 0;
  if (!EEP_WriteStringBlk(EEP_CLIENT_JWT_ADDR, Card.As.Public.ClientRefreshJwtStr, sizeof(Card.As.Public.ClientRefreshJwtStr), Config.Public.ClientRefreshJwtStr))
    return 0;  
  if (!EEP_WriteBlk(EEP_WIFI_CONFIG_ADDR, &Card.As.Public.WifiConfig, sizeof(Card.As.Public.WifiConfig), &Config.Public.WifiConfig, RangeCheck_Wifi))
    return 0;  
  return 1;
}

static uint8_t ReadChargeCard(void)
{
  int16_t fCount;
  int32_t t;

//  ResetCurSector();  
  fCount = RFID_ReadFieldCount();
  memset(&Card.As.ChargeCard, 0, sizeof(TChargeCard));   
  while (fCount--)
  {
    if (!RFID_ReadFieldAndSkip(Config.Public.K48Pub))
      return 0;
    switch(TLV_ReadTag()) //tag
    {
      case TAG_USERNAME:
        TLV_ReadString(Card.As.ChargeCard.NameStr, sizeof(Card.As.ChargeCard.NameStr));
        break;
        
      case TAG_PHONE_NR:
        TLV_ReadString(Card.As.ChargeCard.PhoneNrStr, sizeof(Card.As.ChargeCard.PhoneNrStr));
        break;
        
      case TAG_FREE_CARD:
        if (!TLV_ReadBool(&t))
          return 0;
        Card.As.ChargeCard.IsFreeCard = t;
        break;
        
      default:
        break;
    }
  } 
  return RFID_ReadCredit();
}

static void Timer_Callback(void *arg)
{
  *((uint32_t*)arg) = 1;
}

static uint8_t DebitChargeCard(float amount)
{
  int success = 0;  
  if (amount<=0)
    success = 1;
  else if (RFID_DebitCard(amount))
  {
    Trans_SetCardCredit(Card.As.ChargeCard.Credit);
    success = RFID_UnlockCard();
  }
  //uncheck card
  return success;
}

static TUIMessage *uiMsg;

void DoCard(TCardType cardType)
{
  float debitAmount=0;
  switch(cardType)
  {
    case ctNoCard:
      break;
        
    case ctChargeCard:
      if (!ReadChargeCard()) //read card error
      {
        TUIMessage *uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
        if (uiMsg)
        {
          uiMsg->Code = MSG_UI_READ_CARD_ERROR;
          uiMsg->Seconds = 2;
          Send2UI(uiMsg);
        }
        Beep(LONG_BEEP);
        return;
      }
      switch(TrState)
      {
        case trAuthen: //authentication
          if (!Card.As.ChargeCard.IsFreeCard)
          {
            if (Card.As.ChargeCard.Locked) //already marked => previous transaction was incompleted
            {
_payment_pending:                  
              uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
              if (uiMsg)
              {
                uiMsg->Code = MSG_UI_FEE_UNPAID;
                uiMsg->Seconds = 2;
                Send2UI(uiMsg);
                Card.Done = 1;
              }
              Beep(LONG_BEEP);
              return;
            }
            if (Card.As.ChargeCard.Credit <= MIN_BALANCE) //not enough money
            {
              uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
              if (uiMsg)
              {
                uiMsg->Code = MSG_UI_INSUFF_BAL;
                uiMsg->Seconds = 2;
                Send2UI(uiMsg);
                Card.Done = 1;
              }
              Beep(LONG_BEEP);
              return;
            }              
          }
          if (!RFID_LockCard()) //cannot mark the card as checked?
          {
            uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
            if (uiMsg)
            {
              uiMsg->Code = MSG_UI_WRITE_CARD_ERROR;
              uiMsg->Seconds = 2;
              Send2UI(uiMsg);
              Card.Done = 1;
            }                
            Beep(LONG_BEEP);
            return;
          }       
          Trans_AuthenRFIDCard(&Card);
          Card.Done = 1;
          DoubleBeep();
          break;
          
        case trCharging: //EV is charging, checking card means stop charging          
          if (!Trans_IsSameCard(&Card))//not the same card show card balance
            goto _show_card_balance;
          Trans_SetState(trBilling, 0);
          //no break
          
        case trBilling: //pay bill
          if (!Trans_IsSameCard(&Card))//not the same card
          {
            Beep(LONG_BEEP);
            return;
          }
          debitAmount = Trans_CheckBill();
          if (!DebitChargeCard(debitAmount))
          {
            uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
            if (uiMsg)
            {
              uiMsg->Code = MSG_UI_WRITE_CARD_ERROR;
              uiMsg->Seconds = 2;
              Send2UI(uiMsg);
//                  Card.Done = 1;
            }                
            Beep(LONG_BEEP);
            return;
          }
          Trans_SetState(trPaid, debitAmount);
          Card.Done = 1;              
          DoubleBeep();
          break;
          
        default: //show card balance
_show_card_balance:                
          if (Card.As.ChargeCard.Locked)
            goto _payment_pending;          
          uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
          if (uiMsg)
          {
            uiMsg->Code = MSG_UI_SHOW_BALANCE;
            uiMsg->As.Float = Card.As.ChargeCard.Credit;
            uiMsg->Seconds = 3;
            Send2UI(uiMsg);
            Card.Done = 1;
            DoubleBeep();
          }
          else
            Beep(LONG_BEEP);        
          break;
      }
      break;

    case ctPrivateCard:
      if (!ReadPrivateCard())
      {        
        Beep(LONG_BEEP);
        return;
      }
      if (!SavePrivateConfig())
      {
        Beep(LONG_BEEP);
        return;
      }
      SetLcgLanguage((TLanguage)Config.Private.OpMode.Language);
      Card.Done = 1;
      DoubleBeep();
      break;
      
    case ctPublicCard:
      if (!ReadPublicCard())
      {
        Beep(LONG_BEEP);
        return;
      }
      if (!SavePublicConfig())
      {
        Beep(LONG_BEEP);
        return;
      }
      MASK_IRQ;
        WiFi_Reset(); //reset wifi in case server or WIFI parameters had been changed
      UNMASK_IRQ
      Card.Done = 1;
      DoubleBeep();
      break;

    default:
      Card.Done = 1;
      Beep(LONG_BEEP);
      break;
  }  
}

void RFIDThread(void *arg) 
{
  static uint32_t timeouted;
  static osTimerId_t CreateCard_timer;
	
	DoubleBeep();
  if (IS_DEBUG_MODE)
  {		
    uiMsg = osMemoryPoolAlloc(UIMemPool, 0);
    if (uiMsg)
    {
      uiMsg->Code = MSG_UI_CREATE_KEY_CARD;
      uiMsg->Seconds = 5;
      Send2UI(uiMsg);
    }		
    // Create a Key card of this machine		
    timeouted = 0;
    CreateCard_timer = osTimerNew(&Timer_Callback, osTimerOnce, &timeouted, NULL);  
    osTimerStart(CreateCard_timer, MsToOSTicks(5000));
    while(!timeouted)
    {
      if (CreatePrivateCard())
      {
        DoubleBeep();
        Card.Done = 1;
        break;
      }
      osDelay(MsToOSTicks(1000));     
    }
    osTimerDelete(CreateCard_timer);
  }
  
  while(1)
  {
    TrState = Trans_GetState();
    if (Config.Private.OpMode.OpenAndFree) 
    {
      switch(TrState)
      {
        case trAuthen: //authentication
          Trans_SetAuthorized(TRUE);
          break;
        case trBilling: //pay bill
          Trans_SetState(trPaid, 0);
          break;
        default:
          break;
      }  
    }
    TCardType cardType = RFID_ReadCardType();
    //if the card has alredy been done it must be removed from reader first in order to 
    //do next transaction   
    if (cardType==ctNoCard) 
      Card.Done = 0;
    else if (!Card.Done)
      DoCard(cardType);          
//    RFID_HaltCard();
    osDelay(MsToOSTicks(RFID_THREAD_DELAY_MS));  
  }
}


