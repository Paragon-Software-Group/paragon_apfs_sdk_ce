// <copyright file="apfsvolsb.cpp" company="Paragon Software Group">
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


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331853 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfsvolsb.h"
#include "apfssuper.h"
#include "apfsbplustree.h"
#ifdef BASE_BIGENDIAN
#include "apfsbe.h"
#endif

namespace UFSD
{

namespace apfs
{


/////////////////////////////////////////////////////////////////////////////
CApfsVolumeSb::CApfsVolumeSb()
    : m_pSuper(NULL)
    , m_Mm(NULL)
    , m_BlockNumber(0)
    , m_pVSB(NULL)
    , m_pLocationTree(NULL)
    , m_pObjectTree(NULL)
    , m_pExtentTree(NULL)
    , m_ObjectTreeRootBlock(0)
    , m_VolIndex(0)
    , m_bEncrypted(false)
    , m_bDirty(false)
    , m_bReadOnly(false)
    , m_bEncryptionKeyFound(false)
    , m_pEncryptedTempBuffer(NULL)
    , m_pAES(NULL)
{}


/////////////////////////////////////////////////////////////////////////////
void
CApfsVolumeSb::Init(CApfsSuperBlock* sb, unsigned char VolIndex)
{
  m_pSuper = sb;
  m_Mm = m_pSuper->m_Mm;
  m_BlockNumber = 0;
  m_pVSB = NULL;
  m_pLocationTree = NULL;
  m_pObjectTree = NULL;
  m_pExtentTree = NULL;
  m_ObjectTreeRootBlock = 0;
  m_VolIndex = VolIndex;
  m_bDirty = false;
  m_bReadOnly = false;
  m_bEncrypted = false;
  m_pAES = NULL;
  m_pEncryptedTempBuffer = NULL;
  m_bEncryptionKeyFound = false;
}


/////////////////////////////////////////////////////////////////////////////
void
CApfsVolumeSb::Destroy() const
{
  Free2(m_pVSB);
  delete m_pLocationTree;
  delete m_pObjectTree;
  delete m_pExtentTree;
  if (m_pAES)
    m_pAES->Destroy();
  Free2(m_pEncryptedTempBuffer);
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::Load(UINT64 BlockNumber)
{
  m_BlockNumber = BlockNumber;

  if (m_pVSB == NULL)
    CHECK_PTR(m_pVSB = reinterpret_cast<apfs_vsb*>(Malloc2(m_pSuper->GetBlockSize())));

  CHECK_CALL(m_pSuper->ReadBytes(BlockNumber * m_pSuper->GetBlockSize(), m_pVSB, m_pSuper->GetBlockSize()));
  BE_ONLY(ConvertVSB(m_pVSB));
  TraceVolumeHeader(m_pVSB);
  CHECK_CALL(CheckVolumeHeader(m_pVSB));

  assert(m_pVSB->vsb_volume_index == m_VolIndex);           //is it always true?

  m_bReadOnly = (m_pVSB->vsb_snapshots_count != 0);

  if (m_bReadOnly && !m_pSuper->IsReadOnly())
    ULOG_TRACE((GetLog(), "Volume #%x is readonly because it has snapshots", m_pVSB->vsb_volume_index));

  //Read Btree object map
  if (m_pLocationTree == NULL)
    CHECK_PTR(m_pLocationTree = new(m_Mm) CApfsTree(m_pSuper));
  CHECK_CALL(m_pLocationTree->Init(m_pVSB->vsb_btom_root, m_VolIndex, true));

  //Read B-Tree Catalog Root Node (BTRN)
  apfs_location_table_data val;
  CHECK_CALL(m_pLocationTree->GetActualLocation(m_pVSB->vsb_root_node_id, NULL, &val));

  if (val.ltd_flags == 4)
  {
    //encrypted
    m_bEncrypted = true;
    ULOG_TRACE((GetLog(), "Encrypted volume, ReadOnly Support"));
    m_bReadOnly = true;
  }

  if (val.ltd_flags != 0 && val.ltd_flags != 4)
  {
    ULOG_WARNING((GetLog(), "Unsupported Flag=%x in the Location Table", val.ltd_flags));
    return ERR_FSUNKNOWN;
  }

  if (val.ltd_length != m_pSuper->GetBlockSize())
  {
    ULOG_WARNING((GetLog(), "Location Table BlockSize=%x and File System BlockSize=%x are differ", val.ltd_length, m_pSuper->GetBlockSize()));
    return ERR_FSUNKNOWN;
  }

  m_ObjectTreeRootBlock = val.ltd_block;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::InitTrees()
{
  if (m_bEncrypted && m_pAES == NULL)
    return ERR_NOERROR;

  if (m_pObjectTree == NULL)
    CHECK_PTR(m_pObjectTree = new(m_Mm) CApfsTree(m_pSuper));
  CHECK_CALL(m_pObjectTree->Init(m_ObjectTreeRootBlock, m_VolIndex, false, m_pLocationTree, true));

  if (m_pExtentTree == NULL)
    CHECK_PTR(m_pExtentTree = new(m_Mm) CApfsTree(m_pSuper));
  CHECK_CALL(m_pExtentTree->Init(m_pVSB->vsb_extent_root, m_VolIndex, false));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::ReadMetaData(UINT64 Offset, void* pBuffer, size_t Bytes) const
{
  if (!m_bEncrypted)
    return m_pSuper->ReadBytes(Offset, pBuffer, Bytes);

  if (m_pAES == NULL)
    return ERR_BADPARAMS;

  assert((Bytes & (m_pSuper->GetBlockSize() - 1)) == 0);
  CHECK_CALL(m_pSuper->ReadBytes(Offset, pBuffer, Bytes));
  CHECK_CALL(m_pSuper->DecryptBlocks(m_pAES, Offset >> m_pSuper->m_Log2OfCluster, pBuffer, Bytes >> m_pSuper->m_Log2OfCluster));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::ReadData(UINT64 Offset, void* pBuffer, size_t Bytes, bool bEncrypted, UINT64 CryptoId)
{
  if (!m_bEncrypted || !bEncrypted)
    return m_pSuper->ReadBytes(Offset, pBuffer, Bytes);
  if (m_pAES == NULL)
    return ERR_BADPARAMS;

  if (m_pEncryptedTempBuffer == NULL)
    CHECK_PTR(m_pEncryptedTempBuffer = Malloc2(APFS_ENCRYPT_PORTION));

  CryptoId = (CryptoId << (m_pSuper->m_Log2OfCluster - APFS_ENCRYPT_PORTION_LOG)) + ((Offset & (m_pSuper->GetBlockSize() - 1)) >> APFS_ENCRYPT_PORTION_LOG);
  unsigned short OffsetInSector = Offset & (APFS_ENCRYPT_PORTION - 1);

  if (OffsetInSector > 0)
  {
    //Read from offset in sector
    size_t BytesToRead = APFS_ENCRYPT_PORTION - OffsetInSector;

    if (BytesToRead > Bytes)
      BytesToRead = Bytes;

    CHECK_CALL(m_pSuper->ReadBytes((Offset >> APFS_ENCRYPT_PORTION_LOG) << APFS_ENCRYPT_PORTION_LOG, m_pEncryptedTempBuffer, APFS_ENCRYPT_PORTION));
    CHECK_CALL(m_pSuper->DecryptSectors(m_pAES, CryptoId, m_pEncryptedTempBuffer, 1));
    Memcpy2(pBuffer, Add2Ptr(m_pEncryptedTempBuffer, OffsetInSector), BytesToRead);

    Bytes -= BytesToRead;
    Offset += BytesToRead;
    pBuffer = Add2Ptr(pBuffer, BytesToRead);
    CryptoId++;
  }

  size_t SectorsToRead = Bytes >> APFS_ENCRYPT_PORTION_LOG;

  if (SectorsToRead > 0)
  {
    //Read aligned sectors
    size_t BytesToRead = SectorsToRead << APFS_ENCRYPT_PORTION_LOG;
    CHECK_CALL(m_pSuper->ReadBytes(Offset, pBuffer, BytesToRead));

#if 0//__WORDSIZE >= 64
    CHECK_CALL(m_pSuper->DecryptSectors(m_pAES, CryptoId, pBuffer, SectorsToRead));
    pBuffer = Add2Ptr(pBuffer, BytesToRead);
    CryptoId += SectorsToRead;
#else
    // for x86 [UAPFS-235]
    for (size_t i = 0; i < SectorsToRead; ++i)
    {
      Memcpy2(m_pEncryptedTempBuffer, pBuffer, APFS_ENCRYPT_PORTION);
      CHECK_CALL(m_pSuper->DecryptSectors(m_pAES, CryptoId++, m_pEncryptedTempBuffer, 1));
      Memcpy2(pBuffer, m_pEncryptedTempBuffer, APFS_ENCRYPT_PORTION);
      pBuffer = Add2Ptr(pBuffer, APFS_ENCRYPT_PORTION);
    }
#endif

    Bytes  -= BytesToRead;
    Offset += BytesToRead;
  }

  if (Bytes > 0)
  {
    //Read tail
    CHECK_CALL(m_pSuper->ReadBytes(Offset, m_pEncryptedTempBuffer, APFS_ENCRYPT_PORTION));
    CHECK_CALL(m_pSuper->DecryptSectors(m_pAES, CryptoId, m_pEncryptedTempBuffer, 1));
    Memcpy2(pBuffer, m_pEncryptedTempBuffer, Bytes);
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::CheckVolumeHeader(apfs_vsb* vsb) const
{
  if (vsb->vsb_magic != APFS_VSB_MAGIC)
  {
    ULOG_WARNING((GetLog(), "Invalid apfs volume superblock magic"));
    return ERR_FSUNKNOWN;
  }

  int Status = ERR_NOFSINTEGRITY;

  if (vsb->header.block_type != APFS_TYPE_VOLUME_SUPERBLOCK)
    ULOG_ERROR((GetLog(), Status, "block type of volume superblock is %x, should be %x", vsb->header.block_type, APFS_TYPE_VOLUME_SUPERBLOCK));
  else if (vsb->header.checkpoint_id > m_pSuper->GetCheckPointId())
    ULOG_ERROR((GetLog(), Status, "Checkpoint id for volume (0x%" PLL "x) must be less or equal than for container (0x%" PLL "x)", vsb->header.checkpoint_id, m_pSuper->GetCheckPointId()));
  else if (vsb->vsb_blocks_quota != 0 && m_pVSB->vsb_blocks_quota < m_pVSB->vsb_blocks_used)
    ULOG_ERROR((GetLog(), Status, "Blocks quota (0x%" PLL "x) should be less or equal than number of blocks used (0x%" PLL "x)", m_pVSB->vsb_blocks_quota, m_pVSB->vsb_blocks_used));
//  else if (m_pVSB->vsb_free_count > m_pVSB->vsb_allocate_count)
//    ULOG_ERROR((GetLog(), Status, "Number of allocated blocks (0x%" PLL "x) should be less or equal than number of freed (0x%" PLL "x)", vsb->vsb_allocate_count, vsb->vsb_free_count));
  else if (!m_pSuper->CheckFSum(vsb, m_pSuper->GetBlockSize()))
    ULOG_ERROR((GetLog(), Status, "Wrong crc for volume superblock"));
  else
    Status = ERR_NOERROR;

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
void
CApfsVolumeSb::TraceVolumeHeader(apfs_vsb* vsb) const
{
#if !defined UFSD_TRACE
  UNREFERENCED_PARAMETER(vsb);
#endif

  ULOG_INFO((GetLog(), "=== Parameters of APFS volume %d ======", vsb->vsb_volume_index));
  TRACE_ONLY(unsigned char* m = reinterpret_cast<unsigned char*>(&vsb->vsb_magic));
  ULOG_INFO((GetLog(), "Magic [APSB]    : %c%c%c%c", m[0], m[1], m[2], m[3]));
  ULOG_INFO((GetLog(), "Object id       : 0x%" PLL "x", vsb->header.id));
  ULOG_INFO((GetLog(), "Checkpoint id   : 0x%" PLL "x", vsb->header.checkpoint_id));
  ULOG_INFO((GetLog(), "Volume name     : %s", vsb->vsb_volname));
  ULOG_INFO((GetLog(), "Blocks used     : 0x%" PLL "x", vsb->vsb_blocks_used));
  ULOG_INFO((GetLog(), "LocTree root    : 0x%" PLL "x", vsb->vsb_btom_root));
  ULOG_INFO((GetLog(), "ExtentTree root : 0x%" PLL "x", vsb->vsb_extent_root));
  ULOG_INFO((GetLog(), "Dirs            : 0x%" PLL "x", vsb->vsb_dirs_count));
  ULOG_INFO((GetLog(), "Files           : 0x%" PLL "x", vsb->vsb_files_count));
  ULOG_INFO((GetLog(), "Symlinks        : 0x%" PLL "x", vsb->vsb_symlinks_count));
  ULOG_INFO((GetLog(), "Other           : 0x%" PLL "x", vsb->vsb_spec_files_count));
  ULOG_INFO((GetLog(), "Snapshots       : 0x%" PLL "x", vsb->vsb_snapshots_count));
  ULOG_INFO((GetLog(), "FileNames       : case %ssensitive", IsCaseSensitive() ? "" : "in"));
  ULOG_INFO((GetLog(), "Inc. features   : 0x%" PLL "x", vsb->vsb_incompat_features));
  ULOG_INFO((GetLog(), "====================================="));
}


/////////////////////////////////////////////////////////////////////////////
api::IBaseLog*
CApfsVolumeSb::GetLog() const
{
  return m_pSuper->GetLog();
}


} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
