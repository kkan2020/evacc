#ifndef __LCD_DIRECT_H__
#define __LCD_DIRECT_H__  
#include "hal.h"

#define LED_READY_PIN         (1L<<0)
#define LED_CHARGE_PIN        (1L<<1)
#define LED_FAULT_PIN         (1L<<2)
#define LED_ONLINE_PIN        (1L<<3)
#define LCD_DATA_MASK         0xff00
#define LCD_RD                (1L<<16)
#define LCD_WR                (1L<<17)
#define LCD_RS                (1L<<18)
#define LCD_RES               (1L<<19)
#define LCD_CS1               (1L<<20)
#define LCD_BL                (1L<<21)
#define LCD_BUS_WIDTH         22
#define LCD_SREG_INIT_STATE   (LCD_RD|LCD_WR|LCD_CS1)

#define ClrLcdCtrlPins(x)         LcdSReg &= ~(x)
#define SetLcdCtrlPins(x)         LcdSReg |= (x)
#define ToggleLcdCtrlPin(x)       LcdSReg ^= (x)

#define LED_ON(x)             ClrLcdCtrlPins(x)
#define LED_OFF(x)            SetLcdCtrlPins(x)
#define LED_TOGGLE(x)         ToggleLcdCtrlPin(x)

extern uint32_t LcdSReg;

void LcdSRegClockOut(void);
void LoadLcdData(uint8_t Value);
void LcdBacklight(uint8_t state);
void InitLcg(void);

#endif
