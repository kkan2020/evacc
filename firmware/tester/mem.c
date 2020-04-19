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
 
#include <stdlib.h>
#include <string.h>
#include "mem.h"

#define AlignToM4(x)                (x)

#define WMSG_ITEM_COUNT             5

#define SHORT_BUF_SIZE              8
#define SHORT_BUF_COUNT             20

#define LONG_BUF_SIZE               256
#define LONG_BUF_COUNT              5

#define MSG_ITEM_SIZE               AlignToM4(sizeof(TMsgItem))
#define MSG_ITEM_COUNT             (WMSG_ITEM_COUNT+SHORT_BUF_COUNT+LONG_BUF_COUNT)

osMemoryPoolId_t MsgItemPool, ShortBufPool, LongBufPool;

TMsgItem *NewMsgItem(int32_t MsgCode, uint16_t BufSize)
{
  TMsgItem *pItem;
  osMemoryPoolId_t pool;

  if (BufSize == 0)
    pool = 0;
  else if (BufSize<=SHORT_BUF_SIZE)
    pool = ShortBufPool;
  else if (BufSize<=LONG_BUF_SIZE)
    pool = LongBufPool;
  else
    return 0;
  
  pItem = osMemoryPoolAlloc(MsgItemPool, 0);
  if (!pItem)
    return 0;
  
  if (pool)
  {
    pItem->Buf.Ptr = osMemoryPoolAlloc(pool, 0);
    if (!pItem->Buf.Ptr)
    {
      osMemoryPoolFree(MsgItemPool, pItem);
      return 0;
    }
  }
  else
    pItem->Buf.Value = 0;

  pItem->MemPool = pool;
  pItem->Code = MsgCode;
  pItem->BufSize = BufSize;
  return pItem;
}

void RecycleMsgItem(TMsgItem *ItemPtr)
{
  if (!ItemPtr)
    return;
  if (ItemPtr->MemPool && ItemPtr->Buf.Ptr)
    osMemoryPoolFree(ItemPtr->MemPool, ItemPtr->Buf.Ptr);
  osMemoryPoolFree(MsgItemPool, ItemPtr);
}

TMsgItem *DupMsgItem(TMsgItem *SrcItem)
{
  TMsgItem *pItem;
  
  pItem = NewMsgItem(SrcItem->Code, SrcItem->BufSize);
  if (pItem)
  {
    if (SrcItem->BufSize)
      memcpy(pItem->Buf.Ptr, SrcItem->Buf.Ptr, SrcItem->BufSize);
    else
      pItem->Buf.Value = SrcItem->Buf.Value;
  }
  return pItem;
}

void InitMemotyPools(void)
{  
  MsgItemPool = osMemoryPoolNew(WMSG_ITEM_COUNT, MSG_ITEM_SIZE, NULL);
  ShortBufPool = osMemoryPoolNew(SHORT_BUF_COUNT, SHORT_BUF_SIZE, NULL);
  LongBufPool = osMemoryPoolNew(LONG_BUF_COUNT, LONG_BUF_SIZE, NULL);
}

