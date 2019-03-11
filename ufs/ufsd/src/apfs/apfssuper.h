// <copyright file="apfssuper.h" company="Paragon Software Group">
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
//     04-October-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __UFSD_APFS_SUPER_H
#define __UFSD_APFS_SUPER_H

#include "../unixfs/unixsuperblock.h"
#include "apfsvolsb.h"

#ifndef UFSD_APFS_RO
#include "rw/apfsbitmap.h"
#endif


namespace UFSD
{

namespace apfs
{

//For generating unique inodes for volumes in apfs container
#define APFS_BITS_PER_INDODE_ID     56
#define APFS_GET_TREE_ID(id)                 static_cast<unsigned char>(id >> APFS_BITS_PER_INDODE_ID)
#define APFS_GET_INODE_ID(id)                (id & ((PU64(1) << APFS_BITS_PER_INDODE_ID) - 1))
#define APFS_MAKE_INODE_ID(TreeId, InodeId)  (((UINT64)(TreeId) << APFS_BITS_PER_INDODE_ID) | (InodeId))

//Displaying not 0th volumes
#define APFS_VOLUMES_DIR_NAME       "Ufsd_Volumes"
#define APFS_VOLUMES_DIR_NAME_LEN   sizeof(APFS_VOLUMES_DIR_NAME) - 1
#define APFS_VOLUMES_DIR_ID         PU64(0x00FFFFFFFFFFFFFF)

#define APFS_SIEVE_FILE_NAME       "$Sieve"

#define APFS_MAX_SIZE_IN_BLOCKS    0xFFFFFFFF

class CApfsFileSystem;

class CApfsSuperBlock : public CUnixSuperBlock
{
  friend class CApfsFileSystem;
#if defined UFSD_APFS_FORMAT
  friend struct CApfsFormat;
#endif
#ifdef UFSD_APFS_CHECK
  friend struct CCheckApfs;
#endif

  apfs_sb*               m_pMSB;                     //Main superblock
  apfs_sb*               m_pCSB;                     //Current checkpoint superblock

  unsigned int           m_SBCount;
  apfs_sb_map_entry*     m_pSBEntry;

  unsigned char          m_MountedVolumesCount;      //Number of mounted subvolumes
  unsigned char          m_TotalVolumesCount;        //Number of all volumes (mounted and not mounted)
  CApfsVolumeSb*         m_pVolSuper;                //Array of volume superblocks

#ifndef UFSD_APFS_RO
  CApfsBitmap*           m_pBlockBitmap;             //Block bitmap for apfs
#endif

  UINT64                 m_SBMapBlockNumber;         //Block number of current checkpoint superblock map
  UINT64                 m_CSBBlockNumber;           //Block number of current checkpoint superblock

public:
  CApfsFileSystem*       m_pFs;                      //Pointer to filesystem object
  api::ICipherFactory*   m_Cf;                       //Pointer to cipher factory
  bool                   m_bNeedFixup;               //Checkpoint Fixup needed

  CApfsSuperBlock(IN api::IBaseMemoryManager* mm, IN api::IBaseLog* log);

  virtual ~CApfsSuperBlock()
  {
    int Status = Dtor();
    if (!UFSD_SUCCESS(Status))
      ULOG_ERROR((GetLog(), Status, "Superblock flushed with error %x", Status));
  }

  int Dtor();

  CApfsTree* GetObjectTree(
      IN unsigned char Index
      ) const
  {
    if (Index < m_MountedVolumesCount)
      return m_pVolSuper[Index].GetObjectTree();
    return NULL;
  }

  CApfsTree* GetExtentTree(
      IN unsigned char Index
      ) const
  {
    if (Index < m_MountedVolumesCount)
      return m_pVolSuper[Index].GetExtentTree();
    return NULL;
  }

  int Init(
      IN  CApfsFileSystem*  pFs,
      OUT size_t*           Flags
      );

  virtual int Flush();

  // Returns the superblock disk structure
  virtual void* SuperBlock() const { return m_pCSB; }

  virtual UINT64 GetFreeBlocksCount() const;
  virtual UINT64 GetBlocksCount() const { return m_pCSB->sb_total_blocks; }
  virtual UINT64 GetInodesCount() const { return MINUS_ONE_64; }
  virtual UINT64 GetFreeInodesCount() const { return GetInodesCount() - GetUsedInodesCount(); }
  virtual UINT64 GetUsedInodesCount() const
  {
    UINT64 Inodes = 0;
    for (unsigned char i = 0; i < m_MountedVolumesCount; ++i)
      Inodes += GetVolume(i)->GetInodesCount();
    return Inodes;
  }

  //Work with inode cache
  virtual int GetInode(
      IN  UINT64        Id,
      OUT CUnixInode**  ppInode,
      IN  bool          fCreate = false
      );

  virtual int UnlinkInode(
      IN CUnixInode* pInode
  );

  // Copy bitmap to the supplied buffer
  virtual int GetBitmap(
      IN unsigned char* Buf,
      IN size_t         BytesOffset,
      IN size_t         Bytes
      )
  {
#ifdef UFSD_APFS_RO
    UNREFERENCED_PARAMETER(Buf);
    UNREFERENCED_PARAMETER(BytesOffset);
    UNREFERENCED_PARAMETER(Bytes);
    return ERR_NOTIMPLEMENTED;
#else
    return m_pBlockBitmap->GetBitmap(Buf, BytesOffset, Bytes);
#endif
  }

#ifndef UFSD_APFS_RO
  CApfsBitmap* GetBitmap() const { return m_pBlockBitmap; }

  int GenerateNextInodeNum(
      IN  unsigned int  Index,
      OUT UINT64*       InodeNum
      ) const;

  size_t FindFree(
      IN  unsigned char VolumeIndex,
      IN  size_t        ToAllocate,
      IN  UINT64        Hint,
      IN  const UINT64* MaxAlloc,
      IN  unsigned int  Flags,
      OUT UINT64*       Allocated
      ) const;

  int FindMaxFree(
      IN unsigned char  VolumeIndex,
      IN  size_t        ToAllocate,
      IN  UINT64        Hint,
      IN  const UINT64* MaxAlloc,
      IN  unsigned int  Flags,
      OUT UINT64*       Lcn,
      OUT size_t*       Len
      ) const;

  int MarkAsFree(
      IN unsigned char  VolumeIndex,
      IN bool           bInVolTree,
      IN UINT64         FirstBit,
      IN size_t         Bits
      );

  int MarkAsUsed(
      IN unsigned char  VolumeIndex,
      IN UINT64         InodeId,
      IN UINT64         FirstBit,
      IN size_t         Bits
      ) const;
#endif

  //Disk space management
  virtual int AllocDiskSpace(
      IN  size_t    /*Len*/,
      OUT UINT64*   /*Lcn*/,
      OUT size_t*   /*OutLen*/ = NULL,
      IN  UINT64    /*HintLcn*/ = 0
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int ReleaseDiskSpace(
      IN UINT64    /*Lcn*/,
      IN size_t    /*Len*/,
      IN bool      /*InCache*/
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  //Is read only (for file or dir if Id != 0, or for whole fs if Id == 0)
  virtual bool IsReadOnly(
      IN UINT64 Id = 0
      ) const;

  apfs_sb* GetSuper() const { return m_pCSB; }
  int LoadSBMap();

  unsigned char* GetFsId() const { return m_pCSB->sb_uuid; }
  UINT64 GetCheckPointId() const{ return m_pCSB->header.checkpoint_id; }

  CApfsVolumeSb* GetVolume(
      IN unsigned char Index
      ) const
  {
    if (Index >= m_MountedVolumesCount)
      return NULL;
    return &m_pVolSuper[Index];
  }
  unsigned char GetMountedVolumesCount() const { return m_MountedVolumesCount; }
  unsigned char GetTotalVolumesCount() const { return m_TotalVolumesCount; }

  virtual int ReadBytes(
      IN  UINT64  Offset,
      OUT void*   pBuffer,
      IN  size_t  Bytes
      ) const
  {
    assert(Offset + Bytes <= m_pCSB->sb_total_blocks << m_Log2OfCluster);
    return m_Rw->ReadBytes(Offset, pBuffer, Bytes);
  }

  virtual int ReadBytes(
      IN  UINT64        Offset,
      OUT void*         pBuff,
      IN  size_t        Bytes,
      IN  unsigned char VolumeIndex,
      IN  bool bMetaData
      ) const
  {
    if (VolumeIndex == BLOCK_BELONGS_TO_CONTAINER)
      return ReadBytes(Offset, pBuff, Bytes);
    if (VolumeIndex >= m_MountedVolumesCount)
      return ERR_BADPARAMS;
    if (bMetaData)
      return m_pVolSuper[VolumeIndex].ReadMetaData(Offset, pBuff, Bytes);
    return ERR_NOTIMPLEMENTED;
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

  //Find block number by it id in superblock map. Parameter type for checking fs integrity
  int GetMetaLocationBlock(
      IN  UINT64          Id,
      IN  unsigned short  Type,
      OUT UINT64*         Block,
      OUT unsigned int*   SizeInBytes
      ) const;

  UINT64 GenerateNextBlockId()
  {
    m_bDirty = true;
    return m_pCSB->sb_next_block_id++;
  }

  static UINT64 CreateFSum(
      IN void*  pData,
      IN size_t Size
      );

  static bool   CheckFSum(
      IN void*  pData,
      IN size_t Size
      );

  //=============================================================================
  //                          Encryption
  //=============================================================================

  //Calculate encryption key, initialize aes objects
  int LoadEncryptionKeys(
      IN PreInitParams* Params,
      IN size_t*        Flags
      );

  //Read blocks and decrypt them
  int ReadEncryptedBlocks(
    IN  UINT64  StartBlock,
        OUT void*   pBuffer,
        IN  size_t  NumBlocks,
        IN  const void* uuid
  ) const;

  int DecryptBlocks(
      IN  api::ICipher* pAes,
      IN  UINT64        StartBlock,
      OUT void*         pBuffer,
      IN  size_t        NumBlocks
      ) const
  {
    UINT64 StartSector = StartBlock << (m_Log2OfCluster - APFS_ENCRYPT_PORTION_LOG);
    size_t Sectors = NumBlocks << (m_Log2OfCluster - APFS_ENCRYPT_PORTION_LOG);

#ifdef UFSD_DRIVER_LINUX
    if (NumBlocks == 1)
#endif
      return DecryptSectors(pAes, StartSector, pBuffer, Sectors);

#ifdef UFSD_DRIVER_LINUX
    // Don't ask. Just believe [UAPFS-422]
    void* pTmp = Malloc2(APFS_ENCRYPT_PORTION);
    CHECK_PTR(pTmp);

    int Status = ERR_NOERROR;
    for (size_t i = 0; i < Sectors && UFSD_SUCCESS(Status); ++i)
    {
      Memcpy2(pTmp, pBuffer, APFS_ENCRYPT_PORTION);
      Status = DecryptSectors(pAes, StartSector++, pTmp, 1);
      Memcpy2(pBuffer, pTmp, APFS_ENCRYPT_PORTION);
      pBuffer = Add2Ptr(pBuffer, APFS_ENCRYPT_PORTION);
    }

    Free2(pTmp);
    return Status;
#endif
  }

  int DecryptSectors(
      IN  api::ICipher* pAes,
      IN  UINT64        StartSector,
      OUT void*         pBuffer,
      IN  size_t        NumSectors
      ) const;

  int DecryptBlocks(
      IN  unsigned char Index,
      IN  UINT64        StartBlock,
      OUT void*         pBuffer,
      IN  size_t        NumBlocks
      ) const;

  //Checks if keybag signature correct
  int CheckKeyBag(
      IN apfs_keybag* pKeyBag,
      IN size_t       SizeInBlocks,
      IN unsigned int BlockType
      ) const;

  //Set or clear flag for checkpoint fixup
  void SetNeedFixup(bool bNeedFixup = true) { m_bNeedFixup = bNeedFixup; }

private:
  int CheckSuperblockHeader(
      IN apfs_sb* sb,
      IN UINT64   BlockNumber,
      IN bool     bLogging = false
      ) const;

  void TraceSuperblockHeader(
      IN apfs_sb* sb
      ) const;

#ifndef UFSD_APFS_RO
  //Fixup checkpoint for superblock, superblock map and all structures addressed from superblock map
  //CheckpointDiff - Difference before container superblock checkpoint and volume superblock checlpoint
  int CheckpointFixup(unsigned int CheckpointDiff);

  //Help function for checkpoint fixup
  //NumCheckpoints - number checkpoint to add
  int UpdateCheckPoint(apfs_block_header *pHdr, UINT64 BlockNum, unsigned int NumCheckpoints) const;
#endif

#ifdef UFSD_APFS_TRACE
  int TraceRawApfs();
  int TraceApfs();
  int EnumBitmap(bool fRangesOnly);
  int TraceApfsBlocks(UINT64 FirstBlock, UINT64 Count, bool fHistory = false);
  int TraceBlock(UINT64 BlockNumber, void* pBlock, bool fHistory = false);
  int TraceTable(void* pBlock) const;
  int TraceTableContent(const apfs_block_header* pBH, const apfs_table_header* pTab) const;
  int TraceApfsVolume(CApfsVolumeSb* pVol);
  int TraceTableDump(UINT64 Block, CApfsTree* pLocation);
  int TraceKeybagBlocks(UINT64 BlockNumber, UINT64 Count, const unsigned char* uuid);
#endif
};

} // namespace apfs

} // namespace UFSD

#endif  //__UFSD_APFS_SUPER_H
