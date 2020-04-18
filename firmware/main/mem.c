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

