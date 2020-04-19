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
 
#include <stdint.h>
#include <string.h>
#include "hal.h"
#include "lcg-par.h"
#include "../font/fonts8x5.h"
#include "languages.h"
#include "messages.h"

const uint8_t CellMasks[] = {0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff}; 
const uint8_t BrushSolid[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t BrushTransparent[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t BrushHalfTone[] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
const TBitmap Brushes[BRUSH_COUNT] =
{
  {8, 8, (uint8_t*)BrushSolid},
  {8, 8, (uint8_t*)BrushTransparent},
  {8, 8, (uint8_t*)BrushHalfTone}
};
const TBackgnd BackgndToInvBackgnd[] = {bkInverse, bkNormal, bkHalfToneInverse, bkHalfTone};
const TPen BackgndToTextPen[] = {penNormal, penErase, penNormal, penErase};

const TFontInfo FontInfo[FONTS_COUNT] =
{
  {8, 5, 1, 0},
  {10, 10, 0, 0}
};

volatile TLanguage CurLanguage = lanChs;
TFont CurChsFont = font10x10;
TWindow Windows[WND_COUNT];
uint8_t ShadowRams[SCREEN_PAGES][SCREEN_COLS];
const TBitmap ScreenBitmap = {SCREEN_ROWS, SCREEN_COLS, (uint8_t*)ShadowRams};
TCanvas ScreenCanvas = {bsSolid, penNormal, (TBitmap*)&ScreenBitmap, { {0}, SCREEN_ROWS, SCREEN_COLS}};

static TPageOffset RowToPageOffset(short Row)
{
  TPageOffset ps;
  short r;
  
  if (Row < 0)
    r = -Row;
  else
    r = Row;
  ps.Page = r >> 3;
  ps.Offset = r & OFFSET_MASK;
  if (Row < 0)
    ps.Page = -ps.Page;
  return ps;
}

TPoint MakePt(short Row, short Col)
{ 
  TPoint p;
  p.Pt.Row = Row;
  p.Pt.Col = Col;
  return p;
}

TRect MakeRect(short Row, short Col, short Height, short Width)
{
  TRect r;

  r.TopLeft.Pt.Row = Row;
  r.TopLeft.Pt.Col = Col;
  r.Height = Height;
  r.Width = Width;
  return r;
}

char PtInRect(TPoint P, TRect A)
{
  if ( (P.Pt.Row < A.TopLeft.Pt.Row) || (P.Pt.Col < A.TopLeft.Pt.Col) || 
  	(P.Pt.Row >= A.TopLeft.Pt.Row + A.Height) || (P.Pt.Col >= A.TopLeft.Pt.Col + A.Width) )
    return 0;
  return 1;
}

TRect IntersectRects(TRect A, TRect W)
{
  TRect t;
  TPoint botA, botW, p;

  memset(&t, 0, sizeof(TRect));

  if (!A.Height || !A.Width || !W.Height || !W.Width)
    return t;

  if (A.TopLeft.Pt.Row > W.TopLeft.Pt.Row)
    p.Pt.Row = A.TopLeft.Pt.Row;
  else
    p.Pt.Row = W.TopLeft.Pt.Row;

  if (A.TopLeft.Pt.Col > W.TopLeft.Pt.Col)
    p.Pt.Col = A.TopLeft.Pt.Col;
  else
    p.Pt.Col = W.TopLeft.Pt.Col;

  t.TopLeft.Whole = p.Whole;

  botA.Pt.Row = A.TopLeft.Pt.Row + A.Height - 1;
  botA.Pt.Col = A.TopLeft.Pt.Col + A.Width - 1;
  botW.Pt.Row = W.TopLeft.Pt.Row + W.Height - 1;
  botW.Pt.Col = W.TopLeft.Pt.Col + W.Width - 1;

  if (botA.Pt.Row > botW.Pt.Row)
    p.Pt.Row = botW.Pt.Row;
  else
    p.Pt.Row = botA.Pt.Row;

  if (botA.Pt.Col > botW.Pt.Col)
    p.Pt.Col = botW.Pt.Col;
  else
    p.Pt.Col = botA.Pt.Col;

  if ( (t.TopLeft.Pt.Row <= p.Pt.Row) && (t.TopLeft.Pt.Col <= p.Pt.Col) )
  {
    t.Height = p.Pt.Row - t.TopLeft.Pt.Row + 1;
	t.Width = p.Pt.Col - t.TopLeft.Pt.Col + 1;
  }
  return t;
}

TRect UnionRects(TRect A, TRect W)
{
  TRect t;
  TPoint botA, botW, p;

  if (!A.Height || !A.Width)
    return W;

  if (!W.Height || !W.Width)
    return A;

  if (A.TopLeft.Pt.Row > W.TopLeft.Pt.Row)
    p.Pt.Row = W.TopLeft.Pt.Row;
  else
    p.Pt.Row = A.TopLeft.Pt.Row;

  if (A.TopLeft.Pt.Col > W.TopLeft.Pt.Col)
    p.Pt.Col = W.TopLeft.Pt.Col;
  else
    p.Pt.Col = A.TopLeft.Pt.Col;

  t.TopLeft.Whole = p.Whole;

  botA.Pt.Row = A.TopLeft.Pt.Row + A.Height - 1;
  botA.Pt.Col = A.TopLeft.Pt.Col + A.Width - 1;
  botW.Pt.Row = W.TopLeft.Pt.Row + W.Height - 1;
  botW.Pt.Col = W.TopLeft.Pt.Col + W.Width - 1;

  if (botA.Pt.Row > botW.Pt.Row)
    p.Pt.Row = botA.Pt.Row;
  else
    p.Pt.Row = botW.Pt.Row;

  if (botA.Pt.Col > botW.Pt.Col)
    p.Pt.Col = botA.Pt.Col;
  else
    p.Pt.Col = botW.Pt.Col;

  t.Height = p.Pt.Row - t.TopLeft.Pt.Row + 1;
  t.Width = p.Pt.Col - t.TopLeft.Pt.Col + 1;
  return t;
}

TRect OffsetRect(TRect A, TPoint P)
{
  A.TopLeft.Pt.Row += P.Pt.Row;
  A.TopLeft.Pt.Col += P.Pt.Col;
  return A;
}

TRect ScaleRect(TRect A, int16_t DiffHeight, int16_t DiffWidth)
{
  A.TopLeft.Pt.Row -= DiffHeight;
  A.TopLeft.Pt.Col -= DiffWidth;
  A.Height += 2*DiffHeight;
  A.Width += 2*DiffWidth;
  return A;
}

void LcgCommand(uint8_t Command)
{
  LoadLcdData(Command);
  ClrLcdCtrlPins(LCD_RS|LCD_WR|LCD_CS1);      
  {__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();};
  SetLcdCtrlPins(LCD_WR|LCD_CS1);  
  {__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();};
}

void RenderScreen(void)
{
  uint8_t *ramPage;
  short t, lastCol;
  TPageOffset ps1, ps2;
  TRect r;

  r = MakeRect(0, 0, ScreenCanvas.BmpPtr->Rows, ScreenCanvas.BmpPtr->Cols);
  r = IntersectRects(r, ScreenCanvas.UpdateRect); //clip to canvas paintable rect
  ps1 = RowToPageOffset(r.TopLeft.Pt.Row);
  ps2 = RowToPageOffset(r.TopLeft.Pt.Row + r.Height - 1);
  lastCol = r.TopLeft.Pt.Col + r.Width;
  ramPage = ScreenCanvas.BmpPtr->Pattern + ps1.Page*ScreenCanvas.BmpPtr->Cols;
  //paint to LCG DDRAM

  for (; ps1.Page <= ps2.Page; ps1.Page++)
  {
    t = r.TopLeft.Pt.Col;
	LcgCommand( (uint8_t)(LCG_PG_ADDR_MASK | ps1.Page) );
	LcgCommand( LCG_COL_HI_ADDR_MASK | ((uint8_t)(t >> 4) & 0x0f) );
	LcgCommand( LCG_COL_LO_ADDR_MASK | ((uint8_t)t & 0x0f) );

	SetLcdCtrlPins(LCD_RS); //data mode
  for (; t < lastCol; t++)
	{
      LoadLcdData(ramPage[t]);
      ClrLcdCtrlPins(LCD_WR|LCD_CS1);
  	  {__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();};
      SetLcdCtrlPins(LCD_WR|LCD_CS1);
  	  {__nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();};
	}
	ramPage += ScreenCanvas.BmpPtr->Cols;
  }
  memset(&ScreenCanvas.UpdateRect, 0, sizeof(TRect));
}

static char Render(TCanvas *CanvasPtr, short Page, short Col, uint8_t CellMask)
{
  uint8_t *ram;
  unsigned short pages;

  if (!CanvasPtr || !CanvasPtr->BmpPtr)
    return 0;
  pages = (CanvasPtr->BmpPtr->Rows + PAGE_HEIGHT - 1) / PAGE_HEIGHT;
  if ( (Page < 0) || (Col < 0) || ((unsigned short)Page >= pages) || ((unsigned short)Col >= CanvasPtr->BmpPtr->Cols) )
    return 0;
  ram = CanvasPtr->BmpPtr->Pattern + Page*CanvasPtr->BmpPtr->Cols + Col;
  switch (CanvasPtr->Pen)
  {
	case penNormal:
      *ram |= CellMask;
	  break;
    case penErase:
	  *ram &= ~CellMask;
	  break;
 	case penXor:
      *ram ^= CellMask;
	  break;
  }
  return 1;
}

TPoint AddPts(TPoint X, TPoint Y)
{
  X.Pt.Row += Y.Pt.Row;
  X.Pt.Col += Y.Pt.Col;
  return X;
}

TPoint ClientToScreen(TWndHandle WndHandle, TPoint A)
{
  TWindow *p;
  TPoint t;

  if ( (WndHandle >= WND_COUNT) || (WndHandle < 0) )
    return A;
  p = &Windows[WndHandle];
  t.Whole = 0;
  while (p && p->Assigned)
  {
	t = AddPts(t, AddPts(p->WndRect.TopLeft, p->ClientRect.TopLeft));
	p = p->Parent;
  }
  A = AddPts(A, t);
  return A;
}
/*
void DrawPixel(TWndHandle WndHandle, short Row, short Col, TClipType ClipType)
{
  TPageOffset ps;
  TCanvas *canvasPtr;
  TRect r;

  if ( (WndHandle >= WND_COUNT) || !Windows[WndHandle].Assigned )
    return;
  canvasPtr = Windows[WndHandle].CanvasPtr;
  r = MakeRect(Row, Col, 1, 1);
  if (ClipType == cpClipToClient)
	r.TopLeft = ClientToScreen(WndHandle, r.TopLeft);
  r = IntersectRects(GetBmpRect(Windows[WndHandle].CanvasPtr->BmpPtr), r);
  if (!r.Height)
    return;
  ps = RowToPageOffset(r.TopLeft.Pt.Row);
  if ( Render(canvasPtr, ps.Page, r.TopLeft.Pt.Col, 1 << ps.Offset) )
    canvasPtr->UpdateRect = UnionRects(canvasPtr->UpdateRect, r);
}
*/
void FillRect(TWndHandle WndHandle, TRect A, TBitmap *BitmapPtr, TClipType ClipType)
{
  TPageOffset ps1, ps2;
  TCanvas *canvasPtr;
  TRect r;
  TPoint offPt;
  int32_t regLastCol, t, bmpStartCol, bmpStartRow, regStartCol;
  uint8_t mask;
  uint32_t i, bmpCurPage, prevPage, longMask, bmpRowPages;

  if ( (WndHandle >= WND_COUNT) || (WndHandle < 0) || !Windows[WndHandle].Assigned ) 
    return;

  offPt.Whole = A.TopLeft.Whole;

  if (ClipType == cpClipToParent)
  {
	if (!Windows[WndHandle].Parent) //no parent so boundary is WndRect
	{
	  WndHandle = 0;
  	  A = IntersectRects(A, Windows[0].WndRect); 
	  goto _paint;
	}
	else
	  WndHandle = Windows[WndHandle].Parent->Handle;
  }
  memcpy(&r, &Windows[WndHandle].ClientRect, sizeof(TRect));
  r.TopLeft.Whole = 0; //relative to client rect
  A = IntersectRects(A, r);
  A.TopLeft = ClientToScreen(WndHandle, A.TopLeft);

_paint:
  canvasPtr = Windows[WndHandle].CanvasPtr;
  r = MakeRect(0, 0, canvasPtr->BmpPtr->Rows, canvasPtr->BmpPtr->Cols);
  A = IntersectRects(A, r); //clip to canvas region

  if (!A.Height)
    return;

  regStartCol = A.TopLeft.Pt.Col;

  if (offPt.Pt.Col < 0)
 	bmpStartCol = -offPt.Pt.Col;
  else
    bmpStartCol = 0;

  if (offPt.Pt.Row < 0)
    bmpStartRow = -offPt.Pt.Row;
  else
    bmpStartRow = 0;

  ps1 = RowToPageOffset(A.TopLeft.Pt.Row);
  ps2 = RowToPageOffset(A.TopLeft.Pt.Row + A.Height - 1);

  regLastCol = regStartCol + A.Width;
  bmpRowPages = (BitmapPtr->Rows + PAGE_HEIGHT - 1) / PAGE_HEIGHT;
  bmpCurPage = bmpStartRow % bmpRowPages;

  if (ps1.Page == ps2.Page) 
  {
    for (i=bmpStartCol, t=regStartCol; t < regLastCol; t++)
	{  
	  mask = (BitmapPtr->Pattern[bmpCurPage*BitmapPtr->Cols+i] << ps1.Offset) & ((CellMasks[ps1.Offset]^CellMasks[ps2.Offset]) | (1 << ps1.Offset));
	  if ( !Render(canvasPtr, ps1.Page, t, mask) )
	    break;
	  i++;
	  i %= BitmapPtr->Cols;
  	}
  }
  else
  {
    for (i=bmpStartCol, t=regStartCol; t < regLastCol; t++)
	{
	  mask = (BitmapPtr->Pattern[bmpCurPage*BitmapPtr->Cols+i] << ps1.Offset) & (~CellMasks[ps1.Offset] | (1 << ps1.Offset));
      if ( !Render(canvasPtr, ps1.Page, t, mask) )
	    break;
	  i++;
	  i %= BitmapPtr->Cols;
  	}

	bmpCurPage = 1 % bmpRowPages;
	ps1.Page++;
    for (; ps1.Page < ps2.Page; ps1.Page++)
	{
      for (i=bmpStartCol, t=regStartCol; t < regLastCol; t++)
	  {
	    if (bmpCurPage > 0)
		  prevPage = bmpCurPage - 1;
		else
		  prevPage = bmpRowPages - 1;

		longMask = (BitmapPtr->Pattern[bmpCurPage*BitmapPtr->Cols+i] << PAGE_HEIGHT) | BitmapPtr->Pattern[prevPage*BitmapPtr->Cols + i];
		mask = longMask >> (PAGE_HEIGHT-ps1.Offset);
        if ( !Render(canvasPtr, ps1.Page, t, mask) )
	      break;
		i++;
	    i %= BitmapPtr->Cols;
	  }
	  bmpCurPage++;
	  bmpCurPage %= bmpRowPages;
	}

    for (i=bmpStartCol, t=regStartCol; t < regLastCol; t++)
	{
	  if (bmpCurPage > 0)
		prevPage = bmpCurPage - 1;
	  else
		prevPage = bmpRowPages - 1;

	  longMask = (BitmapPtr->Pattern[bmpCurPage*BitmapPtr->Cols+i] << PAGE_HEIGHT) | BitmapPtr->Pattern[prevPage*BitmapPtr->Cols + i];
	  mask = longMask >> (PAGE_HEIGHT-ps1.Offset);
	  mask = mask & CellMasks[ps2.Offset];
      if ( !Render(canvasPtr, ps2.Page, t, mask) )
	    break;
	  i++;
	  i %= BitmapPtr->Cols;
	}
  }
  canvasPtr->UpdateRect = UnionRects(canvasPtr->UpdateRect, A);
}

TRect GetBmpClipRect(TBitmap *BitmapPtr, short Row, short Col)
{
  return MakeRect(Row, Col, BitmapPtr->Rows - BitmapPtr->RowMargin,  BitmapPtr->Cols - BitmapPtr->ColMargin);
}

void PaintBitmap(TWndHandle WndHandle, TRect ClipRect, TBitmap *BmpPtr, short IndentRow, short IndentCol, TClipType ClipType)
{
  TRect r;

  if (!BmpPtr)
    return;
  r = GetBmpClipRect(BmpPtr, ClipRect.TopLeft.Pt.Row + IndentRow, ClipRect.TopLeft.Pt.Col + IndentCol);
  FillRect(WndHandle, r, BmpPtr, ClipType);
}

void DrawHLine(TWndHandle WndHandle, TPoint P, short Width, TClipType ClipType)
{
  TRect r;
  
  if ( (WndHandle >= WND_COUNT) || (WndHandle < 0) )
    return;
  r.TopLeft.Whole = P.Whole;
  if (Width < 0)
  {
	Width = -Width;
    r.TopLeft.Pt.Col -= Width - 1;
  }
  r.Height = 1;
  r.Width = Width;
  FillRect(WndHandle, r, (TBitmap*)&Brushes[bsSolid], ClipType);
}

void DrawVLine(TWndHandle WndHandle, TPoint P, short Height, TClipType ClipType)
{
  TRect r;
  
  if ( (WndHandle >= WND_COUNT) || (WndHandle < 0) )
    return;
  r.TopLeft.Whole = P.Whole;
  if (Height < 0)
  {
    Height = -Height;
	r.TopLeft.Pt.Row -= Height - 1;
  }
  r.Height = Height;
  r.Width = 1;
  FillRect(WndHandle, r, (TBitmap*)&Brushes[bsSolid], ClipType);
}

void DrawRect(TWndHandle WndHandle, TRect A, TFillType FillType, TClipType ClipType)
{
  TPoint p;

  if (!A.Height || !A.Width)
    return;
  if (FillType == fiFill)
	FillRect(WndHandle, A, (TBitmap*)&Brushes[bsSolid], ClipType);
  else
  {
    p.Whole = A.TopLeft.Whole;
    A.Width--;
    A.Height--;
    DrawHLine(WndHandle, p, A.Width, ClipType);
    p.Pt.Col += A.Width;
    DrawVLine(WndHandle, p, A.Height, ClipType);
    p.Pt.Row += A.Height;
    DrawHLine(WndHandle, p, -A.Width, ClipType);
    p.Pt.Col -= A.Width; 
    DrawVLine(WndHandle, p, -A.Height, ClipType);
  }
}

void ClearRect(TWndHandle WndHandle, TRect A, TBackgnd Backgnd, TClipType ClipType)
{
  TBrush bs;
  TPen pn, npn;

  pn = Windows[WndHandle].CanvasPtr->Pen;
  bs = (TBrush)0;
  npn = (TPen)0;
  switch (Backgnd)
  {
    case bkNormal:
	  bs = bsSolid;
	  npn = penErase;
	  break;
	case bkInverse:
	  bs = bsSolid;
	  npn = penNormal;
	  break;
	case bkHalfTone:
	case bkHalfToneInverse:
	  if (Backgnd == bkHalfTone)
	  {
	    Windows[WndHandle].CanvasPtr->Pen = penErase;
	    npn = penNormal;
	  }
	  else
	  {
	    Windows[WndHandle].CanvasPtr->Pen = penNormal;
		npn = penErase;
	  }
	  FillRect(WndHandle, A, (TBitmap*)&Brushes[bsSolid], ClipType);
	  bs = bsHalfTone;
	  break;
  }
  Windows[WndHandle].CanvasPtr->Pen = npn;
  FillRect(WndHandle, A, (TBitmap*)&Brushes[bs], ClipType);
  Windows[WndHandle].CanvasPtr->Pen = pn;
}

void DefaultWndPaint(TWindow *WndPtr)
{
  TRect r;
  uint8_t i, indent;
  
  if (WndPtr->Hidden)
    return;
  ClearRect(WndPtr->Handle, WndPtr->WndRect, WndPtr->Backgnd, cpClipToParent);

  //paint border
  switch (WndPtr->Border)
  {
    case borNone:
	  break;
	case borSingle:
	case borDouble:
	  memcpy(&r, &WndPtr->WndRect, sizeof(TRect));
	  if (WndPtr->Border == borSingle)
	    i = 1;
 	  else if (WndPtr->Border == borDouble)
	    i = 2;
	  else
	    i = 0;
	    
	  while (i--)
	  {
  	    DrawRect(WndPtr->Handle, r, fiNoFill, cpClipToParent);
		if (i)
	      r = ScaleRect(r, -1, -1);
	  }
	break;
  }
  //paint Name
  if (WndPtr->Name)
  {
    indent = TextWidth(WndPtr->Name);
	if (indent >= WndPtr->WndRect.Width)
	  indent = 1;
	else
	  indent = (WndPtr->WndRect.Width - indent)/2;
	if (WndPtr->Parent)
	  i = WndPtr->Parent->Handle;
	else
	  i = 0;
    memcpy(&r, &WndPtr->WndRect, sizeof(TRect));
    r.Height = DefFontHeight() + 1;
    OutText(i, r, WndPtr->Name, indent, TEXT_HIGH_LIGHT, cpClipToParent);
  }
}

void WndProcessMessage(TWindow *WndPtr, TMsgItem *Msg)
{
  uint8_t i;

  if (WndPtr->Hidden)
    return;
    
  if (Msg->Code == MSG_REPAINT)
  {
    WndPtr->Paint(WndPtr);
    for (i=0; i < WndPtr->ObjectsCount; i++)
      if (WndPtr->Objects[i]->Paint && !WndPtr->Objects[i]->Hidden)
        WndPtr->Objects[i]->Paint(WndPtr->Objects[i]);
  }
  else if (Msg->Code == MSG_ANIMATE)
  {
    for (i=0; i < WndPtr->ObjectsCount; i++)
      if (WndPtr->Objects[i]->ProcessMsg && !WndPtr->Objects[i]->Hidden)
        WndPtr->Objects[i]->ProcessMsg(WndPtr->Objects[i], Msg);
  }
  else
  {
    for (i=0; i < WndPtr->ObjectsCount; i++)
      if (WndPtr->Objects[i]->ProcessMsg)
	    WndPtr->Objects[i]->ProcessMsg(WndPtr->Objects[i], Msg);
  }
}

TWndHandle CreateWnd(TWndHandle ParentHandle, char *Name, TRect WndRect, uint8_t InnerSpace, TCanvas *CanvasPtr, TBorder Border, TBackgnd Backgnd)
{
  uint8_t i, titleRows;
  short d;

  for (i=0; i < WND_COUNT; i++)
    if (!Windows[i].Assigned)
      break;

  if (i >= WND_COUNT)
    return -1;

  Windows[i].Handle = i;
  Windows[i].Name = Name;
  Windows[i].Assigned = 1;
  Windows[i].CanvasPtr = CanvasPtr;
  Windows[i].Border = Border;
  Windows[i].Backgnd = Backgnd;
  Windows[i].Paint = DefaultWndPaint;
  Windows[i].ProcessMsg = WndProcessMessage;
  Windows[i].MsgQ_Id = osMessageQueueNew (WND_MSG_QUEUE_SIZE, sizeof(TMsgItem), NULL);
  if ( (ParentHandle < 0) || (ParentHandle >= WND_COUNT) )
    Windows[i].Parent = 0;
  else
    Windows[i].Parent = &Windows[ParentHandle];

  memcpy(&Windows[i].WndRect, &WndRect, sizeof(WndRect)); //WndRect references to parent's ClientRect

  switch (Border)
  {
    case borSingle:
      d = InnerSpace + 1;
   	  break;
    case borDouble:
      d = InnerSpace + 2;
      break;
    case borNone:
  	default:
      d = InnerSpace;
      break;
  }
  if (Name)
    titleRows = DefFontHeight();
  else
    titleRows = 0;
  //make ClientRect references to WndRect
  Windows[i].ClientRect.TopLeft.Pt.Row = d + titleRows;
  Windows[i].ClientRect.TopLeft.Pt.Col = d;
  Windows[i].ClientRect.Height = WndRect.Height - 2*d - titleRows;
  Windows[i].ClientRect.Width =  WndRect.Width - 2*d;
  return i;
}

void SetFocus(TWndHandle WndHandle, TWndObject *ObjPtr)
{
  if (WndHandle < WND_COUNT)
    Windows[WndHandle].Focus = ObjPtr;
}

void Repaint(void)
{
  uint32_t handle;
  TMsgItem msg;
  
  msg.Code = MSG_REPAINT;

  for (handle=0; handle < WND_COUNT; handle++)
    if (Windows[handle].Assigned && !Windows[handle].Hidden)
      Windows[handle].ProcessMsg(&Windows[handle], &msg);
  RenderScreen();
}

void OffsetWnd(TWndHandle WndHandle, short Row, short Col)
{
  if ( (WndHandle >= WND_COUNT) || (WndHandle < 0) )
    return;
  Windows[WndHandle].WndRect.TopLeft.Pt.Row += Row;
  Windows[WndHandle].WndRect.TopLeft.Pt.Col += Col;
}
/*
void MoveWnd(TWndHandle WndHandle, short Row, short Col)
{
  if ( (WndHandle >= WND_COUNT) || (WndHandle < 0) )
    return;
  Windows[WndHandle].WndRect.TopLeft.Pt.Row = Row;
  Windows[WndHandle].WndRect.TopLeft.Pt.Col = Col;
}
*/

uint8_t FontWidth(TFont Font)
{
  if (Font >= FONTS_COUNT)
    return 0;
  return FontInfo[Font].Cols - FontInfo[Font].ColMargin;
}

uint8_t FontHeight(TFont Font)
{
  if (Font >= FONTS_COUNT)
    return 0;
  return FontInfo[Font].Rows - FontInfo[Font].RowMargin;
}

TFont DefFont(void)
{
  if (CurLanguage == lanChs)
    return font10x10;
  else
    return font8x5;
}

uint8_t DefFontHeight(void)
{
  TFont f;
  f = DefFont();
  return FontInfo[f].Rows - FontInfo[f].RowMargin;
}

uint8_t DefFontWidth(void)
{
  TFont f;
  f = DefFont();
  return FontInfo[f].Cols - FontInfo[f].ColMargin;
}

uint16_t TextWidth(char *Text)
{
  int asian, west;
  
  if (!Text)
    return 0;
    
  asian = west = 0;
  while (*Text)
  {
    if (*Text++ < 0xA1)
      west++;
    else if (!(*Text))
      west++;
    else
    {
      if (*Text++ < 0xA1)
        west += 2;
      else
        asian++;
    }
  }       
  if (CurLanguage == lanEng)
    return (DefFontWidth() + 1) * (asian*2 + west);
  else
    return (DefFontWidth() + 1)*asian + (FontWidth(font8x5) + 1)*west;
}

const uint8_t BitMasks[] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

void FontToBmp(uint8_t *FontPat, TBitmap *Bitmap)
{
  int i, j, extraBits, exactBytes, mask, extraBytes, partitions;
  uint8_t c;

  exactBytes = Bitmap->Cols * (Bitmap->Rows/8);
  memcpy(Bitmap->Pattern, FontPat, exactBytes);
  
  extraBits = Bitmap->Rows % 8;
  if (!extraBits)
    return;

  mask = BitMasks[extraBits];

  i = Bitmap->Cols*extraBits;
  extraBytes = i % 8;
  if (extraBytes) 
    extraBytes = 1;
  else
    extraBytes = 0;
  extraBytes += i/8;
    
  
  partitions = 8/extraBits;
    
  for (i=0; i<extraBytes; i++)
  {
    c = FontPat[exactBytes+i];
    for (j=0; j<partitions; j++)
    {
      Bitmap->Pattern[i*partitions + exactBytes + j] = c & mask;
      c >>= extraBits;
    }
  }
}

void DecodeGB(char **Src, TBitmap *Bitmap)
{
  uint8_t *c;
  uint32_t i, col;
  TFont font;
  
  font = DefFont();
  Bitmap->Rows = FontInfo[font].Rows;
  Bitmap->Cols = FontInfo[font].Cols;
  Bitmap->RowMargin = FontInfo[font].RowMargin;
  Bitmap->ColMargin = FontInfo[font].ColMargin;
  
  c = (uint8_t*)*Src;
  (*Src)++;
  
  if (CurLanguage == lanEng)
    memcpy(Bitmap->Pattern, AsciiToBmp8x5(*c), Bitmap->Cols);  
  else if (CurLanguage == lanChs)
  {
    if ( (c[0] < 0xA1) || (c[1] < 0xA1) )
    {
      memcpy(Bitmap->Pattern, AsciiToBmp8x5(c[0]), FontInfo[font8x5].Cols);
  	  Bitmap->Cols = FontInfo[font8x5].Cols;
      if (font == font10x10)
      {
        //centerize font
  	    for (i=0; i<Bitmap->Cols; i++)
  	    {
  	      col = Bitmap->Pattern[i] << (FontInfo[font10x10].Rows - 8);
  	      Bitmap->Pattern[i] = (uint8_t)col;
  	      Bitmap->Pattern[i+Bitmap->Cols] = (uint8_t)(col >> 8);
  	    }
      }
    }
    else
    {
      (*Src)++;
      i = (c[0] - (uint8_t)0xA1) * 94 + c[1] - (uint8_t)0xA1;
      if (font == font10x10)
        FontToBmp((uint8_t*)FontGB10[i], Bitmap);
    }
  }
}

void PaintChar(TWndHandle WndHandle, TRect ClipRect, char *CharCode, TPen Pen, TClipType ClipType)
{
  TBitmap bmp;
  TPen oldPen;
  uint8_t buf[48];

  bmp.Pattern = buf;
  DecodeGB(&CharCode, &bmp);
  oldPen = Windows[WndHandle].CanvasPtr->Pen;
  Windows[WndHandle].CanvasPtr->Pen = Pen;
  PaintBitmap(WndHandle, ClipRect, &bmp, 0, 0, cpClipToClient);  
  Windows[WndHandle].CanvasPtr->Pen = oldPen;
}

void OutText(TWndHandle WndHandle, TRect ClipRect, char *Text, short Indent, uint8_t Attri, TClipType ClipType)
{

  TBitmap bmp;
  int16_t row, col;
  TBackgnd backgnd;
  TPen oldPen;
  uint8_t buf[48];
//  TFont font;
  
  if (!Text)
    return;

  //clear backgnd
  backgnd = Windows[WndHandle].Backgnd;
  if ( (Attri & TEXT_HIGH_LIGHT) && (backgnd<CountOf(BackgndToInvBackgnd)) )
    backgnd = BackgndToInvBackgnd[backgnd];
  ClearRect(WndHandle, ClipRect, backgnd, ClipType);
  
  if (Attri & TEXT_ALIGN_CENTER)
    Indent = ( ClipRect.Width - TextWidth(Text) ) / 2;
  else if (Attri & TEXT_ALIGN_RIGHT)
    Indent = ClipRect.Width - TextWidth(Text);

//  font = DefFont();
  bmp.Pattern = buf;
  row = 0;
  col = Indent;
  oldPen = Windows[WndHandle].CanvasPtr->Pen;
  Windows[WndHandle].CanvasPtr->Pen = BackgndToTextPen[backgnd];
  
  while ( (col < ClipRect.Width) && *Text ) 
  {
    DecodeGB(&Text, &bmp);
		if (Attri & TEXT_WORD_WRAP)
		{
		  if (col+bmp.Rows >= ClipRect.Width)
		  {
		    row += bmp.Rows + 1;
		    col = Indent;
		    if (row >= ClipRect.Height)
		      goto _exit;
		  }
		}
    PaintBitmap(WndHandle, ClipRect, &bmp, row, col, ClipType);
    col += bmp.Cols + 1;
  }
  
/*
  while (row < ClipRect.Height)
  {
 	while ( (col < ClipRect.Width) )
    {
      if (!*Text)
        goto _exit;
      DecodeGB(&Text, &bmp);
      PaintBitmap(WndHandle, ClipRect, &bmp, row, col, ClipType);
      col += bmp.Cols + 1;
    }
	if (!(Attri & TEXT_WORD_WRAP))
	  goto _exit;
	row += bmp.Rows + 1;
	col = Indent;
  }
*/  
_exit:
  Windows[WndHandle].CanvasPtr->Pen = oldPen;
}

const uint8_t LcgInitCmds1[] = {LCG_BIAS, LCG_ADC, LCG_SHL, LCG_INIT_LN};
const uint8_t LcgInitCmds2[] = {LCG_PWR_CNTL1, LCG_PWR_CNTL2, LCG_PWR_CNTL3};
const uint8_t LcgInitCmds3[] = 
{
  LCG_R_SEL, LCG_REF_V_MODE, LCG_REF_V_R, LCG_DISP_ON, LCG_PG_ADDR_MASK, 
  LCG_COL_HI_ADDR_MASK, LCG_COL_LO_ADDR_MASK
};

void InitLcgDispMode(void)
{
  int t;
  uint32_t w;
  //init display mode
  ResetWDT(); 
  for (t=0; t<sizeof(LcgInitCmds1); t++)
    LcgCommand(LcgInitCmds1[t]);

  //init power control 1
  for (t=0; t<sizeof(LcgInitCmds2); t++)
  {
    LcgCommand(LcgInitCmds2[t]);
//    osDelay(MsToOSTicks(60));
    w = SystemCoreClock/1000*60;
    while (w--) __nop();
    ResetWDT(); 
  }

  //init power control 2
  for (t=0; t<sizeof(LcgInitCmds3); t++)
    LcgCommand(LcgInitCmds3[t]);  
}

void InitLcg(void)
{
  uint16_t t;

  ResetWDT();
  ClrLcdCtrlPins(LCD_RES);
  for (t=0; t<1000; t++) {__nop();};      
  ResetWDT();
  SetLcdCtrlPins(LCD_RES);
  for (t=0; t<1000; t++) {__nop();};
  ResetWDT();
  InitLcgDispMode();
}

void SetLcgContrast(uint8_t Value)
{
  if (Value > LCG_CONTRAST_MAX)
    Value = LCG_CONTRAST_MAX;
  else if (Value < LCG_CONTRAST_MIN)
    Value = LCG_CONTRAST_MIN;
  LcgCommand(LCG_REF_V_MODE);
  LcgCommand(Value);
}
/*
void SetLcgRegResistor(uint8_t Value)
{
  if (Value > LCG_REG_RESISTOR_MAX)
    Value = LCG_REG_RESISTOR_MAX;
  LcgCommand(0x20|Value);
}
*/
