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

#define GB12_PATTERN_SIZE     18
#define GB12_FONT_COUNT       95
#define GB12_MSG_COUNT         34
#ifndef FONT_SIZE_MASK
  #define FONT_SIZE_MASK              0xFF000000
#endif
#ifndef DLG_INDEX_MASK
  #define DLG_INDEX_MASK              0x00FFFFFF
#endif

#define HINT_SPLASH                    0x0C000000
#define UNIT_SEC                       0x0C000001
#define UINT_MIN                       0x0C000002
#define UINT_HOUR                      0x0C000003
#define UNIT_CURRENT                   0x0C000004
#define UNIT_ENERGY                    0x0C000005
#define UINT_TEMP                      0x0C000006
#define UNIT_CCY                       0x0C000007
#define MSG_TIME                       0x0C000008
#define MSG_FEE                        0x0C000009
#define MSG_CREDIT                     0x0C00000A
#define MSG_MAKE_KEY_CARD              0x0C00000B
#define MSG_READ_CARD_EEROR            0x0C00000C
#define MSG_WRITE_CARD_ERROR           0x0C00000D
#define MSG_FEE_UNPAID                 0x0C00000E
#define MSG_INSUFFICIENT_BALANCE       0x0C00000F
#define MSG_CARD_BALANCE               0x0C000010
#define MSG_PLUG_IN_CHARGE_GUN         0x0C000011
#define MSG_HANDSHAKING                0x0C000012
#define MSG_UNPLUG_TO_PAY              0x0C000013
#define MSG_CHECK_CARD_TO_PAY          0x0C000014
#define MSG_OPEN_APP_TO_PAY            0x0C000015
#define MSG_PARKING                    0x0C000016
#define MSG_ENERGY                     0x0C000017
#define MSG_PENALTY                    0x0C000018
#define MSG_TOTAL                      0x0C000019
#define MSG_PAID                       0x0C00001A
#define MSG_BALANCE                    0x0C00001B
#define MSG_LID_OPENED                 0x0C00001C
#define MSG_PANIC_BTN_PRESSED          0x0C00001D
#define MSG_FATAL_ERROR                0x0C00001E
#define MSG_DEBUG_MODE                 0x0C00001F
#define MSG_OVER_TEMP                  0x0C000020
#define MSG_TEMP_OUT_OF_RANGE          0x0C000021

typedef struct
{
  const char *EngStr, *ChsStr;
} TDialog;

extern const unsigned char FontGB12[][18];
extern const TDialog DialogGB12[];

#endif
