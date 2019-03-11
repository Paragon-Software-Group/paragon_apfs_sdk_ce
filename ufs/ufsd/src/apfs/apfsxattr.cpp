// <copyright file="apfsxattr.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
// Revision History :
//
//     03-November-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331632 $";
#endif

#include <ufsd.h>

#include "../h/ucommon.h"
#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uavl.h"
#include "../h/uerrors.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixinode.h"
#include "../unixfs/unixsuperblock.h"

#include "apfs_struct.h"
#include "apfsvolsb.h"
#include "apfssuper.h"
#include "apfsinode.h"

namespace UFSD
{

namespace apfs
{


/////////////////////////////////////////////////////////////////////////////
InodeXAttr::InodeXAttr(IN api::IBaseMemoryManager* Mm)
  : UMemBased<InodeXAttr>(Mm)
  , m_Extent(0)
  , m_ValueSize(0)
  , m_AllocSize(0)
  , m_Type(0)
  , m_NameLen(0)
  , m_pNameVal(NULL)
  , m_bCloned(false)
{
  m_Entry.init();
}


/////////////////////////////////////////////////////////////////////////////
int InodeXAttr::Init(
    IN unsigned char* Name,
    IN unsigned short NameLen,
    IN unsigned short Type,
    IN char*          Value,
    IN unsigned short ValueSize
    )
{
  size_t OldBufSize = m_NameLen + m_ValueSize * (m_Extent == 0);

  if (NameLen != 0)
    m_NameLen = CPU2LE(NameLen);
  m_Type = CPU2LE(Type);

  bool bHasExtent = (m_Type == XATTR_DATA_TYPE_EXTENT || m_Type == XATTR_DATA_TYPE_EXTENT1);
  size_t NewBufSize = m_NameLen + ValueSize * (bHasExtent == false);

  if (NewBufSize > OldBufSize)
  {
    unsigned char *pOldBuf = m_pNameVal;
    CHECK_PTR(m_pNameVal = reinterpret_cast<unsigned char*>(Malloc2(NewBufSize)));

    if (OldBufSize > 0)
      Memcpy2(m_pNameVal, pOldBuf, OldBufSize);
    Free2(pOldBuf);
  }

  if (Name != NULL)
    Memcpy2(m_pNameVal, Name, NameLen);

  if (bHasExtent)
  {
    apfs_xattr_data_extent* xExtent = reinterpret_cast<apfs_xattr_data_extent*>(Value);
    if (xExtent->ds.ds_bytes > MINUS_ONE_T)
      return ERR_NOTIMPLEMENTED;
    m_ValueSize = static_cast<size_t>(CPU2LE(xExtent->ds.ds_bytes));
    m_AllocSize = CPU2LE(xExtent->ds.ds_blocks);
    m_Extent = CPU2LE(xExtent->extent_id);
  }
  else
  {
    m_ValueSize = CPU2LE(ValueSize);
    m_Extent = 0;
    Memcpy2(m_pNameVal + m_NameLen, Value, m_ValueSize);
  }

  m_bCloned = false;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
static bool CheckEa(api::IBaseMemoryManager* m_Mm, const InodeXAttr* x, const void* Name, unsigned short NameLen)
{
#ifdef STATIC_MEMORY_MANAGER
  UNREFERENCED_PARAMETER(m_Mm);
#endif
  return x->m_NameLen - 1 == NameLen && Memcmp2(x->m_pNameVal, Name, NameLen) == 0;
}


/////////////////////////////////////////////////////////////////////////////
bool CApfsInode::HasEa() const
{
  InodeXAttr* x;
  list_head* pos;

  list_for_each(pos, &m_EAList)
  {
    x = list_entry(pos, InodeXAttr, m_Entry);
    if (!CheckEa(m_Mm, x, XATTR_FORK, XATTR_FORK_LEN) && !CheckEa(m_Mm, x, XATTR_SYMLINK, XATTR_SYMLINK_LEN))
      return true;
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetXAttr(
    IN  const char*     Name,
    IN  unsigned short  NameLen,
    OUT InodeXAttr**    xAttr
    ) const
{
  InodeXAttr* x;
  list_head* pos;

  list_for_each(pos, &m_EAList)
  {
    x = list_entry(pos, InodeXAttr, m_Entry);

    if (NameLen == x->m_NameLen - 1 && Memcmp2(Name, x->m_pNameVal, x->m_NameLen - 1) == 0)
    {
      *xAttr = x;
      return ERR_NOERROR;
    }
  }

  return ERR_NOTFOUND;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ListEa(
    OUT void*     Buffer,
    IN  size_t    BytesPerBuffer,
    OUT size_t*   Bytes
    )
{
  int Status = ERR_NOERROR;
  size_t OutSize = 0;

  InodeXAttr* x;
  list_head* pos;

  list_for_each(pos, &m_EAList)
  {
    x = list_entry(pos, InodeXAttr, m_Entry);
    if (CheckEa(m_Mm, x, XATTR_FORK, XATTR_FORK_LEN) || CheckEa(m_Mm, x, XATTR_SYMLINK, XATTR_SYMLINK_LEN))
      continue;

    if (Buffer)
    {
      if (OutSize + x->m_NameLen > BytesPerBuffer)
      {
        ULOG_ERROR((GetLog(), ERR_MORE_DATA, "ListEa: input buffer size %" PZZ "x is too little", BytesPerBuffer));
        Status = ERR_MORE_DATA;
        break;
      }

      Memcpy2(Add2Ptr(Buffer, OutSize), x->m_pNameVal, x->m_NameLen);
    }

    OutSize += x->m_NameLen;
  }

  if (Bytes)
    *Bytes = OutSize;

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    OUT void*       Value,
    IN  size_t      BytesPerValue,
    OUT size_t*     Len
    )
{
  if (Name == NULL || NameLen == 0)
    return ERR_BADPARAMS;

//  ULOG_TRACE((GetLog(), "GetEa %" PLL "x: '%.*s'", m_Id, NameLen, Name));

  int Status = ERR_NOFILEEXISTS;
  list_head* pos;

  list_for_each(pos, &m_EAList)
  {
    InodeXAttr* x = list_entry(pos, InodeXAttr, m_Entry);
    //x->Dump(GetLog());

    if (static_cast<size_t>(x->m_NameLen) - 1 == NameLen && Memcmp2(x->m_pNameVal, Name, NameLen) == 0)
    {
      if (BytesPerValue)
      {
        if (x->m_ValueSize > BytesPerValue)
        {
          ULOG_TRACE((GetLog(), "GetEa: input buffer size %#" PZZ "x is too little, required %#" PZZ "x", BytesPerValue, x->m_ValueSize));
          Status = ERR_MORE_DATA;

          if (Len)
            *Len = x->m_ValueSize;
          break;
        }

        if(Value)
        {
          if (x->m_Type == XATTR_DATA_TYPE_EXTENT || x->m_Type == XATTR_DATA_TYPE_EXTENT1)
            CHECK_CALL(ReadWriteEaToExtent(x->m_Extent, Value, x->m_ValueSize, false));
          else
            Memcpy2(Value, Add2Ptr(x->m_pNameVal, x->m_NameLen), x->m_ValueSize);
        }
      }

      if (Len)
        *Len = x->m_ValueSize;

      Status = ERR_NOERROR;
      break;
    }
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetXAttrData(
    IN  InodeXAttr* xAttr,
    IN  size_t      Off,
    IN  size_t      ReqLen,
    OUT void*       pBuffer,
    OUT size_t*     OutLen
    )
{
  int Status = ERR_NOERROR;
  size_t BytesToCopy = (Off + ReqLen > xAttr->m_ValueSize) ? xAttr->m_ValueSize - Off : ReqLen;

  if (BytesToCopy > ReqLen)
  {
    if (ReqLen)
      ULOG_TRACE((GetLog(), "GetEa: input buffer size %" PZZ "x is too little, required %#" PZZ "x",
        ReqLen, BytesToCopy));
    Status = ERR_MORE_DATA;
  }
  else
  {
    if (xAttr->m_Type != XATTR_DATA_TYPE_EXTENT && xAttr->m_Type != XATTR_DATA_TYPE_EXTENT1)
    {
      if (BytesToCopy > 0)
        Memcpy2(pBuffer, Add2Ptr(xAttr->m_pNameVal, xAttr->m_NameLen + Off), BytesToCopy);
    }
    else
    {
      size_t Offset = 0;
      size_t StartBlock = CEIL_DOWN64(Off, m_pSuper->m_BlockSize);
      size_t BlockOffset = mod_u64(Off, m_pSuper->m_BlockSize);
      size_t WantedBlocks = CEIL_UP64(Off + BytesToCopy, m_pSuper->m_BlockSize) - StartBlock;

      CUnixExtent Extent;

      while (Offset < BytesToCopy)
      {
        if (StartBlock + WantedBlocks < m_StartExtent.Len)
        {
          assert(m_StartExtent.Lcn && m_StartExtent.Lcn != SPARSE_LCN);

          Extent.Lcn = m_StartExtent.Lcn + StartBlock;
          Extent.Len = MIN(WantedBlocks, m_StartExtent.Len);
          Extent.IsEncrypted = m_StartExtent.IsEncrypted;
          Extent.CryptoId = m_StartExtent.CryptoId;
        }
        else
        {
          CHECK_CALL(FindExtent(xAttr->m_Extent, StartBlock, WantedBlocks, &Extent, NULL, false));

          if (Extent.Len == 0)
          {
            ULOG_WARNING((GetLog(), "No suitable extent for Id=%" PLL "x, Vcn=%" PZZ "x", xAttr->m_Extent, StartBlock));
            Status = ERR_READFILE;
            BytesToCopy = 0;
            break;
          }
        }

        size_t LoadedBlocksSizeInBytes = Extent.Len << m_pSuper->m_Log2OfCluster;
        size_t BytesPortionToRead = (BytesToCopy - Offset > LoadedBlocksSizeInBytes - BlockOffset) ?
          LoadedBlocksSizeInBytes - BlockOffset :
          BytesToCopy - Offset;

        CHECK_CALL(m_pVol->ReadData((Extent.Lcn << m_pSuper->m_Log2OfCluster) + BlockOffset, Add2Ptr(pBuffer, Offset), BytesPortionToRead, Extent.IsEncrypted, Extent.CryptoId));

        Offset += BytesPortionToRead;
        WantedBlocks -= Extent.Len;
        StartBlock += Extent.Len;
        BlockOffset = 0;
      }
    }

    if (OutLen)
      *OutLen = BytesToCopy;
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadWriteEaToExtent(
    IN UINT64           Id,
    IN OUT const void*  Value,
    IN size_t           BytesPerValue,
    IN bool             bWrite
    )
{
  size_t Clusters = (BytesPerValue + m_pSuper->m_BlockSize - 1) >> m_pSuper->m_Log2OfCluster;
  size_t Offset = 0;
  UINT64 Vcn = 0;

  CUnixExtent Extent;

  // Simple extents reading
  while (Offset < BytesPerValue)
  {
    CHECK_CALL(FindExtent(Id, Vcn, Clusters, &Extent, NULL, bWrite));

    if (Extent.Lcn == SPARSE_LCN)
    {
      LOG_ERROR(GetLog(), ERR_READFILE, "Wrong xattr extent r=%" PLL "x", Id);
      return ERR_READFILE;
    }

    size_t BufSize = Extent.Len << m_pSuper->m_Log2OfCluster;
    size_t Bytes = (BytesPerValue - Offset > BufSize) ? BufSize : BytesPerValue - Offset;

    if (Value)
    {
      if (bWrite)
        CHECK_CALL(m_pSuper->WriteBytes(Extent.Lcn << m_pSuper->m_Log2OfCluster, Add2Ptr(Value, Offset), Bytes));
      else
        CHECK_CALL(m_pVol->ReadData(Extent.Lcn << m_pSuper->m_Log2OfCluster, Add2Ptr(Value, Offset), Bytes, Extent.IsEncrypted, Extent.CryptoId));
    }

    Offset += Bytes;
    Clusters -= Extent.Len;
    Vcn += Extent.Len;
  }
  return ERR_NOERROR;
}


} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
