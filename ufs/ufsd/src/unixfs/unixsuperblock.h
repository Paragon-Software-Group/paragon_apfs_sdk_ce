// <copyright file="unixsuperblock.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/*++

Module Name:

    unixsuperblock.h

Abstract:

    This file contains definition of CUnixSuperBlock class.

Revision History:

    07-August-2017  - created.

--*/

#ifndef __UFSD_UNIX_SUPERBL_H
#define __UFSD_UNIX_SUPERBL_H

#if !defined USE_BUILTINT_DIV64 & !defined UFSD_USE_EMUL_DIV64 & defined UFSD_DRIVER_LINUX
  #define UFSD_USE_EMUL_DIV64
#endif

#include "../h/uint64.h"

#include "unixinode.h"

//max number of blocks in cache
#ifndef UFSD_SMALL_CACHE
#define BLOCKS_CACHE_LIM        0x2000
#else
#define BLOCKS_CACHE_LIM        0x100
#endif

//Max number of blocks for flush in SmartFlushBlock
#define MAX_FLUSH_BLOCKS_PORTION  256

//default value of SPARSE_LCN
#define SPARSE_LCN MINUS_ONE_64

#define BLOCK_BELONGS_TO_CONTAINER 0xFF

namespace UFSD
{

class CUnixBlock;


struct CUnixExtent
{
  UINT64    Id;
  UINT64    Vcn;
  UINT64    Lcn;
  size_t    Len;
  UINT64    CryptoId;
  bool      IsEncrypted;
  bool      IsAllocated;
  void*     pData;          // pointer to live extent data (such as apfs_extent_data*, ...)

  CUnixExtent()
    : Id(0)
    , Vcn(0)
    , Lcn(SPARSE_LCN)
    , Len(0)
    , CryptoId(0)
    , IsEncrypted(false)
    , IsAllocated(false)
    , pData(NULL)
  {
  }

  void Init(UINT64 id, UINT64 vcn, UINT64 lcn, size_t len) { Id = id; Vcn = vcn; Lcn = lcn; Len = len; }
  void Clear() { Vcn = Len = 0; Lcn = SPARSE_LCN; }

  const char* Type() const { return Lcn? "Extent":"Hole"; }
  void Trace(const char* Title, api::IBaseLog* Log) const
  {
#ifdef UFSD_TRACE
    if (Lcn != SPARSE_LCN)
    {
      if (Lcn != 0)
        ULOG_DEBUG1((Log, "%s%sExtent: [%" PLL "x, %" PLL "x) -> [%" PLL "x, %" PLL "x)", Title, *Title?" ":"", Vcn, End(), Lcn, EndLcn()));
      else
        ULOG_DEBUG1((Log, "%s%sHole: [%" PLL "x, %" PLL "x)", Title, *Title ? " " : "", Vcn, End()));
    }
#else
  UNREFERENCED_PARAMETER(Title);
  UNREFERENCED_PARAMETER(Log);
#endif
  }

  bool CheckOffset(UINT64 Offset, unsigned int BlockSize) const
  {
    return (Vcn * BlockSize <= Offset && (Vcn + Len) * BlockSize > Offset);
  }

  bool CheckVcn(UINT64 v) const
  {
    return (Vcn <= v && Vcn + Len > v);
  }

  bool CheckLcn(UINT64 l) const
  {
    return (Lcn <= l && Lcn + Len > l);
  }

  bool operator==(const CUnixExtent& e) const
  {
    return e.Id == Id && e.Vcn == Vcn && e.Lcn == Lcn && e.Len == Len;
  }

  bool operator!=(const CUnixExtent& e) const
  {
    return e.Id != Id || e.Vcn != Vcn || e.Lcn != Lcn || e.Len != Len;
  }

  void Truncate(UINT64 v)
  {
    if (Vcn >= v)
      Clear();
    else if (CheckVcn(v))
    {
      assert(v - Vcn <= MINUS_ONE_T);
      Len = static_cast<size_t>(v - Vcn);
    }
  }

  UINT64 End() const { return Vcn + Len; }
  UINT64 EndLcn() const { return Lcn + Len; }
};


struct CUnixSuperBlock : UMemBased<CUnixSuperBlock>
{
  api::IBaseStringManager*      m_Strings;
  api::IDeviceRWBlock*          m_Rw;
  api::ITime*                   m_Time;
  api::IBaseLog*                m_Log;

  avl_tree                      m_BlockCache;      //rbtree stored inodes sorted by id
  avl_tree                      m_InodeCache;      //rbtree stored blocks sorted by block number

  bool                          m_bReadOnly;
  unsigned int                  m_BytesPerSector;
  unsigned int                  m_SectorsPerBlock;

  struct list_head              m_BlocksRankList;   //list of blocks sorted by access time (only closed blocks)
  unsigned int                  m_BlocksCacheLimit; //max size of m_BlocksRankList
  unsigned int                  m_BlocksCount;      //current size of m_BlocksRankList

  unsigned int                  m_BlockSize;
  unsigned int                  m_Log2OfCluster;
  unsigned int                  m_InodeSize;

  bool                          m_bDirty;
  bool                          m_bInited;

  CUnixSuperBlock(
    IN api::IBaseMemoryManager* Mm,
    IN api::IBaseLog*           Log
    );

  virtual ~CUnixSuperBlock();

  //=============================================================================
  //                 CUnixSuperBlock virtual functions
  //=============================================================================

  // Returns the superblock disk structure
  virtual void* SuperBlock() const = 0;

  virtual int Flush();

  virtual UINT64 GetFreeBlocksCount() const = 0;
  virtual UINT64 GetBlocksCount() const = 0;

  // returns max inode number
  virtual UINT64 GetInodesCount() const = 0;
  virtual UINT64 GetFreeInodesCount() const = 0;
  virtual UINT64 GetUsedInodesCount() const = 0;

  //Work with inode cache
  virtual int GetInode(
    IN  UINT64        Id,
    OUT CUnixInode**  ppInode,
    IN  bool          fCreate = false
  ) = 0;

  virtual int UnlinkInode(CUnixInode *inode) = 0;

  // Copy bitmap to supplied buffer
  virtual int GetBitmap(
    IN unsigned char* Buf,
    IN size_t         BytesOffset,
    IN size_t         Bytes
  ) = 0;

  //read/write block operations
  virtual int ReadBlocks(UINT64 Block, void* pBuff, size_t Count = 1) const;
  virtual int WriteBlocks(UINT64 Block, const void* pBuff, size_t Count = 1) const;

  virtual int AllocDiskSpace(
    IN  size_t    Len,
    OUT UINT64*   Lcn,
    OUT size_t*   OutLen = NULL,
    IN  UINT64    HintLcn = 0
  ) = 0;

  virtual int ReleaseDiskSpace(
    IN  UINT64    Lcn,
    IN  size_t    Len,
    IN  bool      InCache          //This flag must be set if blocks from this range may be in cache
  ) = 0;

  virtual int GetAvailableExpandBlocks(
    IN  bool fast,
    OUT UINT64 *MaxBlocks
  ) const
  {
    UNREFERENCED_PARAMETER(fast);
    UNREFERENCED_PARAMETER(MaxBlocks);
    return ERR_NOTIMPLEMENTED;
  }

  //=============================================================================
  //                 The end of CUnixSuperBlock virtual functions
  //=============================================================================

  api::IBaseLog* GetLog() const { return GetVcbLog( this ); }

  //Is read only (for file or dir if Id != 0, or for whole fs if Id == 0)
  virtual bool IsReadOnly(UINT64 Id = 0) const
  {
    UNREFERENCED_PARAMETER(Id);
    return m_bReadOnly DRIVER_ONLY(|| !!m_Rw->IsReadOnly());
  }

  bool IsInited() const { return m_bInited; }

  void SetDirty(bool bDirty = true) { m_bDirty = bDirty; }
  unsigned int GetBlockSize() const { return m_BlockSize; }
  unsigned int GetInodeSize() const { return m_InodeSize; }
  unsigned int GetSectorSize() const { return m_BytesPerSector; }
  unsigned int GetSectorsPerBlock() const { return m_SectorsPerBlock; }

  virtual int ReadBytes(UINT64 Offset, void* pBuff, size_t Bytes) const
  {
    return m_Rw->ReadBytes(Offset, pBuff, Bytes);
  }

  virtual int ReadBytes(UINT64 Offset, void* pBuff, size_t Bytes, unsigned char VolumeIndex, bool bMetaData) const
  {
    UNREFERENCED_PARAMETER(VolumeIndex);
    UNREFERENCED_PARAMETER(bMetaData);
    return ReadBytes(Offset, pBuff, Bytes);
  }

  virtual int WriteBytes(UINT64 Offset, const void* pBuff, size_t Bytes) const
  {
    return m_Rw->WriteBytes(Offset, pBuff, Bytes);
  }

  //Create and initialize cache block
  virtual int CreateCacheBlock(
    IN  UINT64        Block,
    OUT CUnixBlock**  ppNewBlock,
    IN  bool          fCreate,
    IN  unsigned int  CacheBlockSize,
    IN  unsigned char VolIndex,
    IN  bool          bCalcCrc
  );

  UINT64 GetCurrentTime() const
  {
#ifdef UFSD_DRIVER_LINUX
    return TimePosixToUnix(m_Time->Time());
#else
    return TimeUFSDToUnix(m_Time->Time());
#endif
  }

  UINT64 GetCurrentTimeNs() const
  {
#ifdef UFSD_DRIVER_LINUX
    return TimePosixToUnixNs(m_Time->Time());
#else
    return TimeUFSDToUnixNs(m_Time->Time());
#endif
  }

  static UINT64 TimeUnixToUFSD(IN UINT64 unix_time)
  {
    return (SecondsToStartOf1970 + unix_time /*+ m_Time->GetBias() * 60*/) * api::ITime::TicksPerSecond;
  }

  static UINT64 TimeUnixToUFSDNs(IN UINT64 unix_time)
  {
    static const unsigned int Ticks = div_u64(1000000000, api::ITime::TicksPerSecond);
    static const UINT64 NsSinceEpoch = SecondsToStartOf1970 * 1000000000;

    return (NsSinceEpoch + unix_time /*+ m_Time->GetBias() * 60*/) / Ticks;
  }

#ifdef UFSD_DRIVER_LINUX
  static UINT64 TimeUnixToPosix(IN UINT64 unix_time)
  {
    api::ITime::utime utime;
    utime.tv_sec  = unix_time;
    utime.tv_nsec = 0;
    return utime.time64;
  }
  static UINT64 TimeUnixToPosixNs(IN UINT64 unix_time)
  {
    api::ITime::utime utime;
    utime.tv_sec  = div_u64(unix_time, 1000000000);
    utime.tv_nsec = unix_time - utime.tv_sec * 1000000000;
    return utime.time64;
  }
  static UINT64 TimePosixToUnix(IN UINT64 posix_time)
  {
    api::ITime::utime utime;
    utime.time64 = posix_time;
    return utime.tv_sec;
  }
  static UINT64 TimePosixToUnixNs(IN UINT64 posix_time)
  {
    api::ITime::utime utime;
    utime.time64 = posix_time;

    return (UINT64)utime.tv_sec * 1000000000 + utime.tv_nsec;
  }
#endif

  static INT64 TimeUFSDToUnix(IN UINT64 ufsd_time)
  {
    INT64 seconds = div_u64(ufsd_time, api::ITime::TicksPerSecond) - SecondsToStartOf1970;
    if (seconds <= 0)
      return 0;
    return seconds;
  }

  static INT64 TimeUFSDToUnixNs(IN UINT64 ufsd_time)
  {
    static const unsigned int Ticks = div_u64(1000000000, api::ITime::TicksPerSecond);
    static const UINT64 NsSinceEpoch = SecondsToStartOf1970 * 1000000000;

    INT64 ns = ufsd_time * Ticks - NsSinceEpoch;
    if (ns <= 0)
      return 0;
    return ns;
  }

  template<typename T>
  int GetInodeT(
    IN  UINT64        Id,
    OUT CUnixInode**  ppInode,
    IN  bool          fCreate
  )
  {
    avl_link* n = avl_lookup(&m_InodeCache, Id);
    CUnixInode* pInode = n ? avl_entry(n, CUnixInode, m_TreeEntry) : NULL;

    if (pInode && pInode->Id() == Id)
    {
      //inode found
      if (fCreate)
        ULOG_WARNING((m_Log, "fCreate flag ignored because inode already existing in cache"));
      *ppInode = pInode;
      pInode->IncRefCount();
      return ERR_NOERROR;
    }

    T* pNewInode = new(m_Mm) T(m_Mm);
    CHECK_PTR(pNewInode);

    int Status = pNewInode->Init(this, Id, fCreate);
    if (Status != ERR_NOERROR)
    {
      delete pNewInode;
      return Status;
    }

    *ppInode = pNewInode;

    avl_insert(&m_InodeCache, &pNewInode->m_TreeEntry);

    return ERR_NOERROR;
  }

  //get block from cache
  int GetBlock(
    IN  UINT64        Block,
    OUT CUnixBlock**  ppBlock,
    IN  bool          fCreate,
    IN  unsigned int  CacheBlockSize,
    IN  unsigned char VolIndex,
    bool              bCalcCrc
  );

  //close block, add it to m_BlocksRankList
  int ReleaseBlock(
    IN CUnixBlock *pClosedBlock
  );

  //flush pFlushBlock block and neighbor blocks in cache
  int SmartFlushBlock(
    IN CUnixBlock *pFlushBlock
  ) const;

  //delete block from cache (m_BlocksRankList and m_BlocksList)
  int DeleteBlockFromCache(
    IN UINT64 Block
  );
};


}

#endif
