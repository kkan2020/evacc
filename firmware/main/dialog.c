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
 
#include <string.h>
#include "lcg-par.h"
#include "languages.h"
#include "dialog.h"


const TDialog* getDlgTable(uint32_t DlgMacro)
{
  DlgMacro = (DlgMacro & FONT_SIZE_MASK) >> 24;
  switch (DlgMacro)
  {
  #ifdef GB10_MSG_COUNT
    case 10:  
      CurChsFont = font10x10;      
      return DialogGB10;
  #endif
  #ifdef GB12_MSG_COUNT
    case 12:  
      CurChsFont = font12x12;
      return DialogGB12;
  #endif
  #ifdef GB16_MSG_COUNT
    case 16:  
      CurChsFont = font16x16;
      return DialogGB16;
  #endif
  #ifdef GB24_MSG_COUNT
    case 24:  
      CurChsFont = font24x24;
      return DialogGB24;
  #endif
    default:  
      return 0;
  }
}

char* GetDialog(uint32_t DialogMacro)
{
  const TDialog *dlgTab;
  
  dlgTab = getDlgTable(DialogMacro);
  
  if (!dlgTab)
    return 0;    
   
  DialogMacro &= DLG_INDEX_MASK;
  if (GetLcgLanguage() == lanChs)
    return (char*)dlgTab[DialogMacro].ChsStr;
  else
    return (char*)dlgTab[DialogMacro].EngStr;
}

void OutDlg(TWndHandle WndHandle, TRect ClipRect, uint32_t DlgMacro, uint8_t Attri)
{
  char *text;
  
  text = GetDialog(DlgMacro);
  if (text)
    OutText(WndHandle, ClipRect, text, 0, Attri, cpClipToClient);  
}

void ShowDlg(uint32_t DlgMacro, uint8_t Attri)
{
  OutDlg(0, Windows[0].ClientRect, DlgMacro, Attri);
  RenderScreen();
}

