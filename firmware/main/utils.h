#ifndef __UTILS_H__
  #define __UTILS_H__
#include <stdint.h>  
  
#define ENCRYPT_C1			52845
#define ENCRYPT_C2			11719
#define ENCRYPT_KEY 		0xC5A3

typedef __packed union
{
  __packed struct
  {
    uint8_t Lo: 4;
    uint8_t Hi: 4;
  } Bcd;
  uint8_t Byte;
} TBinDigit;

typedef __packed struct
{
  TBinDigit Year, Month, Day, Hour, Min, Sec;
} TDateTime;


typedef __packed struct
{
  TBinDigit Hour, Min;
} TTimeSch;

__inline void DelayLoop(uint32_t Count);
uint8_t ByteToHexStr(char *Dest, uint8_t Value);
uint8_t HexStrToByte(char *Hex);
uint16_t IntToStr(char *Dest, int32_t Value); //return length of result string
int32_t StrToInt(char* Src);
uint32_t FloatToStr(char *Buf, float Value, uint32_t DecimalPt);
//void Decrypt(uint8_t *Buf, uint8_t Size);
//void Encrypt(uint8_t *Buf, uint8_t Size);
uint32_t BcdToInt(TBinDigit BinDigit);
TBinDigit IntToBcd(uint32_t Value);
uint32_t DateToStr(char *Buf, TDateTime *DateTimePtr);
uint32_t DateToFullYearStr(char *Buf, TDateTime *DateTimePtr);
uint32_t TimeToStr(char *Buf, TDateTime *DateTimePtr);
uint32_t TimeSchToStr(char *Buf, TTimeSch *SchPtr);
void StrToTimeSch(TTimeSch *SchPtr, char *Text);
uint32_t DateTimeToStr(char *Buf, TDateTime *DateTimePtr);
void StrToDate(TDateTime *DateTimePtr, char *Text);
void StrToTime(TDateTime *DateTimePtr, char *Text);
uint16_t Reverse16(uint16_t Value);
uint32_t Reverse32(uint32_t Value);
void ReadRTC(TDateTime *DateTimePtr);
void WriteRTC(TDateTime *DateTimePtr);

#endif
