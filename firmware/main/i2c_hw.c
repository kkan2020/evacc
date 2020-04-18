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
#include "stm32f10x.h"
#include "cmsis_os2.h"
#include "hal.h"
#include "i2c_hw.h"

osMutexId_t mutexEEP;

void I2CTimerStart(TI2C *I2CPort, uint16_t msec);
void I2CTimerStop(TI2C *I2CPort);
uint8_t I2C_WaitForTxEnd(TI2C *I2CPort);
uint8_t I2C_WaitForRxEnd(TI2C *I2CPort);
void I2C_Write(TI2C *I2CPort, uint8_t* Buf, uint16_t Size);
void I2C_Read(TI2C *I2CPort, uint8_t *Buf, uint16_t Size);


void I2CDlyCycles(uint32_t Cycles)
{
  while (Cycles--) 
    __nop();
}

void I2CTimerStart(TI2C *I2CPort, uint16_t msec)
{
  TIM_Cmd(I2CPort->Timer, DISABLE);
  I2CPort->Timeout = 0;
  TIM_SetCounter(I2CPort->Timer, 0);
  TIM_SetAutoreload(I2CPort->Timer, 2*msec-1); //timer clock is running at 2kHz
  TIM_ClearITPendingBit(I2CPort->Timer, TIM_IT_Update);
  TIM_Cmd(I2CPort->Timer, ENABLE);
} 

void I2CTimerStop(TI2C *I2CPort)
{
  TIM_Cmd(I2CPort->Timer, DISABLE);
}

uint8_t I2C_WaitForTxEnd(TI2C *I2CPort)
{
  while ((I2CPort->Reg->SR2 & 0x02)&& !I2CPort->Timeout); 

  if (I2CPort->Timeout)
  {
    return 0;
  }
  I2CTimerStop(I2CPort);
  return 1;
}

uint8_t I2C_WaitForRxEnd(TI2C *I2CPort)
{
  while (!I2CPort->DataReady && !I2CPort->Timeout); 

  if (I2CPort->Timeout)
  {
    return 0;
  }
  I2CTimerStop(I2CPort);
  return 1;
}

#pragma O0
void I2C_Write(TI2C *I2CPort, uint8_t* Buf, uint16_t Size)
{
//  I2CWaitFor(); //will get stucked
  if (!Size)
    return;
  I2CTimerStart(I2CPort, 1000); 
  while (IsI2CBusy(I2CPort) && !I2CPort->Timeout);
  I2CPort->TxBuf = Buf;
  I2CPort->Transmit = 1;
  I2CPort->TxCount = Size;
  I2CPort->Timeout = 0;
  /* generate start condition */
  I2CTimerStart(I2CPort, 4*8*Size); 
  I2C_ITConfig(I2CPort->Reg, I2C_IT_EVT|I2C_IT_ERR, ENABLE); 
  I2C_GenerateSTART(I2CPort->Reg, ENABLE);
}

#pragma O0
void I2C_Read(TI2C *I2CPort, uint8_t *Buf, uint16_t Size)
{
  if (!Size)
    return;
  /* Set the I2C direction to reception */
  I2CPort->Transmit = 0;  
  I2CPort->RxBuf = Buf;
  I2CPort->RxCount = Size;
  I2CPort->Timeout=0;
  I2CPort->DataReady = 0;
  /* generate start condition */
  I2CTimerStart(I2CPort, 4*8*Size);  
  I2C_ITConfig(I2CPort->Reg, I2C_IT_EVT|I2C_IT_BUF|I2C_IT_ERR, ENABLE);
  I2C_GenerateSTART(I2CPort->Reg, ENABLE);
}

int EEPWrite(TI2C *I2CPort, uint16_t Addr, void *Buf, uint16_t Size)
{
  TEepromBuf eepBuf;
  uint8_t wrSize, *pChar;
  int ret=0;

  if (Size==0)
    return 1;
  if (osMutexAcquire(mutexEEP, osWaitForever) != osOK)
    return 0;
#if !(defined(_24C32_)||defined(_24C64_))
//  I2CPort->DataAddrMSB = Addr>>8;  
#endif  
  EEPWriteProtect(OFF);
  pChar = Buf; 
  while (Size)
  {
    wrSize = EEP_PAGE_SIZE - (Addr & (EEP_PAGE_SIZE-1));
    if (wrSize>Size)
      wrSize = Size;
    memcpy(eepBuf.Data, pChar, wrSize);
    I2CPort->DummyWrite = 0;
#if defined(_24C32_)||defined(_24C64_)
    eepBuf.Addr[0] = Addr >> 8;
    eepBuf.Addr[1] = Addr;
#else
    eepBuf.Addr = Addr;    
#endif    
    I2C_Write(I2CPort, (uint8_t*)&eepBuf, sizeof(eepBuf.Addr)+wrSize); //addr word + data size
    if ( !I2C_WaitForTxEnd(I2CPort) )
      goto _exit;
	  I2CDlyCycles(200000);
    pChar += wrSize;
    Addr += wrSize;
    Size -= wrSize;
  }
  ret = 1;
_exit:  
  EEPWriteProtect(ON);
  osMutexRelease(mutexEEP);
  return ret;
}

int EEPRead(TI2C *I2CPort, uint16_t Addr, void *Buf, uint16_t Size)
{
  int ret=0;
#if defined(_24C32_)||defined(_24C64_)
  uint8_t a[2];
#else
  uint8_t a[1];
#endif  
  
  if (Size==0)
    return 1;
  
#if defined(_24C32_)||defined(_24C64_)
  a[0] = Addr >> 8;
  a[1] = Addr;
#else  
  a[0] = Addr;
#endif

  if (osMutexAcquire(mutexEEP, osWaitForever) != osOK)
    return 0;
#if !(defined(_24C32_)||defined(_24C64_))  
//  I2CPort->DataAddrMSB = Addr>>8;  
#endif  
  //start with dummy write to set data address
  I2CPort->DummyWrite = 1;
  I2C_Write(I2CPort, a, sizeof(a)); //dummy write to set eeprom address
  while (I2CPort->DummyWrite && !I2CPort->Timeout);
  I2CTimerStop(I2CPort);
  if (I2CPort->Timeout)
    goto _exit;
  
  I2CDlyCycles(100);
  I2CPort->Transmit = 0;
  I2CPort->RxBuf = Buf;
  I2CPort->RxCount = Size;
  I2CPort->DataReady = 0;  
  I2CPort->Timeout=0;
  I2CTimerStart(I2CPort, 4*8*Size); 
  I2C_ITConfig(I2CPort->Reg, I2C_IT_EVT|I2C_IT_ERR, ENABLE);         
  I2C_AcknowledgeConfig(I2CPort->Reg, ENABLE);  
  I2C_GenerateSTART(I2CPort->Reg, ENABLE); //gen restart condition
  while (!I2CPort->Timeout && !I2CPort->DataReady);
  if (I2CPort->Timeout)
    goto _exit;
  else  
  {
    I2CTimerStop(I2CPort);
    I2CDlyCycles(100);
    ret = 1;
  }
_exit:
  osMutexRelease(mutexEEP);
  return ret;    
}

void InitEEP(void)
{
  mutexEEP = osMutexNew(NULL);
  while (mutexEEP==NULL);
}

void EepromHandler(TI2C *I2CPort)
{
  __IO uint32_t SR1Register;
  __IO uint32_t SR2Register;
                   
  /* Read the I2C SR1 and SR2 status registers */
  SR1Register = I2CPort->Reg->SR1;
  SR2Register = I2CPort->Reg->SR2;

  /* If SB = 1, I2C master sent a START on the bus: EV5) */
  if ((SR1Register &0x0001) == 0x0001)
  {
    /* Send the slave address for transmssion or for reception (according to the configured value
        in the write master write routine */
    if (I2CPort->Transmit)
      I2C_Send7bitAddress(I2CPort->Reg, I2CPort->SlaveAddr7|I2CPort->DataAddrMSB, I2C_Direction_Transmitter);
    else
      I2C_Send7bitAddress(I2CPort->Reg, I2CPort->SlaveAddr7|I2CPort->DataAddrMSB, I2C_Direction_Receiver);
    SR1Register = 0;
    SR2Register = 0;
  }
  
  /* If I2C is Master (MSL flag = 1) */
  if ((SR2Register &0x0001) == 0x0001)
  {
    /* If ADDR = 1, EV6 */
    if ((SR1Register &0x0002) == 0x0002)  //address acknowledged
    {
      if (I2CPort->Transmit)
      {
        /* Write the first data in case the Master is Transmitter */
        /* Initialize the Transmit counter */
        I2CPort->TxIdx = 0;

        /* If only one byte data to be sent, disable the I2C BUF IT
        in order to not have a TxE  interrupt */
        if (I2CPort->TxCount == 1)
          I2C_ITConfig(I2CPort->Reg, I2C_IT_BUF, DISABLE);
        else
          I2C_ITConfig(I2CPort->Reg, I2C_IT_BUF, ENABLE);  
        
        /* Write the first data in the data register */
        I2C_SendData(I2CPort->Reg, I2CPort->TxBuf[I2CPort->TxIdx++]);
        /* Decrement the number of bytes to be written */
        I2CPort->TxCount--;
      }    
      else /* Master Receiver */
      {
        /* Initialize Receive counter */
        I2CPort->RxIdx = 0;
        /* At this stage, ADDR is cleared because both SR1 and SR2 were read.*/
        /* EV6_1: used for single byte reception. The ACK disable and the STOP
        Programming should be done just after ADDR is cleared. */
        if (I2CPort->RxCount == 1)
        {
          /* Clear ACK */
          I2C_AcknowledgeConfig(I2CPort->Reg, DISABLE);
          /* Program the STOP */
          I2C_GenerateSTOP(I2CPort->Reg, ENABLE);
        }
      }
      SR1Register = 0;
      SR2Register = 0;
    }
    /* Master transmits the remaing data: from data2 until the last one.  */
    /* If TXE is set */
    if ((SR1Register &0x0084) == 0x0080) //Tx buffer empty but may still transferring
    {
      /* If there is still data to write */
      if (I2CPort->TxCount!=0)
      {
        /* If remain one byte of data to be sent, disable the I2C BUF IT
        in order to not have a TxE  interrupt */
        if (I2CPort->TxCount == 1)
          I2C_ITConfig(I2CPort->Reg, I2C_IT_BUF, DISABLE);  

        /* Write the data in DR register */
        I2C_SendData(I2CPort->Reg, I2CPort->TxBuf[I2CPort->TxIdx++]);
        /* Decrment the number of data to be written */
        I2CPort->TxCount--;
      }
      SR1Register = 0;
      SR2Register = 0;
    }

    /* If BTF and TXE are set (EV8_2), program the STOP */
    if ((SR1Register &0x0084) == 0x0084) //Tx buffer empty AND transfer completed -> last byte of data has been sent
    {
      /* Program the STOP */
      if (!I2CPort->DummyWrite)        
      {
        I2C_GenerateSTOP(I2CPort->Reg, ENABLE);
        /* Disable EVT IT In order to not have again a BTF IT */
        I2C_ITConfig(I2CPort->Reg, I2C_IT_EVT, DISABLE);
      }
      else
        I2CPort->DummyWrite = 0;
    
      SR1Register = 0;
      SR2Register = 0;
    }

    /* If RXNE is set */
    if ((SR1Register &0x0040) == 0x0040)
    {
      /* Read the data register */
      I2CPort->RxBuf[I2CPort->RxIdx++] = I2C_ReceiveData(I2CPort->Reg);
      /* Decrement the number of bytes to be read */
      I2CPort->RxCount--;

      /* If it remains only one byte to read, disable ACK and program the STOP (EV7_1) */
      if (I2CPort->RxCount == 1)
      {
        /* Clear ACK */
        I2C_AcknowledgeConfig(I2CPort->Reg, DISABLE);
          /* Program the STOP */
        I2C_GenerateSTOP(I2CPort->Reg, ENABLE);
      }

      if(I2CPort->RxCount==0)
      {
        I2CPort->DataReady = 1;
        I2C_ITConfig(I2CPort->Reg, I2C_IT_EVT, DISABLE);
      }

      SR1Register = 0;
      SR2Register = 0;
    }
  }  
}

void EepromTimeoutHandler(TI2C *I2CPort)
{
  TIM_Cmd(I2CPort->Timer, DISABLE);
  I2CPort->Timeout = 1;
  I2C_ITConfig(I2CPort->Reg, I2C_IT_EVT|I2C_IT_BUF|I2C_IT_ERR, DISABLE);
  I2C_Configuration();
  EEPWriteProtect(ON);
}

