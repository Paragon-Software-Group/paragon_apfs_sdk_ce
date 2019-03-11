// <copyright file="unixblock.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
// Revision History:
//
//     08-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#if defined UFSD_BTRFS || defined UFSD_EXTFS2 || defined UFSD_XFS || defined UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName [] = __FILE__ ",$Revision: 331632 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/uerrors.h"
#include "../h/assert.h"
#include "../h/uavl.h"

#include "unixblock.h"

namespace UFSD {


/////////////////////////////////////////////////////////////////////////////
int CUnixBlock::Init(
  IN CUnixSuperBlock* Super,
  IN UINT64           Block,
  IN bool             fCreate,
  IN unsigned int     BlockSize,
  IN unsigned char    VolIndex,
  IN bool             bCalcCrc
  )
{
  m_pSuper = Super;
  m_TreeEntry.key  = Block;
  m_VolIndex = VolIndex;
  m_bCalcCrc = bCalcCrc;

  if (BlockSize == 0)
    BlockSize = m_pSuper->GetBlockSize();         //if blocksize not defined get default value from superblock

  if (m_pBuffer == NULL || fCreate)
  {
    assert(m_pBuffer == NULL);
    Free2(m_pBuffer);
    CHECK_PTR(m_pBuffer = Zalloc2(BlockSize));
  }

  if (!fCreate)
    CHECK_CALL(m_pSuper->ReadBytes(Block << m_pSuper->m_Log2OfCluster, m_pBuffer, BlockSize, VolIndex, true));

  m_bDirty = fCreate;         //block buffer differs from block content on disk

  assert (m_ReffCounter != 0xFFFF);
  m_ReffCounter++;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
CUnixBlock::CUnixBlock(api::IBaseMemoryManager* mm)
  : UMemBased<CUnixBlock>(mm)
  , m_pBuffer(NULL)
  , m_bDirty(false)
  , m_VolIndex(0)
  , m_ReffCounter(0)
  , m_pSuper(NULL)
  , m_bCalcCrc(false)
{
  m_Entry.init();
  m_RankEntry.init();
}


/////////////////////////////////////////////////////////////////////////////
int CUnixBlock::Dtor()
{
  assert(m_ReffCounter == 0);

  int Status = Flush();

  m_Entry.remove();
  m_RankEntry.remove();
  Free2(m_pBuffer);

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixBlock::Release()
{
  assert(m_ReffCounter > 0);
  m_ReffCounter--;

  if (m_ReffCounter == 0)
    CHECK_CALL( m_pSuper->ReleaseBlock(this) );
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixBlock::Flush()
{
  if (m_bDirty)
  {
    ULOG_DEBUG1(( GetLog(), "Flush Block %" PLL "x", m_TreeEntry.key ));
    CHECK_CALL( m_pSuper->WriteBlocks(m_TreeEntry.key, m_pBuffer) );
    m_bDirty = false;
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixSmartBlockPtr::Init(
  IN CUnixSuperBlock* pSuper,
  IN UINT64           Block,
  IN bool             fCreate,
  IN unsigned int     BlockSize,
  IN unsigned char    VolIndex,
  IN bool             bCalcCrc
  )
{
  if (!pSuper)
    return ERR_BADPARAMS;

  if (IsInited())
    CHECK_CALL( m_pBlock->Release() );      //release current block because me must get new one
  CHECK_CALL(pSuper->GetBlock(Block, &m_pBlock, fCreate, BlockSize, VolIndex, bCalcCrc));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixSmartBlockPtr::Close()
{
  int Status = ERR_NOERROR;
  if (IsInited())
  {
    Status = m_pBlock->Release();
    m_pBlock = NULL;
  }
  return Status;
}


/////////////////////////////////////////////////////////////////////////////
void CUnixSmartBlockPtr::SetDirty(bool bDirty) const
{
  assert(m_pBlock != NULL);
  if (m_pBlock)
    m_pBlock->SetDirty(bDirty);
}


/////////////////////////////////////////////////////////////////////////////
bool
CUnixSmartBlockPtr::IsDirty() const
{
  assert(m_pBlock != NULL);
  if (m_pBlock)
    return m_pBlock->IsDirty();
  return false;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixSmartBlockPtr::Flush() const
{
  if (m_pBlock)
    return m_pBlock->Flush();

  return ERR_NOERROR;
}


} //namespace UFSD

#endif // UFSD_USE_UNIX_FS
