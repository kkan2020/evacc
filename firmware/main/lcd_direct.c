#include <stdlib.h>
#include <stdint.h>
#include "hal.h"
#include "lcd_direct.h"

uint32_t LcdSReg = LCD_SREG_INIT_STATE;

void LcdSRegClockOut(void)
{
  uint32_t t, mask;  
  
  t = LcdSReg;
  mask = 1L<<(LCD_BUS_WIDTH-1);
  LcdTxSclPin(0);
  LcdResetStbPin(0);
  while (mask)
  {    
    if (t & mask)
      LcdRxSdaPin(1);
    else
      LcdRxSdaPin(0);
    LcdTxSclPin(1);
    __nop();    
    mask >>= 1;
    LcdTxSclPin(0);
    __nop();    
  }
  LcdResetStbPin(1);
  __nop();    
  LcdResetStbPin(0);
}

void LoadLcdData(uint8_t Value)
{
  LcdSReg &= ~LCD_DATA_MASK;
  LcdSReg |= ((uint16_t)Value) << 8;
}

void LcdBacklight(uint8_t state)       
{
	LcdBL_Pin(state?0:1);
}

void InitLcg(void)
{
  uint16_t t;

  ResetWDT();
  ClrLcdCtrlPins(LCD_RES);
  LcdSRegClockOut();
  for (t=0; t<1000; t++) {__nop();}; 
  ResetWDT();
  SetLcdCtrlPins(LCD_RES);
  LcdSRegClockOut();  
  for (t=0; t<1000; t++) {__nop();}; 
  ResetWDT();
  InitLcgDispMode();
  ResetWDT();
}