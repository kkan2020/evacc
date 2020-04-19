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
 
#ifndef __LCG_PAR_H__
	#define __LCG_PAR_H__

#include <stdint.h>
#include "mem.h"

#define PARTI_COUNT					2
#define PARTI_WIDTH					64
#define PARTI_PAGES					8
#define PAGE_HEIGHT					8
#define SCREEN_PAGES				PARTI_PAGES				
#define SCREEN_COLS					(PARTI_COUNT * PARTI_WIDTH)
#define SCREEN_ROWS					(SCREEN_PAGES * PAGE_HEIGHT)
#define HALF_FONT_WIDTH		  8
#define FULL_FONT_WIDTH			16
#define FONT_PAGES					2
#define BRUSH_WIDTH					8
#define BRUSH_COUNT					3
#define OFFSET_MASK					0x07
#define WND_COUNT					  1
#define WND_MSG_QUEUE_SIZE	5

#define LCG_BIAS					  0xa2
#define LCG_ADC						  0xa0
#define LCG_SHL						  0xc8
#define LCG_INIT_LN					0x40
#define LCG_PWR_CNTL1				0x26
#define LCG_PWR_CNTL2				0x2e
#define LCG_PWR_CNTL3				0x2f
#define LCG_R_SEL					  0x25
#define LCG_REF_V_MODE			0x81
#define LCG_REF_V_R					0x20
#define LCG_DISP_ON					0xaf
#define LCG_PG_ADDR_MASK			0xb0
#define LCG_COL_HI_ADDR_MASK	0x10
#define LCG_COL_LO_ADDR_MASK 	0x00

#define LCG_CONTRAST_MAX			63
#define LCG_CONTRAST_MIN			1
#define LCG_REG_RESISTOR_MAX	7
#define LCG_REG_RESISTOR_MIN	0

#define FONTS_COUNT					    2

#define TEXT_HIGH_LIGHT					(1<<0)
#define TEXT_WORD_WRAP					(1<<1)
#define TEXT_ALIGN_RIGHT				(1<<2)
#define TEXT_ALIGN_CENTER				(1<<3)

typedef enum {bsSolid, bsTransparent, bsHalfTone} TBrush;
typedef enum {penNormal, penErase, penXor} TPen;
typedef enum {crDefault, crHand, crHourGlass} TCursorType;
typedef enum {fiNoFill, fiFill} TFillType;
typedef enum {cpClipToClient, cpClipToParent} TClipType;
typedef enum {borNone, borSingle, borDouble} TBorder;
typedef enum {bkNormal, bkInverse, bkHalfTone, bkHalfToneInverse} TBackgnd;
typedef enum {font8x5, font10x10, font12x12} TFont;
typedef enum {lanEng, lanChs} TLanguage;

typedef uint8_t  THalfFont[FONT_PAGES][HALF_FONT_WIDTH];
typedef uint8_t  TFullFont[FONT_PAGES][FULL_FONT_WIDTH];
typedef int8_t TWndHandle;

typedef union
{
  struct
  {
    int16_t Row, Col;
  } Pt;
  uint32_t Whole;
} TPoint;
 
typedef struct
{
  TPoint TopLeft;
  int16_t Height, Width;
} TRect;

typedef struct 
{
  int16_t Rows, Cols;
  uint8_t *Pattern;
  uint8_t RowMargin, ColMargin;
} TBitmap;

typedef struct
{
  TBrush Brush;
  TPen Pen;
  TBitmap *BmpPtr;
  TRect UpdateRect;
} TCanvas; 

typedef struct TWndObject
{
  TWndHandle Handle;
  TRect DispRect;
  uint8_t Hidden;
  void (*ProcessMsg)(struct TWndObject *ObjPtr, TMsgItem *Msg);
  void (*Paint)(struct TWndObject *ObjPtr);
} TWndObject;

typedef void (*TWndFunc)(TWndObject *ObjPtr, TMsgItem *Msg);

typedef struct TWindow
{
  TRect WndRect;
  TCanvas *CanvasPtr;
  TBorder Border;
  TBackgnd Backgnd;
  void (*Paint)(struct TWindow *WndPtr);
  void (*ProcessMsg)(struct TWindow *WndPtr, TMsgItem *Msg);
  TRect ClientRect;
  uint8_t Handle, Assigned, Hidden;
  struct TWindow *Parent;
  uint8_t ObjectsCount;
  TWndObject **Objects, *Focus;
  char *Name;
  osMessageQueueId_t MsgQ_Id;
} TWindow;

typedef struct
{
  int16_t Page, Offset;
} TPageOffset;

typedef struct
{
  TCursorType CursorType;
  TPoint Point;
} TCursor;

typedef struct
{
  TWindow *WndPtr;
  TCursor Cursor;
  uint8_t StartLine;
} TScreen;

typedef struct
{
  int16_t Row, Col;
  TBitmap *BitmapPtr;
} TImage;

typedef struct
{
  uint8_t Rows, Cols;
  uint8_t RowMargin, ColMargin;
  void (*PaintPattern)(uint8_t *Pattern);
} TFontInfo;

extern volatile TLanguage CurLanguage;
extern TFont CurChsFont;
extern TWindow Windows[WND_COUNT];
extern TCanvas ScreenCanvas;
extern const TWindow ScreenWnd;
extern const TFontInfo FontInfo[FONTS_COUNT];
extern const TBitmap Brushes[BRUSH_COUNT];

void LcgCommand(uint8_t Command);
char PtInRect(TPoint P, TRect A);
TPoint MakePt(int16_t Row, int16_t Col);
TRect MakeRect(int16_t Row, int16_t Col, int16_t Height, int16_t Width);
TRect OffsetRect(TRect A, TPoint P);
TRect ScaleRect(TRect A, int16_t DiffHeight, int16_t DiffWidth);
TRect IntersectRects(TRect A, TRect W);
TRect UnionRects(TRect A, TRect W);
void ResetLcgDisplayMode(void);
void InitLcgDispMode(void);
void InitLcg(void);
void SetLcgContrast(uint8_t Value);
//void SetLcgRegResistor(uint8_t Value);
void PaintBitmap(TWndHandle WndHandle, TRect ClipRect, TBitmap *BmpPtr, int16_t IndentRow, int16_t IndentCol, TClipType ClipType);
void FillRect(TWndHandle WndHandle, TRect A, TBitmap *BitmapPtr, TClipType ClipType);
//void DrawPixel(TWndHandle WndHandle, int16_t Row, int16_t Col, TClipType ClipType);
void DrawHLine(TWndHandle WndHandle, TPoint P, int16_t Width, TClipType ClipType);
void DrawVLine(TWndHandle WndHandle, TPoint P, int16_t Height, TClipType ClipType);
void DrawRect(TWndHandle WndHanlde, TRect A, TFillType FillType, TClipType ClipType);
void DefaultWndPaint(TWindow *WndPtr);
void SetFocus(TWndHandle WndHandle, TWndObject *ObjPtr);
void OffsetWnd(TWndHandle WndHandle, int16_t Row, int16_t Col);
//void MoveWnd(TWndHandle WndHandle, int16_t Row, int16_t Col);
//void DefaultPaint(TWindow *WndPtr);
void ClearRect(TWndHandle WndHandle, TRect A, TBackgnd Backgnd, TClipType ClipType);
uint16_t TextWidth(char *Text);
char *GetDialog(uint32_t DialogMacro);
uint8_t FontHeight(TFont Font);
uint8_t FontWidth(TFont Font);
uint8_t DefFontHeight(void);
uint8_t DefFontWidth(void);
TFont DefFont(void);
TWndHandle CreateWnd(TWndHandle ParentHandle, char *Title, TRect WndRect, uint8_t InnerSpace, TCanvas *CanvasPtr, TBorder Border, TBackgnd Backgnd);
void PaintChar(TWndHandle WndHandle, TRect ClipRect, char *CharCode, TPen Pen, TClipType ClipType);
void OutText(TWndHandle WndHandle, TRect ClipRect, char *Text, int16_t Indent, uint8_t Attri, TClipType ClipType);
void RenderScreen(void);
void Repaint(void);

#endif

