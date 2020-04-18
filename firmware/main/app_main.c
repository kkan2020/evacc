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
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include <ctype.h>
#include "stm32f10x.h"
#include "rtx_os.h"
#include "hal.h"
#include "UIThread.h"
#include "PilotThread.h"
#include "CT_Thread.h"
#include "RFIDThread.h"
#include "dhThread.h" 
#include "app_main.h"
#include "i2c_hw.h"
#include "crc16.h"
#include "ds18b20.h"
#include "stm32f10x_it.h"
#include "lcg-par.h"

const char DefDhDeviceNameStr[] = "EVACC v3.x";
const char DefDevRefreshJwtStr[] = "eyJhbGciOiJIUzI1NiJ9.eyJwYXlsb2FkIjp7ImEiOls0LDUsNiw4LDldLCJlIjoyNDcyMDMwNTM0NjgxLCJ0IjowLCJ1IjoxMjk3LCJuIjpbIjEyOTQiXSwiZHQiOlsiMCJdfX0.gAoHXUgOTLj2acfIttPLWqwKaJemcP0tfZ6Ey-ob2rw";
const char DefClientRefreshJwtStr[] = "eyJhbGciOiJIUzI1NiJ9.eyJwYXlsb2FkIjp7ImEiOlsxNywzLDQsNiw5XSwiZSI6MjQ3MjAzMDUzNDY4MSwidCI6MCwidSI6MTI5NywibiI6WyIxMjk0Il0sImR0IjpbIjAiXX19.yMFfs5-_BPqmeF2xKbSKBHKqIDcHgsWdkqZFeQKP8ys";
const char DefDhServerUrl[] = "http://playground.devicehive.com/api/rest";

const TPower DefPower =
{ 
  .ChargeCurrentMax = DEF_CHARGE_CURRENT,
  .ChargeVoltage = DEF_CHARGE_VOLTAGE,
  .CTAmperPerVolt = CT_AMPERE_PER_VOLT,
  .IsManaged = FALSE,
};

const TWifiConfig DefWifiConfig =
{
  .Mode = DEF_WIFI_MODE,
  .SsidStr = DEF_WIFI_SSID,
  .Wpa2KeyStr = "",
};

TI2C I2C2Port = {.Reg=I2C2, .SlaveAddr7=I2C2_SLAVE_ADDR7, .Timer=I2C2_TIMER};

osThreadId_t app_main_id, UIThread_id, PilotThread_id, CTThread_id, RFIDThread_id, DhThread_id; 

__IO uint8_t CoverOpened, TempOutOfRange, TempTestPin;
__IO float CurTemperature;

int IsTempOutOfRange(void) {
  return TempOutOfRange || TempTestPin;
}

#ifdef EMULATION_ENABLED
void SetTempTestPin(uint8_t value)
{
  TempTestPin = value;
}
#endif

int IsCoverOpended(void) 
{
  return CoverOpened && !IS_DEBUG_MODE;
}

int IsHardwareOkay(void)
{
   return IS_CABLE_OK && !IS_PANIC && !IsCoverOpended();
}

void AllTrim(char *str)
{
  char *end, *head;

  head = str;
  //  leading space
  while(isspace((unsigned char)*head)) head++;

  if(*head == 0)  // All spaces?
  {
    str[0] = 0;
    return;
  }
  //  trailing space
  end = head + strlen(head) - 1;
  while(end > head && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;
  
  if (head < str)
  {
    while (head <= end)
      *str++ = *head++;
    *str = 0;
  }
}

void WaitForRandowKeyReady(void)
{
  while (!IsRandomBytesReady)
    osDelay(MsToOSTicks(1000));
}

static int isPrintable(uint8_t value)
{
  return isalpha(value) || isdigit(value) || (value=='-');
}

static char GenPrintable(uint8_t value)
{
  struct _rand_state rstate;
  _srand_r(&rstate, value);
  do
  {
    value = _rand_r(&rstate) & 0xff;
  } while (!isPrintable(value));
  return (char)value;
}

void RegenUID(void)
{
  int i;
  for (i=0; i<UID_LEN; i++)
  {
    uint8_t c = RandomBytes[i];
    if (!isPrintable(c))
      Config.Private.HwUidStr[i] = GenPrintable(c);
    else
      Config.Private.HwUidStr[i] = c;
  }
  Config.Private.HwUidStr[UID_LEN] = 0;
}

void RegenDeviceIdStr(void)
{
  int i;
  for (i=0; i<DEV_UID_LEN; i++)
  {
    uint8_t c = RandomBytes[UID_LEN+i];
    if (!isPrintable(c))
      Config.Private.DeviceIdStr[i] = GenPrintable(c);
    else
      Config.Private.DeviceIdStr[i] = c;
  }
  Config.Private.DeviceIdStr[DEV_UID_LEN] = 0; //terminate the string
}

int EEP_ReadBlk(uint16_t eepromAddr, void *dest, uint16_t size, TRangeCheckFunc RangechckFunc)
{
  int i;
  size += 2; //add 2 bytes for crc
  uint8_t *blk = alloca(size); 
  for (i=0; i<3; i++)
  {
    if (!EEPRead(&I2C2Port, eepromAddr, blk, size))
      continue;
    if (ValidateCrcBlk(blk, size))
    {
      if (RangechckFunc)
        RangechckFunc(blk);
      MASK_IRQ
        memcpy(dest, blk, size-2);
      UNMASK_IRQ
      return 1;
    }
  }
  return 0;
}

int EEP_WriteBlk(uint16_t eepromAddr, void* src, uint16_t size, void* updateObj, TRangeCheckFunc RangechckFunc)
{
  int i;
  size += 2; //add 2 bytes for crc
  uint8_t *blk = alloca(size); 
  MASK_IRQ
    memcpy(blk, src, size-2);
  UNMASK_IRQ
  if (RangechckFunc)
    RangechckFunc(blk);
  CalcCrcBlk(blk, size);  
  for (i=0; i<3; i++)
  {
    if (!EEPWrite(&I2C2Port, eepromAddr, blk, size))
      continue;
    if (!EEPRead(&I2C2Port, eepromAddr, blk, size))
      continue;
    if (ValidateCrcBlk(blk, size))
    {
      if (updateObj)
      {
        MASK_IRQ
          memcpy(updateObj, blk, size-2);
        UNMASK_IRQ
      }
      return 1;
    }
  }
  return 0;
}

int EEP_ReadStringBlk(uint16_t eepromAddr, char *dest, uint16_t size)
{
  int ret = EEP_ReadBlk(eepromAddr, dest, size, NULL);
  if (ret)
  {
    MASK_IRQ   
      dest[size-1] = 0; //make sure dest is null terminaled
    UNMASK_IRQ
  }
  return ret;
}

int EEP_WriteStringBlk(uint16_t eepromAddr, char *srcStr, uint16_t size, char *updateStr)
{
  int ret = EEP_WriteBlk(eepromAddr, srcStr, size, updateStr, NULL);
  if (ret && updateStr)
  {
    MASK_IRQ   
      updateStr[size-1] = 0; //make sure updateStr is null terminated
    UNMASK_IRQ
  }
  return ret;  
}

void RangeCheck_Power(void *obj)
{
  TPower *pwr = obj;
  if (pwr->ChargeCurrentMax > CHARGE_CURRENT_MAX)
    pwr->ChargeCurrentMax = CHARGE_CURRENT_MAX;
  else if (pwr->ChargeCurrentMax < CHARGE_CURRENT_MIN)
    pwr->ChargeCurrentMax = CHARGE_CURRENT_MIN;      
  if (pwr->ChargeVoltage > CHARGE_VOLTAGE_MAX)
    pwr->ChargeVoltage = CHARGE_VOLTAGE_MAX;
  else if (pwr->ChargeVoltage < CHARGE_VOLTAGE_MIN)
    pwr->ChargeVoltage = CHARGE_VOLTAGE_MIN;
}

void RangeCheck_Wifi(void *obj)
{
  TWifiConfig *wifi = obj;
  if (wifi->Mode > 1)
    wifi->Mode = 0;
  wifi->SsidStr[SSID_LEN] = 0;
  wifi->Wpa2KeyStr[WPA2KEY_LEN] = 0;
}

void LoadConfigAll(void)
{
  memset(&Config, 0, sizeof(Config));
  /*** read private config ***/
#ifndef REGEN_UIDS  
  if (!EEP_ReadStringBlk(EEP_HW_UID_ADDR, Config.Private.HwUidStr, sizeof(TUIdStr)))
#endif    
  {
    WaitForRandowKeyReady();
    RegenUID();
    EEP_WriteStringBlk(EEP_HW_UID_ADDR, Config.Private.HwUidStr, sizeof(TUIdStr), NULL);
  }      
  
#ifndef REGEN_UIDS
  if (!EEP_ReadStringBlk(EEP_DH_DEV_ID_ADDR, Config.Private.DeviceIdStr, sizeof(TDeviceIdStr)))
#endif    
  {
    WaitForRandowKeyReady();
    RegenDeviceIdStr();
    EEP_WriteStringBlk(EEP_DH_DEV_ID_ADDR, Config.Private.DeviceIdStr, sizeof(TDeviceIdStr), NULL);
  }
  if (!EEP_ReadStringBlk(EEP_LABEL_ADDR, Config.Private.LabelStr, sizeof(Config.Private.LabelStr)))
  {
    Config.Private.LabelStr[0] = 0;
  }
  AllTrim(Config.Private.LabelStr);
  if (strlen(Config.Private.LabelStr)==0)
    strcpy(Config.Private.LabelStr, DefDhDeviceNameStr);
  
  if (!EEP_ReadBlk(EEP_POWER_ADDR, &Config.Private.Power, sizeof(Config.Private.Power), RangeCheck_Power))
  {
    EEP_WriteBlk(EEP_POWER_ADDR, (void*)&DefPower, sizeof(DefPower), &Config.Private.Power, RangeCheck_Power);
  }
  if (!EEP_ReadBlk(EEP_OP_MODE_ADDR, &Config.Private.OpMode, sizeof(Config.Private.OpMode), NULL))
  {
    Config.Private.OpMode.OpenAndFree = 1;    
    Config.Private.OpMode.Language = lanEng;
    EEP_WriteBlk(EEP_OP_MODE_ADDR, &Config.Private.OpMode, sizeof(Config.Private.OpMode), NULL, NULL);
  }
  
  /*** read public config ***/
  if (!EEP_ReadBlk(EEP_KEY48_PUB_ADDR, Config.Public.K48Pub, sizeof(Config.Public.K48Pub), NULL)) 
  {
    memset(Config.Public.K48Pub, 0xCC, sizeof(Config.Public.K48Pub)); //default K48Pub=0xCCCCCCCCCCCC  
    EEP_WriteBlk(EEP_KEY48_PUB_ADDR, Config.Public.K48Pub, sizeof(Config.Public.K48Pub), NULL, NULL);
  }  
  if (!EEP_ReadStringBlk(EEP_GROUP_ID_ADDR, Config.Public.GroupUidStr, sizeof(TUIdStr)))
  {
    memset(Config.Public.GroupUidStr, 0, sizeof(Config.Public.GroupUidStr));
    EEP_WriteStringBlk(EEP_GROUP_ID_ADDR, Config.Public.GroupUidStr, sizeof(TUIdStr), NULL);
  }    
  if (!EEP_ReadBlk(EEP_RATES_ADDR, &Config.Public.Rates, sizeof(Config.Public.Rates), NULL))  
  {
    memset(&Config.Public.Rates, 0, sizeof(Config.Public.Rates));
    EEP_WriteBlk(EEP_RATES_ADDR, &Config.Public.Rates, sizeof(Config.Public.Rates), NULL, NULL);
  }  
  if (!EEP_ReadBlk(EEP_DH_DEV_ADDR, &Config.Public.DhDevice, sizeof(Config.Public.DhDevice), NULL))
  {
    Config.Public.DhDevice.DeviceType = DEF_DH_DEV_TYPE;
    Config.Public.DhDevice.NetworkId = DEF_DH_NT_ID;
    EEP_WriteBlk(EEP_DH_DEV_ADDR, &Config.Public.DhDevice, sizeof(Config.Public.DhDevice), NULL, NULL);
  }  
#ifndef REFRESH_SER_URL    
  if (!EEP_ReadStringBlk(EEP_SERVER_URL_ADDR, Config.Public.ServerUrlStr, sizeof(Config.Public.ServerUrlStr)))
#endif    
  {
    strcpy(Config.Public.ServerUrlStr, (char*)DefDhServerUrl);    
    EEP_WriteStringBlk(EEP_SERVER_URL_ADDR, Config.Public.ServerUrlStr, sizeof(Config.Public.ServerUrlStr), NULL);
  }
  
#ifndef REFRESH_JWTS    
  if (!EEP_ReadStringBlk(EEP_DEVICE_JWT_ADDR, Config.Public.DevRefreshJwtStr, sizeof(Config.Public.DevRefreshJwtStr)))
#endif    
  {
    strcpy(Config.Public.DevRefreshJwtStr, DefDevRefreshJwtStr);
    EEP_WriteStringBlk(EEP_DEVICE_JWT_ADDR, Config.Public.DevRefreshJwtStr, sizeof(Config.Public.DevRefreshJwtStr), NULL);
  }
  
#ifndef REFRESH_JWTS      
  if (!EEP_ReadStringBlk(EEP_CLIENT_JWT_ADDR, Config.Public.ClientRefreshJwtStr, sizeof(Config.Public.ClientRefreshJwtStr)))
#endif    
  {
    strcpy(Config.Public.ClientRefreshJwtStr, DefClientRefreshJwtStr);
    EEP_WriteStringBlk(EEP_CLIENT_JWT_ADDR, Config.Public.ClientRefreshJwtStr, sizeof(Config.Public.ClientRefreshJwtStr), NULL);
  }
  
  if (!EEP_ReadBlk(EEP_WIFI_CONFIG_ADDR, &Config.Public.WifiConfig, sizeof(Config.Public.WifiConfig), RangeCheck_Wifi))
  {
    EEP_WriteBlk(EEP_WIFI_CONFIG_ADDR, (void*)&DefWifiConfig, sizeof(DefWifiConfig), &Config.Public.WifiConfig, RangeCheck_Wifi);
  }  
}
  
void CheckCoverState(void)
{
  static uint8_t Counter, good;
  
	good = 0;
  IR_PULSE_TX(ON);
  osDelay(MsToOSTicks(10));
  if (IR_PULSE_RX == 1)
  {
    IR_PULSE_TX(OFF);
    osDelay(MsToOSTicks(10));
    if (IR_PULSE_RX == 0)
    {
      if (Counter > 0)
        Counter--;
      if (Counter==0)
        CoverOpened = 0;
			good = 1;
    }
  }
		
	if (!good)
	{
		if (Counter < 5)                       
			Counter++;
		else
			CoverOpened = 1;
	}
  IR_PULSE_TX(OFF);
}

static void CheckTemperature(void)
{
  static uint8_t Counter;
  static float margin;
  
  if (GetTemp(&CurTemperature))
  {
    if (((CurTemperature>=WORKING_TEMP_MIN+margin)&&(CurTemperature<=WORKING_TEMP_MAX-margin)))
    {
      if (Counter > 0)
        Counter--;
      if (Counter==0) 
      {
        TempOutOfRange = 0;
        margin = 0.0;
      }
    }
    else if (Counter < 3)
    {
      Counter++;
      if (Counter==3) 
      {
        TempOutOfRange = 1;
        margin = TEMP_HYSTERESIS;
      }
    }
  }
  ConvertTemp();  
}

void Beep(uint16_t duration)
{
	Buzzor(ON);
  osDelay(MsToOSTicks(duration));
	Buzzor(OFF);
}

void DoubleBeep(void)
{
  Beep(SHORT_BEEP);
  osDelay(MsToOSTicks(50));
  Beep(SHORT_BEEP);
}  

void app_main(void *argument) {
  uint32_t param = NULL;
  static uint16_t tempCounter=1;
  
  InitEEP();
  LoadConfigAll();
 
  UIThread_id = osThreadNew(UIThread, &param, &UIThread_attr); 
  while(!UIThread_id);   
  
  PilotThread_id = osThreadNew(PilotThread, &param, &PilotThread_attr); 
  while(!PilotThread_id);
  
  CTThread_id = osThreadNew(CTThread, &param, &CTThread_attr); 
  while(!CTThread_id);

  RFIDThread_id = osThreadNew(RFIDThread, &param, &RFIDThread_attr); 
  while(!RFIDThread_id);

  DhThread_id = osThreadNew(dhThread, &param, &dhThread_attr);
	if (!DhThread_id)
    while(1);

  osThreadSetPriority(osThreadGetId(), osPriorityLow);
  ConvertTemp();
  osThreadFlagsSet(CTThread_id, CT_UPD_VARS);
  osThreadFlagsSet(PilotThread_id, PILOT_UPD_VARS);
  
  while(1)
  {
    /* Check if cover is opened*/
    CheckCoverState();
    
    /* Check if temperature in range */
    if (tempCounter==0)
    {
      CheckTemperature();			
    }
    if (++tempCounter>=20) //check per 5 seconds
      tempCounter = 0;
    osDelay(MsToOSTicks(100));
  }
}

