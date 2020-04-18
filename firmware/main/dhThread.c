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
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include "stm32f10x.h"
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal.h"
#include "stm32f10x_it.h"
#include "dhThread.h"
#include "app_main.h"
#include "mem.h"
#include "PilotThread.h"
#include "crc32.h"
#include  "RFID_Card.h"
#include "jsmn.h"
#include "trans.h"
#include "CT_Thread.h"
#include "app_main.h"

osRtxThread_t dhThread_tcb;
uint64_t dhThreadStk[128];
const osThreadAttr_t dhThread_attr = { 
  .cb_mem = &dhThread_tcb,
  .cb_size = sizeof(dhThread_tcb),
  .stack_mem = dhThreadStk,  
  .stack_size = sizeof(dhThreadStk), 
  .priority = osPriorityHigh,
};

TSPort WiFiPort = {.Uart=WIFI_UART};
TSPort GprsPort = {.Uart=GPRS_UART};
osMessageQueueId_t SPortMsgQ;

const char resultJsonStr[] = "{\"action\":\"%s\",\"devId\":\"%s\",\"result\":%d}";
jsmntok_t tokens[80];
jsmn_parser jsparser;
char tokbuf[61];

typedef int(*TAction)(TSPort *port, const char *json, int tokenCount);
typedef struct
{
  const char *command;
  TAction action;
} TCommand;
  
int SendStr(TSPort *port, const char *s)
{
  if (port->TxSize==0) //is idle?
  {
    int len = strlen(s);
    if (len > PKT_PLAYLOAD_SIZE)
      return 0;    
    port->TxSize= len+4;
    port->TxPos = 0;
    memcpy(port->TxBuffer, s, len);
    CalcCrc32Blk(port->TxBuffer, port->TxSize);
    USART_ITConfig(port->Uart, USART_IT_TXE, ENABLE);      
    return 1;
  }
  return 0;
}

static int Send(TSPort *port, int size)
{
  if (size > PKT_PLAYLOAD_SIZE)
    return 0;
  while (port->TxSize!=0) //wait until idle
  {
    osDelay(2); 
  }
  port->TxSize= size+4;
  port->TxPos = 0;
  CalcCrc32Blk(port->TxBuffer, port->TxSize);
  USART_ITConfig(port->Uart, USART_IT_TXE, ENABLE);      
  return 1;
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
			strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

static void tokencpy(char *buf, int bufSize, const char *jsStr, jsmntok_t *tok)
{
  int len = tok->end-tok->start;
  if (len>=bufSize)
    len = sizeof(bufSize)-1;
  memcpy(buf, &jsStr[tok->start], len);
  buf[len] = 0;
}

int wifiRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"wifi/read\",\"wifiMode\":%d,\"ssid\":\"%s\",\"wpa2\":\"%s\"}";
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr,
    Config.Public.WifiConfig.Mode,
    Config.Public.WifiConfig.SsidStr,
    Config.Public.WifiConfig.Wpa2KeyStr
  );
  return Send(port, strlen((char*)port->TxBuffer));
}

int configRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"config/read\",\"devId\":\"%s\","
    "\"label\":\"%s\",\"typeId\":%d,\"netId\":%d,"
    "\"data\":{\"vac\":%d,\"iac\":%d,\"3Ph\":%s,\"pMan\":%s}}";  
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr, 
    Config.Private.DeviceIdStr,
    Config.Private.LabelStr, 
    Config.Public.DhDevice.DeviceType,
    Config.Public.DhDevice.NetworkId,
    //data object
    Config.Private.Power.ChargeVoltage, 
    Config.Private.Power.ChargeCurrentMax, 
    IS_3PHASE_POWER?"true":"false",
    Config.Private.Power.IsManaged?"true":"false"
  );    
  return Send(port, strlen((char*)port->TxBuffer));
}

int serUrlRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"serUrl/read\",\"serUrl\":\"%s\"}";
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr, 
    Config.Public.ServerUrlStr
  );
  return Send(port, strlen((char*)port->TxBuffer));
}

int devTokenRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"dT/read\",\"dt\":\"%s\"}";
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr, 
    Config.Public.DevRefreshJwtStr
  );
  return Send(port, strlen((char*)port->TxBuffer));
}

int cliTokenRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"cT/read\",\"t\":\"%s\"}";
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr, 
    Config.Public.ClientRefreshJwtStr
  );
  return Send(port, strlen((char*)port->TxBuffer));
}

int ratesRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"rates/read\",\"devId\":\"%s\","
    "\"tState\":\"%s\",\"kWh\":%0.4f,\"park_hr\":%0.4f,\"parkPen_min\":%0.4f,"
    "\"freePark_min\":%d,\"temp\":%0.4f}";
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr,
    Config.Private.DeviceIdStr,
    Trans_GetStateName(),
    Config.Public.Rates.Energy_kWh,
    Config.Public.Rates.Parking_hr,
    Config.Public.Rates.ParkPenalty_min,
    Config.Public.Rates.FreeParking_min,
    CurTemperature
  );
  return Send(port, strlen((char*)port->TxBuffer));
}

//{"kWh":%f, "park_hr":%f, "parkPen_min":%f, "freePark_min":%f}
int ratesWrite(TSPort *port, const char *json, int tokenCount)
{
  for (int i=3; i<tokenCount; i++) {
    tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
    i++;
    if (strcmp(tokbuf, "kWh") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      Config.Public.Rates.Energy_kWh = atof(tokbuf);
    }
    else if (strcmp(tokbuf, "park_hr") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      Config.Public.Rates.Parking_hr = atof(tokbuf);
    }
    else if (strcmp(tokbuf, "parkPen_min") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      Config.Public.Rates.ParkPenalty_min = atof(tokbuf);
    }
    else if (strcmp(tokbuf, "freePark_min") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      Config.Public.Rates.FreeParking_min = atof(tokbuf);
    }
  }
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "rates/write",
    Config.Private.DeviceIdStr, 0);
  return Send(port, strlen((char*)port->TxBuffer));
}

int powerRead(TSPort *port, const char *json, int tokenCount)
{
  float current, KWh;
  const char fmtStr[] = "{\"action\":\"power/read\",\"devId\":\"%s\","
    "\"tState\":\"%s\",\"v(V)\":%d,\"i(A)\":%0.4f,\"kWh\":%0.4f,\"temp\":%0.4f}";
  GetPowerVar(&current, &KWh);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr, 
    Config.Private.DeviceIdStr,
    Trans_GetStateName(),
    Config.Private.Power.ChargeVoltage,
    current,
    KWh,
    CurTemperature
  );      
  return Send(port, strlen((char*)port->TxBuffer));  
}

int stateRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"state/read\",\"devId\":\"%s\","
    "\"tState\":\"%s\",\"panic\":%s,\"hwError\":%s,\"lidOpen\":%s,\"temp\":%0.4f}";
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr, 
    Config.Private.DeviceIdStr,
    Trans_GetStateName(),
    IsPanic()?"true":"false",
    IsFatalError()?"true":"false",
    IsCoverOpended()?"true":"false",
    CurTemperature
  );        
  return Send(port, strlen((char*)port->TxBuffer));
}

//{"delay_s":%d, "deposit":%f}
int transAuthen(TSPort *port, const char *json, int tokenCount)
{
  unsigned int delay = 0;
  double deposit = 0;
  for (int i=3; i<tokenCount; i++) {
    tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
    i++;
    if (strcmp(tokbuf, "delay_s") == 0)
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      delay = round(atof(tokbuf));
    }
    else if (strcmp(tokbuf,  "deposit") == 0)
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      deposit = atof(tokbuf);
    }
  }
  Trans_Authen(deposit, delay);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "trans/authen",
    Config.Private.DeviceIdStr, 0);
  return Send(port, strlen((char*)port->TxBuffer));
}

int transStop(TSPort *port, const char *json, int tokenCount)
{
  Trans_SetState(trParking, 0);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "trans/stop",
    Config.Private.DeviceIdStr, 0);
  return Send(port, strlen((char*)port->TxBuffer));
}

int billRead(TSPort *port, const char *json, int tokenCount)
{
  const char fmtStr[] = "{\"action\":\"bill/read\",\"devId\":\"%s\","
    "\"tState\":\"%s\",\"chargeMin\":%0.4f,\"kWh\":%0.4f,\"energyFee\":%0.4f,\"parkMin\":%d,"
  "\"parkFee\":%0.4f,\"parkPen\":%0.4f,\"payable\":%0.4f,\"paidAmt\":%0.4f,\"isPaid\":%s,\"temp\":%0.4f}";

  TBill bill;
  Trans_GetBill(&bill);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr,
    Config.Private.DeviceIdStr,
    Trans_GetStateName(),
    (float)(bill.ChargingSec)/60,
    bill.Energy_kWh,
    bill.EnergyFee,
    bill.ParkingMin,
    bill.ParkingFee,
    bill.ParkPenalty,
    bill.PayableAmount,
    bill.PaidAmount,
    bill.IsPaid?"true":"false",
    CurTemperature
  );
  return Send(port, strlen((char*)port->TxBuffer));      
}

//{"amount":%f}
int billPay(TSPort *port, const char *json, int tokenCount)
{
  float amount;
  for (int i=3; i<tokenCount; i++) {
    tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
    i++;
    if (strcmp(tokbuf, "amount") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      amount = atof(tokbuf);
    }
  }
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "bill/pay",
    Config.Private.DeviceIdStr, 
    Trans_SetState(trPaid, amount)? 0: -1);
  return Send(port, strlen((char*)port->TxBuffer));
}

int meterRead(TSPort *port, const char *json, int tokenCount)
{
  float current;
  TBill bill;
  const char fmtStr[] = "{\"action\":\"meter/read\",\"devId\":\"%s\","
    "\"tState\":\"%s\",\"chargeMin\":%0.4f,\"current\":%0.4f,\"kWh\":%0.4f,"
  "\"energyFee\":%0.4f,\"parkFee\":%0.4f,\"temp\":%0.4f}";

  GetPowerVar(&current, NULL);
  Trans_GetBill(&bill);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, fmtStr,
    Config.Private.DeviceIdStr,
    Trans_GetStateName(),
    (float)(bill.ChargingSec)/60,
    current,
    bill.Energy_kWh,
    bill.EnergyFee,
    bill.ParkingFee,
    CurTemperature
  );
  return Send(port, strlen((char*)port->TxBuffer));      
}

int statePoll(TSPort *port, const char *json, int tokenCount)
{
  switch(Trans_GetState()) 
  {
    case trIdle:
    case trHandshake:
      return stateRead(port, json, tokenCount);
    case trAuthen:
      return ratesRead(port, json, tokenCount);
    case trCharging:
      return meterRead(port, json, tokenCount);
    case trParking:
    case trBilling:
      return billRead(port, json, tokenCount);
    case trPaid:
      return billRead(port, json, tokenCount);
  }
  return 1;
}

int resetError(TSPort *port, const char *json, int tokenCount)
{
  osThreadFlagsSet(PilotThread_id, PILOT_RESET_ERROR);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "error/reset",
    Config.Private.DeviceIdStr, 0);
  return Send(port, strlen((char*)port->TxBuffer));
}

#ifdef EMULATION_ENABLED
//{"stateId":%d}
int pilotStateWrite(TSPort *port, const char *json, int tokenCount)
{
  int state = -1;
  for (int i=3; i<tokenCount; i++) 
  {
    tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
    i++;
    if (strcmp(tokbuf, "stateId") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      state = atoi(tokbuf);
      break;
    }
  }
  if (state!=-1) 
    SetVoltage((TVoltageLevel)state);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "pilotState/write",
    Config.Private.DeviceIdStr, 
    state!=-1? 0: -1);
  return Send(port, strlen((char*)port->TxBuffer));
}

int tempTestSet(TSPort *port, const char *json, int tokenCount)
{
  int state = -1;
  for (int i=3; i<tokenCount; i++) 
  {
    tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
    i++;
    if (strcmp(tokbuf, "value") == 0) 
    {
      tokencpy(tokbuf, sizeof(tokbuf), json, &tokens[i]);
      state = atoi(tokbuf);
      break;
    }
  }
  if (state!=-1) 
    SetTempTestPin(state);
  snprintf((char*)port->TxBuffer, PKT_PLAYLOAD_SIZE, resultJsonStr, 
    "tempTest/set",
    Config.Private.DeviceIdStr, 
    state!=-1? 0: -1);
  return Send(port, strlen((char*)port->TxBuffer));
}
#endif

const TCommand Commands[] = 
{
  {"wifi/read", wifiRead},
  {"config/read", configRead},
  {"serUrl/read", serUrlRead},
  {"dT/read", devTokenRead},
  {"cT/read", cliTokenRead},
  {"rates/read", ratesRead},
  {"rates/write", ratesWrite},
  {"state/read", stateRead},
  {"trans/authen", transAuthen},
  {"trans/stop", transStop},
  {"bill/read", billRead},
  {"bill/pay", billPay},
  {"power/read", powerRead},
  {"state/poll", statePoll},
  {"meter/read", meterRead},
  {"error/reset", resetError},
#ifdef EMULATION_ENABLED
  {"pilotState/write", pilotStateWrite},
  {"tempTest/set", tempTestSet},
#endif  
};

void dhThread(void *arg)
{ 
  TSPort *port;
  osStatus_t osStatus;
//  static uint32_t flags;

  SPortMsgQ = osMessageQueueNew(2, sizeof(TSPort*), NULL);
  while(!SPortMsgQ);
	USART_Cmd(WIFI_UART, ENABLE);
  WiFi_Reset(); 	
  while(1)
  {		        
    osStatus = osMessageQueueGet(SPortMsgQ, &port, NULL, osWaitForever/*MsToOSTicks(10)*/);
    if (osStatus==osOK)
    {
      if (!ValidateCrc32Blk(port->RxBuffer, port->RxPos))
        continue;
      char *js = (char*)port->RxBuffer;
      js[port->RxPos-4] = 0; //remove crc bytes
      jsmn_init(&jsparser);
      int r = jsmn_parse(&jsparser, js, port->RxPos-4, tokens, sizeof(tokens)/sizeof(tokens[0]));
      if (r<0) //invalid json string
        continue;
      	/* Assume the top-level element is an object */
      if (r < 1 || tokens[0].type != JSMN_OBJECT)
        continue;      
      if (jsoneq(js, &tokens[1], "cmd") != 0) //first key must be "cmd"
        break;
      for (int i=0; i<sizeof(Commands)/sizeof(Commands[0]); i++)
        if (jsoneq(js, &tokens[2], Commands[i].command)==0)
        {
          Commands[i].action(port, js, r);
          break;
        }
    }
  }
}
