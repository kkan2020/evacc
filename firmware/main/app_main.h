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
 
#ifndef __APP_MAIN_H__
#define __APP_MAIN_H__
#include "mifare.h"
#include "RFID_Card.h"
#include "cmsis_os2.h"

#define EEP_NEXT_PAGE(addr)           (((addr+EEP_PAGE_SIZE-1)>>3)*EEP_PAGE_SIZE) /* >>3 for 8 bytes page*/
#define EEP_BLK_SIZE(x)               (sizeof(x)+2) /*add 2 bytes for crc*/

#define EEP_HW_UID_ADDR               0
#define EEP_DH_DEV_ID_ADDR            EEP_HW_UID_ADDR + EEP_BLK_SIZE(TUIdStr)
#define EEP_LABEL_ADDR                EEP_DH_DEV_ID_ADDR + EEP_BLK_SIZE(TDeviceIdStr)
#define EEP_POWER_ADDR                EEP_LABEL_ADDR + EEP_BLK_SIZE(TLabelStr)
#define EEP_OP_MODE_ADDR              EEP_POWER_ADDR + EEP_BLK_SIZE(TPower)
#define EEP_KEY48_PUB_ADDR            EEP_OP_MODE_ADDR + EEP_BLK_SIZE(TOpMode)
#define EEP_GROUP_ID_ADDR             EEP_KEY48_PUB_ADDR + EEP_BLK_SIZE(TKey48)
#define EEP_RATES_ADDR                EEP_GROUP_ID_ADDR + EEP_BLK_SIZE(TUIdStr)
#define EEP_DH_DEV_ADDR               EEP_RATES_ADDR + EEP_BLK_SIZE(TRates)            
#define EEP_SERVER_URL_ADDR           EEP_DH_DEV_ADDR + EEP_BLK_SIZE(TDhDevice)
#define EEP_DEVICE_JWT_ADDR           EEP_SERVER_URL_ADDR + EEP_BLK_SIZE(TUrlStr)
#define EEP_CLIENT_JWT_ADDR           EEP_DEVICE_JWT_ADDR + EEP_BLK_SIZE(TJwtStr)
#define EEP_WIFI_CONFIG_ADDR          EEP_CLIENT_JWT_ADDR + EEP_BLK_SIZE(TJwtStr)

#define IS_PANIC                      (IS_EM_STOP_PRESSED || IsCoverOpended())
#define IS_CABLE_OK                   (IS_CABLE_CONNECTED)
#define IS_POWER_MANAGED              (Config.Private.Power.IsManaged)

#define LONG_BEEP                     600
#define SHORT_BEEP                    70


/* DeviceHive config **************************************************************/
#define DEF_DH_DEV_TYPE                          7
#define DEF_DH_NT_ID                             1294

typedef void (*TRangeCheckFunc)(void *Obj);

extern __IO uint8_t CoverOpened;
extern __IO float CurTemperature;
extern const char DefDhDeviceNameStr[];
extern const char DefDevRefreshJwtStr[];
extern const char DefClientRefreshJwtStr[];
extern osThreadId_t CTThread_id, PilotThread_id, DhThread_id;
  
int IsCoverOpended(void);
int IsTempOutOfRange(void);
int IsHardwareOkay(void);
void RangeCheck_Power(void *obj);
void RangeCheck_Wifi(void *obj);
int EEP_ReadBlk(uint16_t eepromAddr, void *dest, uint16_t size, TRangeCheckFunc RangechckFunc);
int EEP_WriteBlk(uint16_t eepromAddr, void* src, uint16_t size, void* updateObj, TRangeCheckFunc RangechckFunc);
int EEP_ReadStringBlk(uint16_t eepromAddr, char *dest, uint16_t size);
int EEP_WriteStringBlk(uint16_t eepromAddr, char *srcStr, uint16_t size, char *updateStr);
void Beep(uint16_t duration);
void DoubleBeep(void);
void app_main(void *argument);
#ifdef EMULATION_ENABLED
void SetTempTestPin(uint8_t value);
#endif

#endif

