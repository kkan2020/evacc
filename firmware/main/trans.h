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
 
#ifndef __TRANS_H__
#define __TRANS_H__

#include <stdint.h>
#include "RFID_Card.h"

typedef enum {trIdle, trHandshake, trAuthen, trCharging, trParking, trBilling, trPaid} TTransState;

typedef struct
{
  uint32_t ChargingSec;
  uint32_t ParkingMin;
  float Energy_kWh;
  float EnergyFee, ParkingFee, ParkPenalty; 
  float PayableAmount, PaidAmount;
  uint8_t IsPayByRFIDCard, IsPaid;
} TBill;

typedef struct
{
  TTransState State;
  uint8_t Authorized;
  float Credit;
  TBill Bill;
  struct
  {
    TCardSn CardSn;
    TChargeCard ChargeCard;
  } Card;
  uint16_t PaidStateDelaySec;
} TTrans;

TTransState Trans_GetState(void);
char* Trans_GetStateName(void);
int Trans_SetState(TTransState newState, float paidAmount);
void Trans_SetAuthorized(uint8_t authorized);
float Trans_CheckBill(void);
float Trans_GetCardCredit(void);
void Trans_SetCardCredit(float credit);
uint8_t Trans_IsCreditOut(void);
void Trans_AuthenRFIDCard(TCard *ACard);
void Trans_Authen(float credit, uint32_t delaySec);
uint8_t Trans_IsSameCard(TCard *ACard);
void Trans_GetBill(TBill *bill);
uint8_t Trans_IsPayByCard(void);
float Trans_GetPaidAmount(void);
void Trans_PaidStateDelayTick(void);
#endif
