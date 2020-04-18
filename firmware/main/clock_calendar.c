/**
  ******************************************************************************
  * @file RTC/src/clock_calendar.c 
  * @author  MCD Application Team
  * @version  V2.0.0
  * @date  04/27/2009
  * @brief  Clock Calendar basic routines
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */ 


/* Includes ------------------------------------------------------------------*/
#include "stdint.h"
#include "stm32f10x_bkp.h"
#include "clock_calendar.h"

/* Private variables--------------------------------------------------------- */

/*Structure variable declaration for system time, system date,
alarm time, alarm date */
struct Date_s s_DateStructVar;
uint8_t SummerTimeCorrEnabled = 0;
uint16_t SummerTimeCorrect;

/**
  * @brief  COnfiguration of RTC Registers, Selection and Enabling of 
  *   RTC clock
  * @param  None
  * @retval : None
  */
void RTC_Configuration()
{
  uint16_t WaitForOscSource = 0;
//  FlagStatus ret;
  uint8_t LSI_in_use =0 ;
  
  /*Allow access to Backup Registers*/
  PWR_BackupAccessCmd(ENABLE);
  
  if(BKP_ReadBackupRegister(BKP_DR1)==CONFIGURATION_RESET)
  {
    /*Enables the clock to Backup and power interface peripherals    */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP | RCC_APB1Periph_PWR,ENABLE);

    /* Backup Domain Reset */
    BKP_DeInit();
    s_DateStructVar.Month=DEFAULT_MONTH ;
    s_DateStructVar.Day=DEFAULT_DAY;
    s_DateStructVar.Year=DEFAULT_YEAR;
    SummerTimeCorrect = OCTOBER_FLAG_SET;
    BKP_WriteBackupRegister(BKP_DR7,SummerTimeCorrect);
    BKP_WriteBackupRegister(BKP_DR2,s_DateStructVar.Month);
    BKP_WriteBackupRegister(BKP_DR3,s_DateStructVar.Day);
    BKP_WriteBackupRegister(BKP_DR4,s_DateStructVar.Year);
    BKP_WriteBackupRegister(BKP_DR1, CONFIGURATION_DONE);
    
    /*Enable 32.768 kHz external oscillator */
/*    
    RCC_LSEConfig(RCC_LSE_ON);
    do    
    {
      ret = RCC_GetFlagStatus(RCC_FLAG_LSERDY);
      WaitForOscSource++;
    } while((ret == RESET)&&(WaitForOscSource<5000));

    if(WaitForOscSource < 5000) //RTC clock source to LSE(external 32.768K)
    { 
      RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);             
      BKP_WriteBackupRegister(BKP_DR1,0xA5A5);//write 0xA5A5 to register after config
    }
    else //use LSI (40kHZ)
*/    
    {
      RCC_LSEConfig(RCC_LSE_Bypass);//RCC_LSE_OFF);        
      RCC_LSICmd(ENABLE);
      while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
      RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);  
      LSI_in_use = 1;
    }
    
    /* RTC Enabled */
    RCC_RTCCLKCmd(ENABLE);
    /*Wait for RTC registers synchronisation */
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();
    /* Setting RTC Interrupts-Seconds interrupt enabled */
    /* Enable the RTC Second */
    RTC_ITConfig(RTC_IT_SEC , ENABLE);
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();

    /* Set RTC prescaler: set RTC period to 1 sec */
    if (!LSI_in_use)
      RTC_SetPrescaler(32765); /* RTC period = RTCCLK/RTC_PR = (32.768 KHz)/(32767+1) */
        /* Prescaler is set to 32766 instead of 32768 to compensate for
        lower as well as higher frequencies*/
    else
      RTC_SetPrescaler(39999); /* RTC period = RTCCLK/RTC_PR = (40 KHz)/(39999+1) */
    /* Wait until last write operation on RTC registers has finished */
    RTC_WaitForLastTask();
 
    /* Set default system time to 09 : 24 : 00 */
    SetTime(DEFAULT_HOURS,DEFAULT_MINUTES,DEFAULT_SECONDS);
    BKP_WriteBackupRegister(BKP_DR1, CONFIGURATION_DONE);
  }
  else
  {
    /* PWR and BKP clocks selection */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    for(WaitForOscSource=0;WaitForOscSource<5000;WaitForOscSource++) __nop();
    /* Wait until last write operation on RTC registers has finished */
//    RTC_WaitForLastTask();
    /* Enable the RTC Second */
    RTC_ITConfig(RTC_IT_SEC, ENABLE);
    RTC_WaitForLastTask();
  }
    
  /* Check if how many days are elapsed in power down/Low Power Mode-
   Updates Date that many Times*/
  CheckForDaysElapsed();
  s_DateStructVar.Month = BKP_ReadBackupRegister(BKP_DR2);
  s_DateStructVar.Day = BKP_ReadBackupRegister(BKP_DR3);
  s_DateStructVar.Year = BKP_ReadBackupRegister(BKP_DR4);
  SummerTimeCorrect = BKP_ReadBackupRegister(BKP_DR7);
}



/**
  * @brief  Summer Time Correction routine
  * @param  None
  * @retval : None
  */
void SummerTimeCorrection(void)
{
  uint8_t CorrectionPending=0;
  uint8_t CheckCorrect=0;
  
  if((SummerTimeCorrect & OCTOBER_FLAG_SET)!=0)
  {
    if((s_DateStructVar.Month==10) && (s_DateStructVar.Day >24 ))
    {
      for(CheckCorrect = 25;CheckCorrect <=s_DateStructVar.Day;CheckCorrect++)
      {
        if(WeekDay(s_DateStructVar.Year,s_DateStructVar.Month,CheckCorrect )==0)
        {
          if(CheckCorrect == s_DateStructVar.Day)
          {
            /* Check if Time is greater than equal to 1:59:59 */
            if(RTC_GetCounter()>=7199)
            {
              CorrectionPending=1;
            }
          }
          else
          {
            CorrectionPending=1;
          }
         break;
       }
     }
   }
   else if((s_DateStructVar.Month > 10))
   {
     CorrectionPending=1;
   }
   else if(s_DateStructVar.Month < 3)
   {
     CorrectionPending=1;
   }
   else if(s_DateStructVar.Month == 3)
   {
     if(s_DateStructVar.Day<24)
     {
       CorrectionPending=1;
     }
     else
     {
       for(CheckCorrect=24;CheckCorrect<=s_DateStructVar.Day;CheckCorrect++)
       {
         if(WeekDay(s_DateStructVar.Year,s_DateStructVar.Month,CheckCorrect)==0)
         {
           if(CheckCorrect == s_DateStructVar.Day)
           {
             /*Check if Time is less than 1:59:59 and year is not the same in which
                March correction was done */
             if((RTC_GetCounter() < 7199) && ((SummerTimeCorrect & 0x3FFF) != \
                                   s_DateStructVar.Year))
             {
               CorrectionPending=1;
             }
             else
             {
               CorrectionPending=0;
             }
             break;
            }
            else
            {
              CorrectionPending=1;
            }
          }
        }
      }
    }
  }
  else if((SummerTimeCorrect & MARCH_FLAG_SET)!=0)
  {
    if((s_DateStructVar.Month == 3) && (s_DateStructVar.Day >24 ))
    {
      for(CheckCorrect = 25;CheckCorrect <=s_DateStructVar.Day;\
         CheckCorrect++)
      {
        if(WeekDay(s_DateStructVar.Year,s_DateStructVar.Month,\
           CheckCorrect )==0)
        {
          if(CheckCorrect == s_DateStructVar.Day)
          {
            /*Check if time is greater than equal to 1:59:59 */
            if(RTC_GetCounter()>=7199)
            {
              CorrectionPending=1;
            }
          }
          else
          {
            CorrectionPending=1;
          }
        break;
        }
      }
    }
    else if((s_DateStructVar.Month > 3) && (s_DateStructVar.Month < 10 ))
    {
      CorrectionPending=1;
    }
    else if(s_DateStructVar.Month ==10)
    {
      if(s_DateStructVar.Day<24)
      {
        CorrectionPending=1;
      }
      else
      {
        for(CheckCorrect=24;CheckCorrect<=s_DateStructVar.Day;\
          CheckCorrect++)
        {
          if(WeekDay(s_DateStructVar.Year,s_DateStructVar.Month,\
            CheckCorrect)==0)
          {
            if(CheckCorrect == s_DateStructVar.Day)
            {
              /*Check if Time is less than 1:59:59 and year is not the same in
              which March correction was done */
              if((RTC_GetCounter() < 7199) && \
                ((SummerTimeCorrect & 0x3FFF) != s_DateStructVar.Year))
              {
                CorrectionPending=1;
              }
              else
              {
                CorrectionPending=0;
              }
            break;
            }
          }
        }
      }
    }
  }

  if(CorrectionPending==1)
  {
    if((SummerTimeCorrect & OCTOBER_FLAG_SET)!=0)
    {
      /* Subtract 1 hour from the current time */
      RTC_SetCounter(RTC_GetCounter() - 3599);
      /* Reset October correction flag */
      SummerTimeCorrect &= 0xBFFF;
      /* Set March correction flag  */
      SummerTimeCorrect |= MARCH_FLAG_SET;
      SummerTimeCorrect |= s_DateStructVar.Year;
      BKP_WriteBackupRegister(BKP_DR7,SummerTimeCorrect);
    }
    else if((SummerTimeCorrect & MARCH_FLAG_SET)!=0)
    {
     /* Add 1 hour to current time */
     RTC_SetCounter(RTC_GetCounter() + 3601);
     /* Reset March correction flag */
     SummerTimeCorrect &= 0x7FFF;
     /* Set October correction flag  */
     SummerTimeCorrect |= OCTOBER_FLAG_SET;
     SummerTimeCorrect |= s_DateStructVar.Year;
     BKP_WriteBackupRegister(BKP_DR7,SummerTimeCorrect);
    }
  }
}


/**
  * @brief  Sets the RTC Current Counter Value
  * @param Hour, Minute and Seconds data
  * @retval : None
  */
void SetTime(uint8_t Hour,uint8_t Minute,uint8_t Seconds)
{
  uint32_t CounterValue;

  CounterValue=((Hour * 3600)+ (Minute * 60)+Seconds);

  RTC_WaitForLastTask();
  RTC_SetCounter(CounterValue);
  RTC_WaitForLastTask();
}

/**
  * @brief  Sets the RTC Date(DD/MM/YYYY)
  * @param DD,MM,YYYY
  * @retval : None
  */
void SetDate(uint8_t Day, uint8_t Month, uint16_t Year)
{
//  uint32_t DateTimer;
  
  /*Check if the date entered by the user is correct or not, Displays an error
    message if date is incorrect  */
  if((( Month==4 || Month==6 || Month==9 || Month==11) && Day ==31) \
    || (Month==2 && Day==31)|| (Month==2 && Day==30)|| \
      (Month==2 && Day==29 && (CheckLeap(Year)==0)))
  {
    //invalid date/time
  }
  else /* if date entered is correct then set the date*/
  {
    BKP_WriteBackupRegister(BKP_DR2,Month);
    BKP_WriteBackupRegister(BKP_DR3,Day);
    BKP_WriteBackupRegister(BKP_DR4,Year);
  }
}

/**
  * @brief Updates the Date (This function is called when 1 Day has elapsed
  * @param None
  * @retval :None
  */
void DateUpdate(void)
{
  s_DateStructVar.Month=BKP_ReadBackupRegister(BKP_DR2);
  s_DateStructVar.Year=BKP_ReadBackupRegister(BKP_DR4);
  s_DateStructVar.Day=BKP_ReadBackupRegister(BKP_DR3);
  
  if(s_DateStructVar.Month == 1 || s_DateStructVar.Month == 3 || \
    s_DateStructVar.Month == 5 || s_DateStructVar.Month == 7 ||\
     s_DateStructVar.Month == 8 || s_DateStructVar.Month == 10 \
       || s_DateStructVar.Month == 12)
  {
    if(s_DateStructVar.Day < 31)
    {
      s_DateStructVar.Day++;
    }
    /* Date structure member: s_DateStructVar.Day = 31 */
    else
    {
      if(s_DateStructVar.Month != 12)
      {
        s_DateStructVar.Month++;
        s_DateStructVar.Day = 1;
      }
     /* Date structure member: s_DateStructVar.Day = 31 & s_DateStructVar.Month =12 */
      else
      {
        s_DateStructVar.Month = 1;
        s_DateStructVar.Day = 1;
        s_DateStructVar.Year++;
      }
    }
  }
  else if(s_DateStructVar.Month == 4 || s_DateStructVar.Month == 6 \
            || s_DateStructVar.Month == 9 ||s_DateStructVar.Month == 11)
  {
    if(s_DateStructVar.Day < 30)
    {
      s_DateStructVar.Day++;
    }
    /* Date structure member: s_DateStructVar.Day = 30 */
    else
    {
      s_DateStructVar.Month++;
      s_DateStructVar.Day = 1;
    }
  }
  else if(s_DateStructVar.Month == 2)
  {
    if(s_DateStructVar.Day < 28)
    {
      s_DateStructVar.Day++;
    }
    else if(s_DateStructVar.Day == 28)
    {
      /* Leap Year Correction */
      if(CheckLeap(s_DateStructVar.Year))
      {
        s_DateStructVar.Day++;
      }
      else
      {
        s_DateStructVar.Month++;
        s_DateStructVar.Day = 1;
      }
    }
    else if(s_DateStructVar.Day == 29)
    {
      s_DateStructVar.Month++;
      s_DateStructVar.Day = 1;
    }
  }
  
  BKP_WriteBackupRegister(BKP_DR2,s_DateStructVar.Month);
  BKP_WriteBackupRegister(BKP_DR3,s_DateStructVar.Day);
  BKP_WriteBackupRegister(BKP_DR4,s_DateStructVar.Year);
}



/**
  * @brief  Checks whether the passed year is Leap or not.
  * @param  None
  * @retval : 1: leap year
  *   0: not leap year
  */
uint8_t CheckLeap(uint16_t Year)
{
  if((Year%400)==0)
  {
    return LEAP;
  }
  else if((Year%100)==0)
  {
    return NOT_LEAP;
  }
  else if((Year%4)==0)
  {
    return LEAP;
  }
  else
  {
    return NOT_LEAP;
  }
}



/**
  * @brief Determines the weekday
  * @param Year,Month and Day
  * @retval :Returns the CurrentWeekDay Number 0- Sunday 6- Saturday
  */
uint16_t WeekDay(uint16_t CurrentYear,uint8_t CurrentMonth,uint8_t CurrentDay)
{
  uint16_t Temp1,Temp2,Temp3,Temp4,CurrentWeekDay;
  
  if(CurrentMonth < 3)
  {
    CurrentMonth=CurrentMonth + 12;
    CurrentYear=CurrentYear-1;
  }
  
  Temp1=(6*(CurrentMonth + 1))/10;
  Temp2=CurrentYear/4;
  Temp3=CurrentYear/100;
  Temp4=CurrentYear/400;
  CurrentWeekDay=CurrentDay + (2 * CurrentMonth) + Temp1 \
     + CurrentYear + Temp2 - Temp3 + Temp4 +1;
  CurrentWeekDay = CurrentWeekDay % 7;
  
  return(CurrentWeekDay);
}



/**
  * @brief Get the Date(DD/MM/YY and DAY )
  * @param None
  * @retval :None
  */
void GetDate(struct Date_s *ADate)
{
  ADate->Year=(BKP_ReadBackupRegister(BKP_DR4));
  ADate->Month=(BKP_ReadBackupRegister(BKP_DR2));
  ADate->Day=(BKP_ReadBackupRegister(BKP_DR3));
}


/**
  * @brief Calcuate the Time (in hours, minutes and seconds  derived from
  *   COunter value
  * @param None
  * @retval :None
  */
void GetTime(struct Time_s *ATime)
{
  uint32_t TimeVar;
  
  TimeVar=RTC_GetCounter();
  TimeVar=TimeVar % 86400;
  ATime->HourHigh=(uint8_t)(TimeVar/3600)/10;
  ATime->HourLow=(uint8_t)(TimeVar/3600)%10;
  ATime->MinHigh=(uint8_t)((TimeVar%3600)/60)/10;
  ATime->MinLow=(uint8_t)((TimeVar%3600)/60)%10;
  ATime->SecHigh=(uint8_t)((TimeVar%3600)%60)/10;
  ATime->SecLow=(uint8_t)((TimeVar %3600)%60)%10;
}


/**
  * @brief Chaeks is counter value is more than 86399 and the number of
  *   elapsed and updates date that many times
  * @param None
  * @retval :None
  */
void CheckForDaysElapsed(void)
{
  uint8_t DaysElapsed;
 
  if((RTC_GetCounter() / SECONDS_IN_DAY) != 0)
  {
    for(DaysElapsed = 0; DaysElapsed < (RTC_GetCounter() / SECONDS_IN_DAY)\
         ;DaysElapsed++)
    {
      DateUpdate();
    }

    RTC_SetCounter(RTC_GetCounter() % SECONDS_IN_DAY);
  }
}
