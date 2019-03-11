// <copyright file="apfssuper.cpp" company="Paragon Software Group">
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


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331828 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixfs.h"
#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfsvolsb.h"
#include "apfssuper.h"
#include "apfsinode.h"
#include "apfs.h"
#include "apfstable.h"
#include "apfsbplustree.h"
#ifdef BASE_BIGENDIAN
#include "apfsbe.h"
#endif

namespace UFSD
{

namespace apfs
{

/////////////////////////////////////////////////////////////////////////////
CApfsSuperBlock::CApfsSuperBlock(IN api::IBaseMemoryManager* mm, IN api::IBaseLog* log)
  : CUnixSuperBlock(mm, log)
  , m_pMSB(NULL)
  , m_pCSB(NULL)
  , m_SBCount(0)
  , m_pSBEntry(NULL)
  , m_MountedVolumesCount(0)
  , m_TotalVolumesCount(0)
  , m_pVolSuper(NULL)
#ifndef UFSD_APFS_RO
  , m_pBlockBitmap(NULL)
#endif
  , m_SBMapBlockNumber(0)
  , m_CSBBlockNumber(0)
  , m_pFs(NULL)
  , m_Cf(NULL)
  , m_bNeedFixup(false)
{
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::Dtor()
{
  int Status = Flush();
#ifndef UFSD_APFS_RO
  CheckpointFixup(3);
#endif

  Free2(m_pMSB);
  Free2(m_pCSB);
  Free2(m_pSBEntry);

  for (unsigned int i = 0; i < m_TotalVolumesCount; i++)
    m_pVolSuper[i].Destroy();
  Free2(m_pVolSuper);

#ifndef UFSD_APFS_RO
  delete m_pBlockBitmap;
#endif

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::Init(
    IN  CApfsFileSystem*  pFs,
    OUT size_t*           Flags
    )
{
  m_bInited = false;

  m_Rw      = pFs->m_Rw;
  m_Strings = pFs->m_Strings;
  m_Time    = pFs->m_Time;
  m_pFs     = pFs;

  //Read, check trace main superblock(msb)
  if (m_pMSB == NULL)
    CHECK_PTR(m_pMSB = reinterpret_cast<apfs_sb*>(Malloc2(APFS_MSB_SIZE)));

  CHECK_CALL(m_Rw->ReadBytes(APFS_MSB_OFFSET, m_pMSB, APFS_MSB_SIZE));
  BE_ONLY(ConvertSB(m_pMSB));

  m_BlockSize = m_pMSB->sb_block_size;
  if (m_pMSB->sb_magic          == APFS_CSB_MAGIC
  &&  m_pMSB->header.block_type == APFS_TYPE_SUPERBLOCK
  &&  m_BlockSize != 0
  && (m_BlockSize & (m_BlockSize - 1)) == 0)
  {
    if(m_BlockSize != APFS_MSB_SIZE)
    {
      //block size != 0x1000
      assert(!"Unexpected block size");
      Free2(m_pMSB);
      CHECK_PTR(m_pMSB = reinterpret_cast<apfs_sb*>(Malloc2(m_BlockSize)));

      CHECK_CALL(m_Rw->ReadBytes(APFS_MSB_OFFSET, m_pMSB, m_BlockSize));
      BE_ONLY(ConvertSB(m_pMSB));
    }
  }

  m_Log2OfCluster = api::GetLog2OfBlockSize(m_BlockSize);
  m_InodeSize = sizeof(apfs_inode);

#ifdef BASE_BIGENDIAN
  ULOG_INFO((m_Log, "Parameters of APFS->be:"));
#else
  ULOG_INFO((m_Log, "Parameters of APFS->le:"));
#endif
  ULOG_INFO((m_Log, "=== Main superblock header ======"));
  TraceSuperblockHeader(m_pMSB);

#ifdef BASE_BIGENDIAN
  {
    ULOG_WARNING((m_Log, "Big endian is not supported"));
    return ERR_FSUNKNOWN;
  }
#endif

  //TraceRawApfs();
  CHECK_CALL_SILENT(CheckSuperblockHeader(m_pMSB, 0, true));

  //Read and check checkpoint superblock (csb) which addressed from main superblock (msb)
  if (m_pCSB == NULL)
    CHECK_PTR(m_pCSB = reinterpret_cast<apfs_sb*>(Malloc2(m_BlockSize)));

  //Prefetch all sb range
  C_ASSERT(sizeof(m_pMSB->sb_first_sb) == 8);
  CHECK_CALL(m_Rw->ReadBytes(m_pMSB->sb_first_sb * m_BlockSize, NULL, m_pMSB->sb_number_of_sb * m_BlockSize, RWB_FLAGS_PREFETCH));

  UINT64 SbAreaBorder = m_pMSB->sb_first_sb + m_pMSB->sb_number_of_sb;

  CHECK_CALL(m_pFs->FindCSBBlock(m_pMSB, m_CSBBlockNumber));
  CHECK_CALL(m_Rw->ReadBytes(m_CSBBlockNumber << m_Log2OfCluster, m_pCSB, m_BlockSize));
  CHECK_CALL(CheckSuperblockHeader(m_pCSB, m_CSBBlockNumber, false));

  assert(m_pCSB->header.checkpoint_id >= m_pMSB->header.checkpoint_id);

  ULOG_INFO((m_Log, "=== Last checkpoint superblock header ======"));
  TraceSuperblockHeader(m_pCSB);

  //Initialize superblock from previous checkpoints
  if (m_pFs->m_Params.CheckpointsAgo > 0)
  {
    ULOG_INFO((m_Log, "Searching container superblock 0x%" PLL "x checkpoints ago", m_pFs->m_Params.CheckpointsAgo));
    if (m_pFs->m_Params.CheckpointsAgo > m_pCSB->header.checkpoint_id)
    {
      UFSDTracek((m_pFs->m_Sb, "Unable to mount the volume with history=%" PLL "x -> last checkpoint is %" PLL "x",
        m_pFs->m_Params.CheckpointsAgo, m_pCSB->header.checkpoint_id));
      ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Unable to mount the volume with history=%" PLL "x -> last checkpoint is %" PLL "x",
        m_pFs->m_Params.CheckpointsAgo, m_pCSB->header.checkpoint_id));
      return ERR_NOFSINTEGRITY;
    }

    UINT64 RequiredCheckPoint = m_pCSB->header.checkpoint_id - m_pFs->m_Params.CheckpointsAgo;
    bool bFound = false;
    UINT64 FirstSb = m_pCSB->sb_first_sb;
    UINT64 n;

    for (n = FirstSb; n < SbAreaBorder; n++)
    {
      CHECK_CALL( m_Rw->ReadBytes(n << m_Log2OfCluster, m_pCSB, m_BlockSize) );
      BE_ONLY(ConvertSB(m_pCSB));

      if (m_pCSB->sb_magic == APFS_CSB_MAGIC && m_pCSB->header.checkpoint_id == RequiredCheckPoint)
      {
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      UFSDTracek((m_pFs->m_Sb, "Unable to mount the volume with history=%" PLL "x -> Can't find required checkpoint", m_pFs->m_Params.CheckpointsAgo));
      ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Unable to mount the volume with history=%" PLL "x -> Can't find required checkpoint", m_pFs->m_Params.CheckpointsAgo));
      return ERR_NOFSINTEGRITY;
    }

    CHECK_CALL(CheckSuperblockHeader(m_pCSB, n, true));

    ULOG_INFO((m_Log, "=== Found checkpoint superblock header ======"));
    m_CSBBlockNumber = n;
    TraceSuperblockHeader(m_pCSB);
    m_bReadOnly = true;
  }

  //Read checkpoint superblock map (sb map)
  m_SBMapBlockNumber = m_CSBBlockNumber - m_pCSB->sb_current_sb_len + 1;

  if (m_SBMapBlockNumber > SbAreaBorder || m_SBMapBlockNumber < m_pCSB->sb_first_sb) //as we have unsigned number
    //SB area loop -> last entry
    m_SBMapBlockNumber += m_pCSB->sb_number_of_sb;

  CHECK_CALL(LoadSBMap());

#ifndef UFSD_APFS_RO
  //init bitmap
  if (m_pBlockBitmap == NULL)
  {
    CHECK_PTR(m_pBlockBitmap = new (m_Mm) CApfsBitmap(m_Mm));
    CHECK_CALL(m_pBlockBitmap->Init(this));
  }
  assert( m_pBlockBitmap->IsUsed( m_pMSB->sb_first_sb, m_pMSB->sb_number_of_sb ) );
  assert( m_pBlockBitmap->IsUsed( m_pMSB->sb_first_meta, m_pMSB->sb_number_of_meta ) );
  assert( m_pBlockBitmap->IsUsed( m_pCSB->sb_volume_root_block ) );
#endif

  //Read volume superblocks
  CApfsTree VolTree(this);
  CHECK_CALL(VolTree.Init(m_pCSB->sb_volume_root_block, BLOCK_BELONGS_TO_CONTAINER, true));

  UINT64 VolumesCount = VolTree.Count();

  if (VolumesCount == 0)
  {
    UFSDTracek((m_pFs->m_Sb, "Apfs container has 0 volumes"));
    return ERR_NOROOTDIR;
  }

  if (VolumesCount > APFS_MAX_SUBVOLUMES)
  {
    UFSDTracek((m_pFs->m_Sb, "Number of volumes (0x%" PLL "x) is more then maximal (0x%x)", VolumesCount, APFS_MAX_SUBVOLUMES));
    return ERR_FSUNKNOWN;
  }

  m_TotalVolumesCount = static_cast<unsigned char>(VolumesCount);
  m_MountedVolumesCount = FlagOn(m_pFs->m_Options, UFSD_OPTIONS_MOUNT_ALL_VOLUMES) ? m_TotalVolumesCount : 1;
  ULOG_INFO((m_Log, "Apfs container has %d volumes", m_MountedVolumesCount));

  if (m_pVolSuper == NULL)
  {
    CHECK_PTR(m_pVolSuper = reinterpret_cast<CApfsVolumeSb*>(Malloc2(m_TotalVolumesCount * sizeof(CApfsVolumeSb))));

    for (unsigned char i = 0; i < m_TotalVolumesCount; i++)
      m_pVolSuper[i].Init(this, i);
  }

  apfs_location_table_data* VolData;
  bool bHasEncryptedVolumes = false;

  for (unsigned char i = 0; i < m_MountedVolumesCount; i++)
  {
    CSimpleSearchKey Key(m_Mm, m_pCSB->sb_volumes_id[i]);
    CHECK_CALL( VolTree.GetData(&Key, SEARCH_KEY_EQ, (void**)&VolData) );

#ifdef BASE_BIGENDIAN
    SwapBytesInPlace(&VolData->ltd_flags);
    SwapBytesInPlace(&VolData->ltd_length);
    SwapBytesInPlace(&VolData->ltd_block);
#endif

    if (VolData->ltd_flags != 0 || VolData->ltd_length != m_BlockSize)
    {
      ULOG_WARNING((GetLog(), "volume location flags=0x%x length=0x%x and container block size (0x%x) are different", VolData->ltd_flags, VolData->ltd_length, m_BlockSize));
      return ERR_FSUNKNOWN;
    }
    CHECK_CALL(m_pVolSuper[i].Load(VolData->ltd_block));
    bHasEncryptedVolumes |= m_pVolSuper[i].IsEncrypted();
    if ( m_pVolSuper[i].IsEncrypted() )
      m_pVolSuper[i].m_bReadOnly = true;
  }

  if (Flags)
  {
    if (m_pVolSuper[0].IsEncrypted())
      SetFlag(*Flags, APFS_FLAGS_UNSUPPORTED_ENCRYPTED);
    if (m_pVolSuper[0].m_pVSB->vsb_snapshots_count != 0)
      SetFlag(*Flags, APFS_FLAGS_UNSUPPORTED_SNAPSHOT);
  }

#ifdef UFSD_APFS_TRACE
  //TraceRawApfs();
#endif

  if (!m_bInited && bHasEncryptedVolumes && m_pCSB->sb_keybag_block != 0 && m_pCSB->sb_keybag_count != 0)
    CHECK_CALL( LoadEncryptionKeys(&m_pFs->m_Params, Flags) );

  for (unsigned char i = 0; i < m_MountedVolumesCount; i++)
    CHECK_CALL(m_pVolSuper[i].InitTrees());

#ifdef UFSD_APFS_TRACE
  //TraceApfs();
#endif

  m_bInited = true;

#ifdef UFSD_APFS_RO
  m_bReadOnly = true;
#endif

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::LoadSBMap()
{
  //Read checkpoint superblock map (sb map)
  Free2(m_pSBEntry);
  unsigned int MaxEntries = (m_pCSB->sb_current_sb_len - 1) * MAX_SBMAP_ENTRIES;
  CHECK_PTR(m_pSBEntry = reinterpret_cast<apfs_sb_map_entry*>(Malloc2(sizeof(apfs_sb_map_entry) * MaxEntries)));
  m_SBCount = 0;

  apfs_sb_map* pMap = reinterpret_cast<apfs_sb_map*>(Malloc2(m_BlockSize));
  CHECK_PTR(pMap);

  UINT64 SbAreaBorder = m_pCSB->sb_first_sb + m_pCSB->sb_number_of_sb;
  for (unsigned int i = 0; i < m_pCSB->sb_current_sb_len - 1; ++i)
  {
    UINT64 Block = m_SBMapBlockNumber + i;

    if (Block >= SbAreaBorder)
      //SB area loop -> first entry
      Block = m_pMSB->sb_first_sb;

    int Status = ReadBlocks(Block, pMap);
    if (Status != ERR_NOERROR)
    {
      ULOG_ERROR((m_Log, Status, "Error reading superblock map"));
      Free2(pMap);
      return Status;
    }

    BE_ONLY(ConvertSBMap(pMap));

    if (pMap->header.id            != Block
    ||  pMap->header.checkpoint_id != m_pCSB->header.checkpoint_id
    ||  pMap->header.block_type    != APFS_TYPE_SUPERBLOCK_MAP)
    {
      ULOG_WARNING((m_Log, "Wrong block header in the checkpoint superblock map"));
      Free2(pMap);
      return ERR_NOFSINTEGRITY;
    }

    m_SBCount += pMap->entries_count;
    Memcpy2(m_pSBEntry + i * MAX_SBMAP_ENTRIES, pMap->entries, pMap->entries_count * sizeof(apfs_sb_map_entry));
  }

  Free2(pMap);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::GetMetaLocationBlock(
    IN UINT64         Id,
    IN unsigned short Type,
    OUT UINT64*       StartBlock,
    OUT unsigned int* SizeInBytes
    ) const
{
  for (unsigned int i = 0; i < m_SBCount; i++)
  {
    if (Id == m_pSBEntry[i].object_id)
    {
      if (Type != m_pSBEntry[i].block_type)
      {
        ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "SbMap: Id=0x%" PLL "x has Type=0x%x, expected Type 0x%x", Id, m_pSBEntry[i].block_type, Type));
        return ERR_NOFSINTEGRITY;
      }

      if (StartBlock)
        *StartBlock = m_pSBEntry[i].block;
      if (SizeInBytes)
        *SizeInBytes = m_pSBEntry[i].size;
      return ERR_NOERROR;
    }
  }

  ULOG_ERROR((GetLog(), ERR_NOTFOUND, "Id=0x%" PLL "x, Type=0x%x not found in sb map", Id, Type));
  return ERR_NOTFOUND;
}


/////////////////////////////////////////////////////////////////////////////
UINT64 CApfsSuperBlock::GetFreeBlocksCount() const
{
#ifndef UFSD_APFS_RO
  return m_pBlockBitmap->GetZeros();
#else
  //Read bitmap descriptor
  UINT64 DescBlock;
  int Status = GetMetaLocationBlock(m_pCSB->sb_spaceman_id, APFS_TYPE_BITMAP_DESCRIPTOR, &DescBlock, NULL);

  if (!UFSD_SUCCESS(Status))
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Bitmap descriptor isn't found"));
    return 0;
  }

  apfs_bmd Desc;
  Status = ReadBytes(DescBlock << m_Log2OfCluster, &Desc, sizeof(Desc));

  if (UFSD_SUCCESS(Status))
    return Desc.free_blocks + Desc.d2_free_blocks;

  ULOG_ERROR((GetLog(), Status, "Failed to read bitmap descriptor"));
  return 0;
#endif
}


/////////////////////////////////////////////////////////////////////////////
void CApfsSuperBlock::TraceSuperblockHeader(
    IN apfs_sb* sb
    ) const
{
#if !defined UFSD_TRACE
  UNREFERENCED_PARAMETER(sb);
#endif

  TRACE_ONLY(unsigned char* m = reinterpret_cast<unsigned char*>(&sb->sb_magic));

  ULOG_INFO((m_Log, "Magic [NXSB]  : %c%c%c%c", m[0], m[1], m[2], m[3]));
  ULOG_INFO((m_Log, "Checkpoint id : %#" PLL "x, next: %" PLL "x", sb->header.checkpoint_id, sb->sb_next_checkpoint_id));
  ULOG_INFO((m_Log, "Total blocks  : %#" PLL "x, %s", sb->sb_total_blocks, m_Rw->IsReadOnly() ? "ro" : "rw"));
  ULOG_INFO((m_Log, "BlockSize     : %#x", sb->sb_block_size));
  ULOG_INFO((m_Log, "Current SB    : %#x + %x", sb->sb_current_sb, sb->sb_current_sb_len));
  ULOG_INFO((m_Log, "SB area       : %#" PLL "x + %x, next: %x", sb->sb_first_sb, sb->sb_number_of_sb, sb->sb_next_sb));
  ULOG_INFO((m_Log, "Meta area     : %#" PLL "x + %x, next: %x", sb->sb_first_meta, sb->sb_number_of_meta, sb->sb_next_meta));
  ULOG_INFO((m_Log, "CSB map       : %#" PLL "x", sb->sb_current_sb + sb->sb_first_sb));
  ULOG_INFO((m_Log, "Volume root   : %" PLL "x", sb->sb_volume_root_block));
  ULOG_INFO((m_Log, "KeyBag        : %" PLL "x + %" PLL "x", sb->sb_keybag_block, sb->sb_keybag_count));

  ULOG_INFO((m_Log, "====================================="));
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::CheckSuperblockHeader(
    IN apfs_sb* sb,
    IN UINT64   Block,
    IN bool     bLogging
    ) const
{
#if !defined UFSD_TRACE
  UNREFERENCED_PARAMETER(Block);
#endif

  int Status = ERR_NOERROR;

  if (sb->sb_magic != APFS_CSB_MAGIC)
  {
    ULOG_WARNING((m_Log, "Invalid apfs %s superblock magic", Block == 0 ? "main" : "checkpoint"));
    Status = ERR_FSUNKNOWN;
  }
  else if (sb->header.block_type != APFS_TYPE_SUPERBLOCK)
  {
    ULOG_WARNING((m_Log, "block type of superblock is %x, should be %x", sb->header.block_type, APFS_TYPE_SUPERBLOCK));
    Status = ERR_NOFSINTEGRITY;
  }
  else if (m_BlockSize == 0 || (m_BlockSize & (m_BlockSize - 1)) != 0)
  {
    ULOG_WARNING((m_Log, "block size should be power of 2 an not zero"));
    Status = ERR_NOFSINTEGRITY;
  }
  else if (sb->sb_total_blocks == 0)
  {
    ULOG_WARNING((m_Log, "Invalid total blocks 0"));
    Status = ERR_NOFSINTEGRITY;
  }
  else if (sb->sb_total_blocks > APFS_MAX_SIZE_IN_BLOCKS)
  {
    ULOG_WARNING((m_Log, "Total blocks (0x%" PLL "x) > max size in blocks (0x%x)", sb->sb_total_blocks, APFS_MAX_SIZE_IN_BLOCKS));
    Status = ERR_NOTIMPLEMENTED;
  }
  else if (sb->sb_max_volumes == 0 || sb->sb_max_volumes > APFS_MAX_SUBVOLUMES)
  {
    ULOG_WARNING((m_Log, "Max number of subvolumes must be between 1 to %d", APFS_MAX_SUBVOLUMES));
    Status = ERR_FSUNKNOWN;
  }
  else if (!CheckFSum(sb, m_BlockSize))
  {
    ULOG_WARNING((m_Log, "Wrong superblock crc, BlockNum=0x%" PLL "x", Block));
    Status = ERR_NOFSINTEGRITY;
  }

  if (Status != ERR_NOERROR && bLogging && m_CSBBlockNumber != 0)
  {
    ULOG_TRACE((m_Log, "Main superblock:"));
    ULOG_DUMP((m_Log, LOG_LEVEL_TRACE, m_pMSB, sizeof(apfs_sb)));
    ULOG_TRACE((m_Log, "Checkpoint superblock (Block 0x%" PLL "x):", Block));
    ULOG_DUMP((m_Log, LOG_LEVEL_TRACE, m_pCSB, sizeof(apfs_sb)));
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::GetInode(
    IN  UINT64        Id,
    OUT CUnixInode**  ppInode,
    IN  bool          fCreate
    )
{
  UINT64 InodeIdToSearch;

  if (APFS_GET_INODE_ID(Id) == APFS_VOLUMES_DIR_ID && Id != APFS_VOLUMES_DIR_ID)
    InodeIdToSearch = Id & APFS_MAKE_INODE_ID(0xFF, APFS_ROOT_INO);
  else
    InodeIdToSearch = Id;

  return GetInodeT<CApfsInode>(InodeIdToSearch, ppInode, fCreate);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::CreateCacheBlock(
    IN  UINT64        Block,
    OUT CUnixBlock**  ppNewBlock,
    IN  bool          fCreate,
    IN  unsigned int  CacheBlockSize,
    IN  unsigned char VolIndex,
    IN  bool          bCalcCrc
    )
{
  CUnixBlock* pBlock = new(m_Mm) CApfsBlock(m_Mm);
  CHECK_PTR(pBlock);

  int Status = pBlock->Init(this, Block, fCreate, CacheBlockSize, VolIndex, bCalcCrc);
  if (!UFSD_SUCCESS(Status))
  {
    delete pBlock;
    return Status;
  }

  if (!fCreate && bCalcCrc && !CheckFSum(pBlock->GetBuffer(), GetBlockSize()))
  {
    TRACE_ONLY(apfs_block_header* header = reinterpret_cast<apfs_block_header*>(pBlock->GetBuffer()));
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Wrong checksum: BlockNum=0x%" PLL "x, Id=0x%" PLL "x, Checkpoint=0x%" PLL "x", Block, header->id, header->checkpoint_id));
    pBlock->Release();
    delete pBlock;
    return ERR_NOFSINTEGRITY;
  }

  *ppNewBlock = pBlock;
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
#define APFS_MOD_VALUE (unsigned int)(-1)

static unsigned int mod_u64_to_0xffffffff(UINT64 Val)
{
  unsigned int r = (Val >> 32) + (Val & APFS_MOD_VALUE);
  return (r != APFS_MOD_VALUE) ? r : 0;
}

/////////////////////////////////////////////////////////////////////////////
UINT64 CApfsSuperBlock::CreateFSum(
    IN void*  pData,
    IN size_t Size
    )
{
  register unsigned int* p = reinterpret_cast<unsigned int*>(pData);
  Size /= 4;

  volatile UINT64 sum1 = 0;
  volatile UINT64 sum2 = 0;

  for(size_t i = 2; i < Size; ++i)//skip first 8 bytes for checksumm
  {
    if((sum1 += p[i]) >= APFS_MOD_VALUE)
      //normalization
      sum1 -= APFS_MOD_VALUE;
    sum2 += sum1;
  }

  sum2 = mod_u64_to_0xffffffff(sum2);
  sum1 = APFS_MOD_VALUE - mod_u64_to_0xffffffff(sum1 + sum2);

  UINT64 cs = (sum2 << 32) | sum1;
  return *(UINT64*)pData = cs;  // write new checksum
}


/////////////////////////////////////////////////////////////////////////////
bool CApfsSuperBlock::CheckFSum(
    IN void*  pData,
    IN size_t Size
    )
{
  UINT64 cs  = *(UINT64*)pData;
  UINT64 cs1 = CreateFSum(pData, Size);
  *(UINT64*)pData = cs; // restore prev value
  //assert(cs == cs1);
  return cs == cs1;
}


#ifdef UFSD_APFS_RO
/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::Flush()
{
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::UnlinkInode(
    IN CUnixInode* /*pInode*/
    )
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
bool CApfsSuperBlock::IsReadOnly(
    UINT64 /*Id*/
    ) const
{
  return true;
}

#endif

} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
