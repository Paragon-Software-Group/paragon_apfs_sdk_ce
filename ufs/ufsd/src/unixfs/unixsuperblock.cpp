// <copyright file="unixsuperblock.cpp" company="Paragon Software Group">
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
//     30-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#if defined UFSD_BTRFS | defined UFSD_EXTFS2 | defined UFSD_XFS | defined UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331714 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"

#include "unixsuperblock.h"
#include "unixblock.h"


namespace UFSD
{

CUnixSuperBlock::CUnixSuperBlock(
  IN api::IBaseMemoryManager* Mm,
  IN api::IBaseLog*           Log
  )
  : UMemBased<CUnixSuperBlock>(Mm)
  , m_Strings(NULL)
  , m_Rw(NULL)
  , m_Time(NULL)
  , m_Log(Log)
  , m_bReadOnly(false)
  , m_BytesPerSector(0)
  , m_SectorsPerBlock(0)
  , m_BlocksCacheLimit(BLOCKS_CACHE_LIM > 1 ? BLOCKS_CACHE_LIM : 2)
  , m_BlocksCount(0)
  , m_BlockSize(0)
  , m_Log2OfCluster(0)
  , m_InodeSize(0)
  , m_bDirty(false)
  , m_bInited(false)
{
  m_BlocksRankList.init();
}


//////////////////////////////////////////////////////////////////////////
CUnixSuperBlock::~CUnixSuperBlock()
{
  assert(m_bDirty == false);

  int Status = Flush();
  assert(UFSD_SUCCESS(Status));
#ifdef UFSD_TRACE
  ULOG_TRACE((GetLog(), "~CUnixSuperBlock -> %x", Status));
#else
  UNREFERENCED_PARAMETER(Status);
#endif

  avl_link* e;

  assert(m_InodeCache.is_empty());
  while (!m_InodeCache.is_empty())
  {
    e = m_InodeCache.first();
    CUnixInode* pInode = avl_entry(e, CUnixInode, m_TreeEntry);
    m_InodeCache.remove(e);
    pInode->Destroy();
  }

  while (!m_BlockCache.is_empty())
  {
    e = m_BlockCache.first();
    CUnixBlock* pBlock = avl_entry(e, CUnixBlock, m_TreeEntry);
    m_BlockCache.remove(e);
    pBlock->Destroy();
  }

  assert(m_BlocksRankList.is_empty());
}


//////////////////////////////////////////////////////////////////////////
int CUnixSuperBlock::ReadBlocks(UINT64 Block, void* pBuff, size_t Count) const
{
  return m_Rw->ReadBytes(Block << m_Log2OfCluster, pBuff, Count << m_Log2OfCluster);
}


/////////////////////////////////////////////////////////////////////////////
int CUnixSuperBlock::CreateCacheBlock(
  IN  UINT64        Block,
  OUT CUnixBlock**  ppNewBlock,
  IN  bool          fCreate,
  IN  unsigned int  CacheBlockSize,
  IN  unsigned char VolIndex,
  IN  bool          bCalcCrc
  )
{
  CUnixBlock* pBlock = new(m_Mm) CUnixBlock(m_Mm);
  CHECK_PTR(pBlock);
  int Status = pBlock->Init(this, Block, fCreate, CacheBlockSize, VolIndex, bCalcCrc);

  if (UFSD_SUCCESS(Status))
    *ppNewBlock = pBlock;
  else
    delete pBlock;
  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixSuperBlock::GetBlock(
  IN  UINT64        Block,
  OUT CUnixBlock**  ppBlock,
  IN  bool          fCreate,
  IN  unsigned int  CacheBlockSize,
  IN  unsigned char VolIndex,
  IN  bool          bCalcCrc
  )
{
  assert(Block != 0);

  avl_link* n = avl_lookup(&m_BlockCache, Block);
  CUnixBlock* pBlock = n ? avl_entry(n, CUnixBlock, m_TreeEntry) : NULL;

  if (pBlock && pBlock->Id() == Block)
  {
    //if fCreate flag set zero block buffer
    if (fCreate)
    {
      Memzero2(pBlock->GetBuffer(), CacheBlockSize);
      pBlock->SetDirty();
    }

    //Block found. Remove blocks from the the top of RankList and exit function
    if (pBlock->GetReffCounter() == 0)
    {
      //pBlock is in m_BlocksRankList only if it is closed (m_ReffCounter = 0)
      pBlock->m_RankEntry.remove();
      m_BlocksCount--;
    }
    pBlock->IncReffCount();
    *ppBlock = pBlock;
    return ERR_NOERROR;
  }

  CUnixBlock* pNewBlock = NULL;
  CHECK_CALL(CreateCacheBlock(Block, &pNewBlock, fCreate, CacheBlockSize, VolIndex, bCalcCrc));
  *ppBlock = pNewBlock;

  avl_insert(&m_BlockCache, &pNewBlock->m_TreeEntry);

  return ERR_NOERROR;
}


//////////////////////////////////////////////////////////////////////////
int CUnixSuperBlock::ReleaseBlock(IN CUnixBlock *pClosedBlock)
{
  assert(pClosedBlock->GetReffCounter() == 0);
  assert(m_BlocksCacheLimit >= 2);

  pClosedBlock->m_RankEntry.insert_after(&m_BlocksRankList);
  m_BlocksCount++;

  int Status = ERR_NOERROR;
  if (m_BlocksCount >= m_BlocksCacheLimit)
  {
    CUnixBlock* pDelBlock = list_entry(m_BlocksRankList.prev, CUnixBlock, m_RankEntry);
    assert(pDelBlock != pClosedBlock);          //not normal case because size of blocks cache >= 2
    m_BlocksCount--;
    Status = SmartFlushBlock(pDelBlock);
    m_BlockCache.remove(&pDelBlock->m_TreeEntry);
    pDelBlock->Destroy();
  }

  return Status;
}


//////////////////////////////////////////////////////////////////////////
int CUnixSuperBlock::DeleteBlockFromCache(IN UINT64 Block)
{
  assert(Block != 0);

  avl_link* n = avl_lookup(&m_BlockCache, Block);
  CUnixBlock* pBlock = n ? avl_entry(n, CUnixBlock, m_TreeEntry) : NULL;

  if (pBlock && pBlock->Id() == Block)
  {
    //Block found. Remove it from cache
    pBlock->SetDirty(false);            //we don't need to flush this block, because it will be cleared

    if (pBlock->GetReffCounter() > 0)
    {
      assert(0);
      return ERR_NOERROR;// ERR_BADPARAMS;
    }
    m_BlocksCount--;

    m_BlockCache.remove(&pBlock->m_TreeEntry);
    pBlock->Destroy();
  }

  return ERR_NOERROR;
}

#if (defined UFSD_APFS && defined UFSD_APFS_RO) && !defined UFSD_BTRFS && !defined UFSD_EXTFS2 && !defined UFSD_XFS
int CUnixSuperBlock::Flush() { return ERR_NOERROR; }
int CUnixSuperBlock::WriteBlocks(UINT64, const void*, size_t) const { return ERR_WPROTECT; }
int CUnixSuperBlock::SmartFlushBlock(CUnixBlock*) const { return ERR_WPROTECT; }
#endif

}

#endif
