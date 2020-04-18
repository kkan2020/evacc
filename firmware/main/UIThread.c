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
 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <alloca.h>
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal.h"
#include "lcg-par.h"
#include "UIThread.h"
#include "CT_Thread.h" 
#include "app_main.h"
#include "qrcode.h"
#include "trans.h"
#include "PilotThread.h"
#include "RFID_Card.h" 
#include "dialog.h"
#include "languages.h"

#define DELAY_SECOND(x)      (x)*1000/(MsPerTick*UI_UPDATE_DLY)
#define QRCODE_VERSION        11

osRtxThread_t UIThread_tcb;
uint64_t UIThreadStk[128+283]; //add 283*8 bytes for QR code generation 
const osThreadAttr_t UIThread_attr = { 
  .cb_mem = &UIThread_tcb,
  .cb_size = sizeof(UIThread_tcb),
  .stack_mem = UIThreadStk,  
  .stack_size = sizeof(UIThreadStk),
  .priority = osPriorityAboveNormal,
};

osMemoryPoolId_t  UIMemPool;  
osMessageQueueId_t UIMsgQ;

static TTransState TrState = trIdle;
static TTransState TrPrevState = trIdle;
static uint8_t TrStateChanged = 1;
static char StateName[16];
static int32_t StateMask;
static uint16_t messageDelay, backlightDelay;
static char textMessage[61];

#define QRCODE_WIDTH      (QRCODE_VERSION*4+17)
static uint8_t bitmapPattern[((QRCODE_WIDTH+7)>>3)*QRCODE_WIDTH];
static TBitmap qrCodeBitmap = {QRCODE_WIDTH, QRCODE_WIDTH, bitmapPattern};

uint32_t GetStateMask(void)
{
  int32_t ret=0;
  MASK_IRQ
    if (IsChargingReady())
      ret |= ST_READY;
    if (IsFatalError())
      ret |= ST_ERROR;
    if (IS_PANIC)
      ret |= ST_PANIC;
    if (IsCharging())
      ret |= ST_CHARGING;
    if (IsCoverOpended())
      ret |= ST_LID_OPEN;
    if (IsTempOutOfRange())
      ret |= ST_OVER_TEMP;      
  UNMASK_IRQ
  return ret;
}

char* writeInteger(char *line, int size, char* caption, int16_t value, char* unit)
{
  snprintf(line, size, "%s%d%s", caption, value, unit);
  return line;
}

char* writeBool(char *line, int size, char* caption, int8_t value)
{
  snprintf(line, size, "%s%s", caption, value? "YES":"NO");
  return line;
}

char* writeFloat(char *line, int size, char* caption, float value, char* unit)
{
  snprintf(line, size, "%s%.1f%s", caption, value, unit);
  return line;
}

void Send2UI(TUIMessage *pMsg)
{
  osMessageQueuePut(UIMsgQ, &pMsg, NULL, osWaitForever);
}

void updateScreenDebug1(void)
{
  TRect r;
  char line[40];
  float current, energy;
    
  r = Windows[0].WndRect;
  r.Height = DefFontHeight() + 1;
  ClearRect(0, r, bkNormal, cpClipToClient);
  
  snprintf(line, sizeof(line), "State:%s", StateName); 
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);  
  
  writeFloat(line, sizeof(line), "Temp:", CurTemperature, "C");
  r.TopLeft.Pt.Row += r.Height;   
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);  

  r.TopLeft.Pt.Row += r.Height; 
  writeInteger(line, sizeof(line), "Capactiy:", CapacityReserved, "A");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);

  r.TopLeft.Pt.Row += r.Height; 
  writeFloat(line, sizeof(line), "Duty Cycle:", PWM_GetDutyCycle()*100, "%");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);

  GetPowerVar(&current, &energy);
  r.TopLeft.Pt.Row += r.Height; 
  memset(line, ' ', sizeof(line));
  snprintf(line, sizeof(line), "%s%.1f%s", "Current:", current, "A");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);
  
  snprintf(line, sizeof(line), "%s%.4f%s", "Energy:", energy, "kWh");
  r.TopLeft.Pt.Row += r.Height; 
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);  
}

static void sec2HMS(uint32_t seconds, uint16_t *h, uint16_t *m, uint16_t *s) {
  *s = seconds % 60;
  seconds /= 60;
  *m = seconds % 60;
  *h = seconds / 60;
}

void ShowChargingPage(void)
{
  TRect r;
  char line[41];
  float current;
  int16_t startCol = 0;
  TBill bill;
  static int16_t level = 1;
   
  r = Windows[0].WndRect;
  ClearRect(0, r, bkNormal, cpClipToClient);

  //draw battery boundary
  r.TopLeft.Pt.Row = 7;
  r.TopLeft.Pt.Col = startCol;
  r.Width = 32;
  r.Height = 57;
  DrawRect(0, r, fiNoFill, cpClipToParent);
  
  r.TopLeft.Pt.Row = 0;
  r.TopLeft.Pt.Col = startCol + 11;
  r.Width = 10;
  r.Height = 8;
  DrawRect(0, r, fiNoFill, cpClipToParent);
  
  //fill battery levels
  r.TopLeft.Pt.Col = startCol;
  r.Width = 32;
  r.Height = 10;
  int i;
  for (i=1; i<=level; i++) 
  {
    r.TopLeft.Pt.Row = 64 - 10*i - (i-1)*1;
    if (r.TopLeft.Pt.Row < 7)
      r.TopLeft.Pt.Row = 7;
    DrawRect(0, r, fiFill, cpClipToParent);
  }
  level++;
  if (level>5)
    level = 1;
  
  //show charging parameters  
  r.TopLeft.Pt.Row = 0;
  r.TopLeft.Pt.Col = 31 + 2;
  r.Width = 128 - 32 - 2;

  char *text1 = GetDialog(MSG_TIME);
  r.Height = DefFontHeight() + 1;
    
  Trans_GetBill(&bill);      
  uint16_t h, m, s;
  sec2HMS(bill.ChargingSec, &h, &m, &s);
  TLanguage lang = GetLcgLanguage();
  if (lang==lanEng)
  {
    OutText(0, r, text1, 0, TEXT_ALIGN_RIGHT, cpClipToClient);
    r.TopLeft.Pt.Row += r.Height; 
    snprintf(line, sizeof(line), "%d:%d:%d", h, m, s);
  }
  else
  {
    snprintf(line, sizeof(line), "%s%d:%d:%d", text1, h, m, s);
  } 
  OutText(0, r, line, 0, TEXT_ALIGN_RIGHT, cpClipToClient);
  r.TopLeft.Pt.Row += r.Height; 

  GetPowerVar(&current, NULL);
  text1 = GetDialog(UNIT_CURRENT);
  char *text2 = GetDialog(UINT_TEMP);
  if (IsTempOutOfRange())
    snprintf(line, sizeof(line), "%s", GetDialog(MSG_OVER_TEMP));
  else
    snprintf(line, sizeof(line), "%.1f%s %.1f%s", CurTemperature, text2, current, text1);
  OutText(0, r, line, 0, TEXT_ALIGN_RIGHT, cpClipToClient);
  r.TopLeft.Pt.Row += r.Height; 
  
  text1 = GetDialog(UNIT_ENERGY);
  snprintf(line, sizeof(line), "%.4f%s", bill.Energy_kWh, text1);
  OutText(0, r, line, 0, TEXT_ALIGN_RIGHT, cpClipToClient);  
  r.TopLeft.Pt.Row += r.Height + FontHeight(font8x5); 

  text1 = GetDialog(MSG_FEE);
  char *ccy = GetDialog(UNIT_CCY);
  snprintf(line, sizeof(line), "%s%s%.2f", text1, ccy, bill.PayableAmount);
  r.TopLeft.Pt.Row += 1; //shift down 1 row for high lighting
  OutText(0, r, line, 0, TEXT_HIGH_LIGHT|TEXT_ALIGN_RIGHT, cpClipToClient);  
}

void ShowBillPage(void)
{
  TRect r;
  char line[40];
  TBill bill;
  char *text1, *text2;
    
  r = Windows[0].WndRect;
  ClearRect(0, r, bkNormal, cpClipToClient);

  Trans_GetBill(&bill);
  if (bill.IsPayByRFIDCard) 
    text1 = GetDialog(MSG_CHECK_CARD_TO_PAY);
  else 
    text1 = GetDialog(MSG_OPEN_APP_TO_PAY);
  r.Height = DefFontHeight() + 1;
  r.TopLeft.Pt.Row += 1; //shift down 1 row for high lighting
  OutText(0, r, text1, 0, TEXT_WORD_WRAP|TEXT_ALIGN_CENTER|TEXT_HIGH_LIGHT, cpClipToClient);
  r.TopLeft.Pt.Row += r.Height + 2; //shift down 2 rows for high lighting

  char *ccy = GetDialog(UNIT_CCY);
  text1 = GetDialog(MSG_PARKING);
  text2 = GetDialog(UINT_MIN);
  TLanguage lang = GetLcgLanguage();
  if (lang==lanEng)
  {
    OutText(0, r, text1, 0, 0, cpClipToClient);
    r.TopLeft.Pt.Row += r.Height; 
    snprintf(line, sizeof(line), "%d%s %s%.2f", bill.ParkingMin, text2, ccy, bill.ParkingFee+bill.ParkPenalty);      
  } 
  else
  {
    snprintf(line, sizeof(line), "%s%d%s%s%.2f", text1, bill.ParkingMin, text2, ccy, bill.ParkingFee+bill.ParkPenalty);  
  }
  OutText(0, r, line, 0, TEXT_ALIGN_RIGHT, cpClipToClient);
  r.TopLeft.Pt.Row += r.Height + 2; 

  text1 = GetDialog(MSG_ENERGY);
  text2 = GetDialog(UNIT_ENERGY);
  if (lang==lanEng)
  {
    OutText(0, r, text1, 0, 0, cpClipToClient);
    r.TopLeft.Pt.Row += r.Height; 
    snprintf(line, sizeof(line), "%.2f%s %s%.2f", bill.Energy_kWh, text2, ccy, bill.EnergyFee);
  }
  else
  {
    snprintf(line, sizeof(line), "%s%.2f%s%s%.2f", text1, bill.Energy_kWh, text2, ccy, bill.EnergyFee);
  }
  OutText(0, r, line, 0, TEXT_ALIGN_RIGHT, cpClipToClient);  
  r.TopLeft.Pt.Row += r.Height+1; 

  text1 = GetDialog(MSG_TOTAL);
  snprintf(line, sizeof(line), "%s %s%.2f", text1, ccy, bill.PayableAmount);
  r.TopLeft.Pt.Row += 1; //shift down 1 row for high lighting
  OutText(0, r, line, 0, TEXT_ALIGN_RIGHT|TEXT_HIGH_LIGHT, cpClipToClient);  
}

void ShowMultiText(char text[])
{
  int i, j, n, len;
  char line[41];

  TRect r = Windows[0].WndRect;
  ClearRect(0, r, bkNormal, cpClipToClient);  
  r.Height = DefFontHeight() + 1;
  len = MIN(strlen(text), sizeof(line)-1);
  for (i=n=0; i<len; i++)
  {
    if (text[i]==';')
      n++;
  }
  n++; //nr. of lines
  n = MIN(n, (int)(Windows[0].WndRect.Height/r.Height)); //constrains n <= max lines
  r.TopLeft.Pt.Row = (Windows[0].WndRect.Height - n*r.Height)/2; //center text vertically
  for (i=j=0; j<len; j++)
  {
    if (text[j]==';')
    {
      memcpy(line, &text[i], j-i);
      line[j-i] = 0;
      i = j+1;
      OutText(0, r, line, 0, TEXT_WORD_WRAP|TEXT_ALIGN_CENTER, cpClipToClient);  
      r.TopLeft.Pt.Row += r.Height; 
    }
  }
  snprintf(line, sizeof(line), "%s", &text[i]);
  OutText(0, r, line, 0, TEXT_WORD_WRAP|TEXT_ALIGN_CENTER, cpClipToClient);  
}

void MakeQRCodeBitmap(void)
{
  QRCode qrcode;
  uint16_t x, y;
  uint8_t mask, *row;
  static char buf[321+1];
//  const char fmtStr[] = "{\"i\":\"%s\",\"t\":%d,\"n\":%d,\"j\":\"%s\",\"s\":\"%s\"}";
    const char fmtStr[] = "{\"i\":\"%s\",\"j\":\"%s\",\"s\":\"%s\"}";
  
  int n = snprintf(buf, sizeof(buf), fmtStr, 
    Config.Private.DeviceIdStr,
//    Config.Public.DhDevice.DeviceType, 
//    Config.Public.DhDevice.NetworkId,
    Config.Public.ClientRefreshJwtStr,
    Config.Public.ServerUrlStr
  );
  if (n>0)
  {
    uint8_t *qrcodeBytes = alloca(qrcode_getBufferSize(QRCODE_VERSION));
    //can encode up to 321 bytes
    qrcode_initBytes(&qrcode, qrcodeBytes, QRCODE_VERSION, ECC_LOW, (uint8_t*)buf, n); 
    memset(bitmapPattern, 0, sizeof(bitmapPattern));
    for (y=0; y<qrcode.size; y++)
    {
      mask = 1 << (y % 8);
      row = &bitmapPattern[(y>>3)*QRCODE_WIDTH];
      for (x=0; x<qrcode.size; x++)
      {
        if (qrcode_getModule(&qrcode, x, y))
          row[x] |= mask;
      }
    }
  }
}

void ShowQRCode(uint8_t row, uint8_t col)
{
  PaintBitmap(0, Windows[0].WndRect, &qrCodeBitmap, row, col, cpClipToClient);
}

void ShowAuthenPage(void)
{
  TRect r = Windows[0].WndRect;
  ClearRect(0, r, bkNormal, cpClipToClient);  
  r.Height = DefFontHeight() + 1;
/*
  char *line = "SCAN QR Code";
  OutText(0, r, line, 0, TEXT_WORD_WRAP|TEXT_ALIGN_CENTER, cpClipToClient);  
*/    
  ShowQRCode(r.Height+(Windows[0].WndRect.Height - r.Height*2 - qrCodeBitmap.Rows)/2, 
    (Windows[0].WndRect.Width - qrCodeBitmap.Cols)/2);
/*  
  r.TopLeft.Pt.Row = Windows[0].WndRect.Height - r.Height-1;
  OutText(0, r, "or CHECK CARD", 0, TEXT_WORD_WRAP|TEXT_ALIGN_CENTER, cpClipToClient);  
*/  
}

void UpdateScreen(void)
{
  float fp, fb;
  char *text1, *text2, *text3;
  
  switch(TrState)
  {
    case trIdle:
      if (IsCoverOpended())
        text1 = GetDialog(MSG_LID_OPENED);        
      else if (StateMask & ST_PANIC)
        text1 = GetDialog(MSG_PANIC_BTN_PRESSED);        
      else if (StateMask & ST_ERROR)
        text1 = GetDialog(MSG_FATAL_ERROR);
      else if (StateMask & ST_OVER_TEMP)
        text1 = GetDialog(MSG_TEMP_OUT_OF_RANGE);
      else
        text1 = GetDialog(MSG_PLUG_IN_CHARGE_GUN);
      if (!IS_DEBUG_MODE)
        snprintf(textMessage, sizeof(textMessage), "%s", text1);
      else
      {
        text2 = GetDialog(MSG_DEBUG_MODE);
        snprintf(textMessage, sizeof(textMessage), "%s;%s", text1, text2);
      }
      ShowMultiText(textMessage);
      if (TrStateChanged)
        backlightDelay = DELAY_SECOND(30);
      break;
      
    case trHandshake:
      text1 = GetDialog(MSG_HANDSHAKING);
      snprintf(textMessage, sizeof(textMessage), "%s", text1);
      ShowMultiText(textMessage);
      backlightDelay = DELAY_SECOND(5); //backlight always on
      break;
    
    case trAuthen:
      if (!Config.Private.OpMode.OpenAndFree)
      {
        MakeQRCodeBitmap();
        ShowAuthenPage();
        backlightDelay = DELAY_SECOND(5); //backlight always on
      }
      break;

    case trCharging:
      ShowChargingPage();
      if (TrStateChanged)
        backlightDelay = DELAY_SECOND(60);//backlight on 1 min.
      break;

    case trParking:
      text1 = GetDialog(MSG_UNPLUG_TO_PAY);
      snprintf(textMessage, sizeof(textMessage), "%s", text1);
      ShowMultiText(textMessage);
      if (TrStateChanged)
        backlightDelay = DELAY_SECOND(180); //backlight on 3 min.
      break;
    
    case trBilling:
      ShowBillPage();
      backlightDelay = DELAY_SECOND(5); //backlight always on
      break;

    case trPaid:
      fp = Trans_GetPaidAmount();
      fb = Trans_GetCardCredit();
      text1 = GetDialog(UNIT_CCY);
      text2 = GetDialog(MSG_BALANCE);
      text3 = GetDialog(MSG_PAID);
      if (Trans_IsPayByCard())
        snprintf(textMessage, sizeof(textMessage), "%s %s%.2f;%s %s%.2f", text3, text1, fp, text2, text1, fb);
      else
        snprintf(textMessage, sizeof(textMessage), "%s %s%.2f", text3, text1, fp);
      ShowMultiText(textMessage);
      if (TrStateChanged)
        backlightDelay = DELAY_SECOND(30);
      break;
  }
  TrStateChanged = 0;
}


void UIThread(void *arg) 
{
  static TUIMessage *pMsg;
	static uint64_t renderCount, w;
  char *ccy, *text1;
  
  UIMsgQ = osMessageQueueNew(5, sizeof(uint32_t), NULL);
  while(!UIMsgQ);
  UIMemPool = osMemoryPoolNew(5, sizeof(TUIMessage), NULL);
  while(!UIMemPool);

  InitLcg();
  SetLcgContrast(26);
  SetLcgLanguage((TLanguage)Config.Private.OpMode.Language);
  CreateWnd(-1, 0, MakeRect(0, 0, SCREEN_ROWS, SCREEN_COLS), 0, &ScreenCanvas, borNone, bkNormal);
  LcdBackLight(ON);
  LedSetState(LED_READY_BIT|LED_CHARGE_BIT|LED_FAULT_BIT, OFF);    
  while(1) 
  {
    if (osMessageQueueGet(UIMsgQ, &pMsg, NULL, UI_UPDATE_DLY)==osOK)
    {
			switch(pMsg->Code)
      {        
        case MSG_UI_SHOW_BALANCE:
          ccy = GetDialog(UNIT_CCY);
          text1 = GetDialog(MSG_CARD_BALANCE);
          snprintf(textMessage, sizeof(textMessage), "%s;%s%.2f", text1, ccy, pMsg->As.Float);
          messageDelay = DELAY_SECOND(pMsg->Seconds);
          backlightDelay += messageDelay;
          break;

        case MSG_UI_INSUFF_BAL:
          text1 = GetDialog(MSG_INSUFFICIENT_BALANCE);
          snprintf(textMessage, sizeof(textMessage), "%s", text1);
          messageDelay = DELAY_SECOND(pMsg->Seconds);
          backlightDelay += messageDelay;
          break;
        
        case MSG_UI_FEE_UNPAID:
          text1 = GetDialog(MSG_FEE_UNPAID);
          snprintf(textMessage, sizeof(textMessage), "%s", text1);
          messageDelay = DELAY_SECOND(pMsg->Seconds);          
          backlightDelay += messageDelay;
          break;
        
        case MSG_UI_READ_CARD_ERROR:
          text1 = GetDialog(MSG_READ_CARD_EEROR);
          snprintf(textMessage, sizeof(textMessage), "%s", text1);
          messageDelay = DELAY_SECOND(pMsg->Seconds);          
          backlightDelay += messageDelay;
          break;
          
        case MSG_UI_WRITE_CARD_ERROR:
          text1 = GetDialog(MSG_WRITE_CARD_ERROR);
          snprintf(textMessage, sizeof(textMessage), "%s", text1);
          messageDelay = DELAY_SECOND(pMsg->Seconds);          
          backlightDelay += messageDelay;
          break;
        
        case MSG_UI_CREATE_KEY_CARD:
          text1 = GetDialog(MSG_MAKE_KEY_CARD);
          snprintf(textMessage, sizeof(textMessage), "%s", text1);
          messageDelay = DELAY_SECOND(pMsg->Seconds);          
          backlightDelay += messageDelay;
          break;
        
        default:
          break;
      }        
      if (pMsg)
        osMemoryPoolFree(UIMemPool, pMsg);	
    }
    
    StateMask = GetStateMask();
    TrPrevState = TrState;
    TrState = Trans_GetState();
    if (!TrStateChanged)
      TrStateChanged = TrState!=TrPrevState? TRUE: FALSE;
    
    switch(TrState)
    {
      case trIdle:
        snprintf(StateName, sizeof(StateName), "CLOSED");
        break;
      case trAuthen:
        snprintf(StateName, sizeof(StateName), "AUTHEN");
        break;
      case trCharging:
        snprintf(StateName, sizeof(StateName), "CHARGING");
        break;
      case trParking:
        snprintf(StateName, sizeof(StateName), "PARKING");
        break;        
      case trBilling:
        snprintf(StateName, sizeof(StateName), "BILLING");
        break;
      case trPaid:
        snprintf(StateName, sizeof(StateName), "PAID");
        break;
      default:
        StateName[0] = 0;
        break;
    }
    
		if (messageDelay)
      ShowMultiText(textMessage);		
    else    
	  {
//      updateScreenDebug1();
//      ShowAuthenPage();
//      backlightDelay = DELAY_SECOND(120);
//	    ShowChargingPage();    
  	  UpdateScreen();
    }
    
		w = osKernelGetTickCount();
		if (w - renderCount > UI_UPDATE_DLY)
		{
      RenderScreen();    
      renderCount = w;		
			
			if (messageDelay)
				messageDelay--;
			
			if (backlightDelay)
				backlightDelay--;
			
			LcdBackLight(backlightDelay?ON:OFF);    
			
			if (StateMask & ST_READY)
				LedSetState(LED_READY_BIT, ON);
			else
				LedSetState(LED_READY_BIT, OFF);      
			
			if (StateMask & (ST_ERROR|ST_LID_OPEN|ST_PANIC)) //non-recoverable error
				LedToggleState(LED_FAULT_BIT);
			else
				LedSetState(LED_FAULT_BIT, OFF);

			if (StateMask & ST_CHARGING)
				LedToggleState(LED_CHARGE_BIT);
			else
				LedSetState(LED_CHARGE_BIT, OFF);            
		}
  }  
}
