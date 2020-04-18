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
 
#ifndef __UI_THREAD_H__
#define __UI_THREAD_H__
#include "app_main.h"
#include "trans.h"

#define MSG_UI_SHOWTEXT                   10
#define MSG_UI_TEMP                       11
#define MSG_UI_DEBUG                      12
#define MSG_UI_SHOW_BALANCE               13
#define MSG_UI_INSUFF_BAL                 14
#define MSG_UI_FEE_UNPAID                 15
#define MSG_UI_READ_CARD_ERROR                 16
#define MSG_UI_WRITE_CARD_ERROR                17
#define MSG_UI_CREATE_KEY_CARD            18

#define MSG_UI_TR_CLOSED                  20
#define MSG_UI_TR_AUTHEN                  21
#define MSG_UI_TR_CHARGING                22
#define MSG_UI_TR_BILLING                 23
#define MSG_UI_TR_PAID                    24
#define MSG_UI_TR_ERROR                   25

#define ST_ONLINE                         (1<<0)
#define ST_ERROR                          (1<<1)
#define ST_PANIC                          (1<<2)
#define ST_LID_OPEN                       (1<<3)
#define ST_OVER_TEMP                      (1<<4)
#define ST_CHARGING                       (1<<5)
#define ST_READY                          (1<<6)

typedef struct
{
  float DutyCycle;
  uint16_t CapacityOffer;
  char StateDesc[16];
} TDebugInfo;

typedef struct
{
  char Chars[sizeof(TBill)-1];
} TTextMsg;

typedef struct
{
  uint16_t Code;
//  int32_t Mask;
  union
  {
    int32_t Integer;
    float Float;
  } As;  
  uint8_t Seconds;
} TUIMessage;

extern osMessageQueueId_t UIMsgQ;
extern osMemoryPoolId_t  UIMemPool;  
extern const osThreadAttr_t UIThread_attr;

void Send2UI(TUIMessage *pMsg);
void UIThread(void *arg) ;

#endif
