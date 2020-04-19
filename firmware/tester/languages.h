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
 
#ifndef  __LANGUAGES_H__
  #define __LANGUAGES_H__

#define GB10_PATTERN_SIZE     13
#define GB10_FONT_COUNT       27
#define GB10_MSG_COUNT         17
#ifndef FONT_SIZE_MASK
  #define FONT_SIZE_MASK              0xFF000000
#endif
#ifndef DLG_INDEX_MASK
  #define DLG_INDEX_MASK              0x00FFFFFF
#endif

#define HINT_SPLASH                    0x0A000000
#define HINT_NO_YES                    0x0A000001
#define HINT_CANCEL_CONFIRM            0x0A000002
#define HINT_RET_DN_CONFIRM            0x0A000003
#define HINT_UP_DN                     0x0A000004
#define HINT_RET_UP_DN                 0x0A000005
#define HINT_CANCEL_DN_UPDATE          0x0A000006
#define HINT_BACK_CONFIRM              0x0A000007
#define HINT_CANCEL_UPDATE             0x0A000008
#define HINT_RET                       0x0A000009
#define HINT_CHG_BAR                   0x0A00000A
#define HINT_MOVEMENT                  0x0A00000B
#define SYS_TIME                       0x0A00000C
#define UNIT_MSEC                      0x0A00000D
#define UNIT_SEC                       0x0A00000E
#define UNIT_PCS                       0x0A00000F
#define MSG_ABOUT                      0x0A000010

typedef struct
{
  const char *EngStr, *ChsStr;
} TDialog;

extern const unsigned char FontGB10[][13];
extern const TDialog DialogGB10[];

#endif
