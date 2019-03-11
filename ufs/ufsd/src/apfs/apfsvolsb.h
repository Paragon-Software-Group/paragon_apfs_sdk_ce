// <copyright file="apfsvolsb.h" company="Paragon Software Group">
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
//     10-October-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __UFSD_APFS_VOL_SUPER_H
#define __UFSD_APFS_VOL_SUPER_H

namespace UFSD
{

namespace apfs
{

class CApfsSuperBlock;
class CApfsTree;

class CApfsVolumeSb
{
  friend class CApfsSuperBlock;
#if defined UFSD_APFS_FORMAT
  friend struct CApfsFormat;
#endif

  CApfsSuperBlock*       m_pSuper;                   //Pointer to container superblock object
  api::IBaseMemoryManager* m_Mm;                     //Pointer to mm for Malloc2 and other macro
  UINT64                 m_BlockNumber;              //Block number where volume sb stored
  struct apfs_vsb*       m_pVSB;                     //Disk structure of volume superblock
  CApfsTree*             m_pLocationTree;            //Location tree (or BTOM - BTree Object Map)
  CApfsTree*             m_pObjectTree;              //Common tree for direntries, inodes, extents, ea and etc
  CApfsTree*             m_pExtentTree;              //Extent tree for volume
  UINT64                 m_ObjectTreeRootBlock;      //Root Block of m_pObjectTree

  unsigned char          m_VolIndex;                 //Index of volume in CApfsSuperBlock::m_pVolSuper array
  bool                   m_bEncrypted;               //Is volume encrypted
  bool                   m_bDirty;                   //Is volume dirty
  bool                   m_bReadOnly;                //Is volume readonly
  bool                   m_bEncryptionKeyFound;      //Is encryption key found
  void*                  m_pEncryptedTempBuffer;

  api::ICipher*          m_pAES;                      //Object for encrypting/decrypting

public:
  CApfsVolumeSb();

  void Init(CApfsSuperBlock* sb, unsigned char VolIndex);
  int Load(UINT64 BlockNumber);
  void Destroy() const;

  apfs_vsb* GetVolumeSb() const { return m_pVSB; }
  CApfsTree* GetLocationTree()const { return m_pLocationTree; }
  CApfsTree* GetObjectTree()const { return m_pObjectTree; }
  CApfsTree* GetExtentTree()const { return m_pExtentTree; }
  char* GetName() const{ return m_pVSB->vsb_volname; }
  UINT64 GetUsedBlocksCount() const { return m_pVSB->vsb_blocks_used; }
  UINT64 GetInodesCount() const
  {
    return m_pVSB->vsb_files_count + m_pVSB->vsb_dirs_count
         + m_pVSB->vsb_symlinks_count + m_pVSB->vsb_spec_files_count;
  }
  UINT64 GetCheckpointId() const { return m_pVSB->header.checkpoint_id; }

  UINT64 GenerateNextInodeNum()
  {
    m_bDirty = true;
    return m_pVSB->vsb_next_cnid++;
  }

  unsigned char* GetUUID() const { return m_pVSB->vsb_uuid; }

  bool IsCaseSensitive() const { return !FlagOn(m_pVSB->vsb_incompat_features, APFS_VSB_CASE_NSENS); }
  bool IsEncrypted() const { return m_bEncrypted; }
  bool IsReadOnly() const { return m_bReadOnly; }
  bool CanDecrypt() const { return !m_bEncrypted || m_bEncryptionKeyFound; }

  UINT64 GetTimeCreated() const { return m_pVSB->vsb_created.time; }
  UINT64 GetTimeModified() const { return m_pVSB->vsb_time_modified; }

  //Read data from disk and decrypt it if required
  int ReadMetaData(UINT64 Offset, void* pBuffer, size_t Bytes) const;

  //Read data from disk and decrypt it if required
  int ReadData(UINT64 Offset, void* pBuffer, size_t Bytes, bool bEncrypted, UINT64 CryptoId);

  //Init encryption structures for object, calculating volume encryption key (vek)
  int InitEncryption(apfs_keys* pVekBlobKey, apfs_keys* pRecsBagPtrKey, const unsigned char* password, size_t PassLen);

  //Init volume tree and location tree
  int InitTrees();

  void SetDirty(bool f = true) { m_bDirty = f; }

private:
  int CheckVolumeHeader(apfs_vsb* vsb) const;
  void TraceVolumeHeader(apfs_vsb* vsb) const;

  //Calculate vek key by vek_blob and kek_blob
  int CalculateVek(apfs_vek* vek_data, apfs_kek* kek_data, const unsigned char* password, size_t PassLen, unsigned char* vek) const;

  //Rfc 3394 encrypt algorithm (ses modification)
  int KeyUnwrap(
    IN  const void* InBuf,
    IN  const void* Key,
    IN  unsigned    Cipher,
    OUT void*       OutBuf,
    OUT UINT64&     IV
  ) const;

  api::IBaseLog* GetLog() const;

#ifndef UFSD_APFS_RO
  size_t CalcAvailableBlocks(size_t BlocksRequired) const;

  int Flush();

  //Add extent to extent tree
  int AddExtentToVolume(UINT64 InodeId, UINT64 FirstBlock, size_t NumBlocks, UINT64 Flags = 0x10) const;
public:
  int UseBlocks(UINT64 InodeId, UINT64 FirstBlock, size_t NumBlocks);
  int FreeBlocks(bool bInVolTree, UINT64 FirstBlock, size_t NumBlocks);
#endif
};

} // namespace apfs

} // namespace UFSD

#endif
