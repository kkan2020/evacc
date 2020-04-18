#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "utils.h"

const char HexChars[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

__inline void DelayLoop(uint32_t Count)
{
  while (Count--);
} 

uint8_t ByteToHexStr(char *Dest, uint8_t Value)
{
  Dest[0] = HexChars[Value>>4];
  Dest[1] = HexChars[Value & 0x0f];
  Dest[2] = 0;
  return 2;
}

static uint8_t HexValue(char Hex)
{
  uint8_t i;
  for (i=0; i<sizeof(HexChars); i++)
    if (Hex == HexChars[i])
      return i;
  return 0;
}

uint8_t HexStrToByte(char *Hex)
{
  uint8_t value = HexValue(Hex[0])*16;
  value += HexValue(Hex[1]);
  return value;
}


//up to ten thousands
//return result string length
uint16_t IntToStr(char *Dest, int32_t Value) //return length of result string
{
  unsigned char i=0, written=0;

  if (Value < 0)
  {
    Value = -Value;
    *Dest = '-';
    i = 1;
  }
  if (Value >=100000)
  {
    strcpy(Dest, "***");
    return 3;
  }
  if (Value >= 10000)
  {
	  Dest[i++] = Value/10000 + '0';
		written = 1;
	  Value %= 10000;
  }
  if ((Value >= 1000)||written)
  {
		Dest[i++] = Value/1000 + '0';
		written = 1;
		Value %= 1000;
  }
  if ((Value >= 100)||written)
  {
		Dest[i++] = Value/100 + '0';
		written = 1;
		Value %= 100;
  }
  if ((Value >= 10)||written)
  {
		Dest[i++] = Value/10 + '0';
		written = 1;
		Value %= 10;
  }
  Dest[i++] = Value + '0';
  Dest[i] = 0;
  return i;
}

int32_t StrToInt(char* Src)
{
  int32_t i, val;
  
  if (!Src)
    return 0;  
  val = 0;
  for (i=0; Src[i]; i++)
  {
    if (Src[i]== ' ')
      continue;
    if ( (Src[i] < '0')||(Src[i]>'9') )
      break;
    val = val*10 + Src[i]-'0';
  }    
  return val;
}

//return result string length
uint32_t FloatToStr(char *Buf, float Value, uint32_t DecimalPt)
{
  int32_t intPart, deciPart;
  uint32_t size;
  
  intPart = Value;
  Value -= (float)intPart;
  while (DecimalPt--)
    Value *= 10;
  deciPart = Value;
  size = IntToStr(Buf, intPart);
  Buf[size++] = '.';
  size += IntToStr(Buf+size, deciPart);
  return size;
}

/*
void Decrypt(char *Buf, uint8_t Size)
{
  uint8_t i, c;
  unsigned short Key;

  Key = ENCRYPT_KEY;
  for (i=0; i<Size ; i++)
  {
    c = (uint8_t)Buf[i];
    Buf[i] = (char)( c ^ (Key >> 8) );
    Key = ((c + Key) * ENCRYPT_C1 + ENCRYPT_C2) & 0x7FFF;
  }
}

void Encrypt(char *Buf, uint8_t Size)
{  
  uint8_t i, c;
  unsigned short Key;

  Key = ENCRYPT_KEY;
  for (i=0; i<Size ; i++)
  {
    c = (uint8_t)Buf[i];
    Buf[i] = (char)( (uint8_t)Buf[i] ^ (Key >> 8) );
    Key = ( ((uint8_t)Buf[i] + Key) * ENCRYPT_C1 + ENCRYPT_C2 ) & 0x7FFF;
  }
}
*/

uint32_t BcdToInt(TBinDigit BinDigit)
{
  return BinDigit.Bcd.Hi*10 + BinDigit.Bcd.Lo;
}

TBinDigit IntToBcd(uint32_t Value)
{
  TBinDigit bin;

  bin.Byte = 0;
  bin.Bcd.Hi =  Value / 10;
  bin.Bcd.Lo = Value - bin.Bcd.Hi*10;
  return bin;  
}

//return result string length
uint32_t DateToStr(char *Buf, TDateTime *DateTimePtr)
{
  Buf[0] = DateTimePtr->Year.Bcd.Hi + '0';
  Buf[1] = DateTimePtr->Year.Bcd.Lo + '0';
  Buf[2] = '/';
  Buf[3] = DateTimePtr->Month.Bcd.Hi + '0';
  Buf[4] = DateTimePtr->Month.Bcd.Lo + '0';
  Buf[5] = '/';
  Buf[6] = DateTimePtr->Day.Bcd.Hi + '0';
  Buf[7] = DateTimePtr->Day.Bcd.Lo + '0';
  Buf[8] = 0;
  return 8;
}

uint32_t DateToFullYearStr(char *Buf, TDateTime *DateTimePtr)
{
  Buf[0] = '2';
  Buf[1] = '0';
  return 2+DateToStr(Buf+2, DateTimePtr);
}

//return result string length
uint32_t TimeToStr(char *Buf, TDateTime *DateTimePtr)
{
  Buf[0] = DateTimePtr->Hour.Bcd.Hi + '0';
  Buf[1] = DateTimePtr->Hour.Bcd.Lo + '0';
  Buf[2] = ':';
  Buf[3] = DateTimePtr->Min.Bcd.Hi + '0';
  Buf[4] = DateTimePtr->Min.Bcd.Lo + '0';
  Buf[5] = ':';
  Buf[6] = DateTimePtr->Sec.Bcd.Hi + '0';
  Buf[7] = DateTimePtr->Sec.Bcd.Lo + '0';
  Buf[8] = 0;
  return 8;
}

//return result string length
uint32_t TimeSchToStr(char *Buf, TTimeSch *SchPtr)
{
  Buf[0] = SchPtr->Hour.Bcd.Hi + '0';
  Buf[1] = SchPtr->Hour.Bcd.Lo + '0';
  Buf[2] = ':';
  Buf[3] = SchPtr->Min.Bcd.Hi + '0';
  Buf[4] = SchPtr->Min.Bcd.Lo + '0';
  Buf[5] = 0;
  return 5;
}

void StrToTimeSch(TTimeSch *SchPtr, char *Text)
{
  SchPtr->Hour.Bcd.Hi = Text[0] - '0';
  SchPtr->Hour.Bcd.Lo = Text[1] - '0';  
  SchPtr->Min.Bcd.Hi = Text[3] - '0';
  SchPtr->Min.Bcd.Lo = Text[4] - '0';
}

//return result string length
uint32_t DateTimeToStr(char *Buf, TDateTime *DateTimePtr)
{
  DateToStr(Buf, DateTimePtr);
  Buf[8] = ' ';
  TimeToStr(&Buf[9], DateTimePtr);
  return 17;
}

void StrToDate(TDateTime *DateTimePtr, char *Text)
{
  DateTimePtr->Year.Bcd.Hi = Text[0] - '0';
  DateTimePtr->Year.Bcd.Lo = Text[1] - '0';
  
  DateTimePtr->Month.Bcd.Hi = Text[3] - '0';
  DateTimePtr->Month.Bcd.Lo = Text[4] - '0';
  
  DateTimePtr->Day.Bcd.Hi = Text[6] - '0';
  DateTimePtr->Day.Bcd.Lo = Text[7] - '0';
}

void StrToTime(TDateTime *DateTimePtr, char *Text)
{
/*  
  char buf[2];
  
  buf[1] = 0;
  buf[0] = Text[0];
*/  
  DateTimePtr->Hour.Bcd.Hi = Text[0] - '0';
  DateTimePtr->Hour.Bcd.Lo = Text[1] - '0';
  
  DateTimePtr->Min.Bcd.Hi = Text[3] - '0';
  DateTimePtr->Min.Bcd.Lo = Text[4] - '0';
  
  DateTimePtr->Sec.Bcd.Hi = Text[6] - '0';
  DateTimePtr->Sec.Bcd.Lo = Text[7] - '0';
}

uint16_t Reverse16(uint16_t Value)
{
  return (Value<<8)|(Value>>8);
}

uint32_t Reverse32(uint32_t Value)
{
  return ((Value&0x000000FF)<<24)|((Value&0x0000FF00)<<8)|((Value&0x00FF0000)>>8)|((Value&0xFF000000)>>24);
}

