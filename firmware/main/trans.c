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
#include "hal.h"
#include "trans.h"
#include "stm32f10x_it.h"
#include "CT_Thread.h"
#include "dhThread.h"
#include "app_main.h"

static TTrans Trans;

TTransState Trans_GetState(void)
{
  MASK_IRQ
    TTransState ret =  Trans.State;
  UNMASK_IRQ
  return ret;
}

char* Trans_GetStateName(void)
{
  char *s;
  switch(Trans_GetState())
  {
    case trIdle:
      s = "idle";
      break;
    case trHandshake:
      s = "handshake";
      break;
    case trAuthen:
      s = "authen";
      break;
    case trCharging:
      s = "charging";
      break;
    case trParking:
      s = "parking";
      break;
    case trBilling:
      s = "billing";
      break;
    case trPaid:
      s = "paid";
      break;
  }
  return s;
}

int Trans_SetState(TTransState newState, float paidAmount)
{
  int ret = 0;
  MASK_IRQ
    switch(newState)
    {
      case trIdle:
        if ( (Trans.State==trAuthen)||(Trans.State==trHandshake)||
          ((Trans.State==trPaid)&&(Trans.PaidStateDelaySec==0)) )
        {
          Trans.State = trIdle;
          ret = 1;
        }
        break;
      
      case trHandshake:
        if (!IsHardwareOkay())
          break;
        if ((Trans.State==trPaid)||(Trans.State==trIdle))
        {
          Trans.PaidStateDelaySec = 0;
          Trans.State = trHandshake;
          ret = 1;
        }
        break;
      
      case trAuthen:
        if (!IsHardwareOkay())
          break;
        if (Trans.State == trHandshake)
        {
            Trans.State = trAuthen;
            ret = 1;
        }
        else if (Trans.State == trAuthen)
        {
          if (Config.Private.OpMode.OpenAndFree || Trans.Authorized) 
          {            
            CountUpTimerReset();
            CountUpTimerStart();
            ResetPowerVar(); //reset meter
            Trans.State = trCharging;
            ret = 1;
          }
        }
        break;
        
      case trCharging:
        break;
      
      case trParking:
        if (Trans.State == trCharging)
        {
            if (Config.Private.OpMode.OpenAndFree) 
              Trans_SetState(trBilling, 0);
            else if (Trans.Bill.IsPayByRFIDCard)
              Trans_SetState(trBilling, 0);
            else
            {
              Trans.State = trParking;
              CountUpTimerReset(); //reset timer to time parking seconds
            }
            ret = 1;
        }
        break;
        
      case trBilling:
        if ((Trans.State==trCharging)||(Trans.State==trParking))
        {
          if (Config.Private.OpMode.OpenAndFree) 
            Trans_SetState(trPaid, 0);
          else 
          {
            Trans.State = trBilling;
          }
          CountUpTimerStop(); //stop counting parking seconds
          ret = 1;
          //tell user about end of charging
        }
        break;

      case trPaid:
        if ((Trans.State==trBilling)||(Trans.State==trCharging))
        {
          Trans.Authorized = FALSE;
          Trans.Bill.PaidAmount = paidAmount;
          Trans.Bill.IsPaid = 1;
          if (Config.Private.OpMode.OpenAndFree)
            Trans.PaidStateDelaySec = 0;
          else
            Trans.PaidStateDelaySec = 30; //delay 30s
          Trans.State = trPaid;
          ret = 1;
        } 
        break;
    }
  UNMASK_IRQ
  return ret;
}

void Trans_SetAuthorized(uint8_t authorized)
{
  MASK_IRQ
    Trans.Authorized = authorized;
  UNMASK_IRQ
}

float Trans_CheckBill(void)
{
  TBill *pBill;
  float parkingHr, payable;

  MASK_IRQ
    pBill = &Trans.Bill;
    if (Trans.State == trCharging)    
    {
      pBill->ChargingSec = CountUpTimerGet();        
      GetPowerVar(NULL, &pBill->Energy_kWh);
    }
    else if (Trans.State == trParking) //parking penality
    {
      pBill->ParkingMin = (CountUpTimerGet()+59)/60;  
      if (pBill->ParkingMin > Config.Public.Rates.FreeParking_min)
        pBill->ParkPenalty = (pBill->ParkingMin - Config.Public.Rates.FreeParking_min)*Config.Public.Rates.ParkPenalty_min;
      else
        pBill->ParkPenalty = 0;
    }
    pBill->EnergyFee = pBill->Energy_kWh*Config.Public.Rates.Energy_kWh; //energy fee
    parkingHr = (pBill->ChargingSec+59)/60/60;
    pBill->ParkingFee = parkingHr*Config.Public.Rates.Parking_hr; //parking fee
    if (pBill->IsPayByRFIDCard && Trans.Card.ChargeCard.IsFreeCard)
      pBill->PayableAmount = 0;
    else
      pBill->PayableAmount  = pBill->EnergyFee + pBill->ParkingFee + pBill->ParkPenalty;
    payable = pBill->PayableAmount;
  UNMASK_IRQ
  return payable;
}

float Trans_GetCardCredit(void)
{
  MASK_IRQ    
    float ret = Trans.Card.ChargeCard.Credit;
  UNMASK_IRQ
  return ret;
}

void Trans_SetCardCredit(float credit)
{
  MASK_IRQ
    Trans.Card.ChargeCard.Credit = credit;
  UNMASK_IRQ
}

uint8_t Trans_IsCreditOut(void)
{
  uint8_t ret = 0;
  if (Config.Private.OpMode.OpenAndFree)
    return 0;
  float payable = Trans_CheckBill();
  MASK_IRQ
    ret = payable >= Trans.Credit+1;
  UNMASK_IRQ
  return ret;
}

void Trans_AuthenRFIDCard(TCard *ACard)
{
  MASK_IRQ
    Trans.Authorized = TRUE;
    memset(&Trans.Bill, 0, sizeof(Trans.Bill));
    Trans.Bill.IsPayByRFIDCard = TRUE;    
    memcpy(Trans.Card.CardSn, ACard->Serial, sizeof(Trans.Card.CardSn));
    Trans.Card.ChargeCard = ACard->As.ChargeCard;
    Trans.Credit = ACard->As.ChargeCard.Credit;
  UNMASK_IRQ
}

void Trans_Authen(float credit, uint32_t delaySec)
{
  DlyTimerSet(delaySec);
  MASK_IRQ
    Trans.Authorized = TRUE;
    Trans.Credit = credit;
    memset(&Trans.Bill, 0, sizeof(Trans.Bill));
    Trans.Bill.IsPayByRFIDCard = FALSE;    
    memset(&Trans.Card, 0, sizeof(Trans.Card));
  UNMASK_IRQ
}

uint8_t Trans_IsSameCard(TCard *ACard)
{
  MASK_IRQ
    uint8_t ret = memcmp(ACard->Serial, Trans.Card.CardSn, sizeof(Trans.Card.CardSn))==0;
  UNMASK_IRQ
  return ret;
}

void Trans_GetBill(TBill *bill)
{
  float ret = Trans_CheckBill();
  MASK_IRQ
    *bill = Trans.Bill;
  UNMASK_IRQ  
}

uint8_t Trans_IsPayByCard(void)
{
  MASK_IRQ
    uint8_t ret = Trans.Bill.IsPayByRFIDCard;
  UNMASK_IRQ  
  return ret;
}

float Trans_GetPaidAmount(void)
{
  MASK_IRQ
    float ret = Trans.Bill.PaidAmount;
  UNMASK_IRQ  
  return ret;
}

void Trans_PaidStateDelayTick(void)
{
  MASK_IRQ
    if (Trans.PaidStateDelaySec>0)
      Trans.PaidStateDelaySec--;
  UNMASK_IRQ  
}
