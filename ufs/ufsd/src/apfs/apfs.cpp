// <copyright file="apfs.cpp" company="Paragon Software Group">
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
//     04-October-2017  - created
//
/////////////////////////////////////////////////////////////////////////////

#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331766 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uavl.h"
#include "../h/uerrors.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixfs.h"
#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfssuper.h"
#include "apfsbplustree.h"
#include "apfs.h"
#include "dirapfs.h"

#ifdef _MSC_VER
#  pragma message ("         apfs support included")
#endif

namespace UFSD
{

namespace apfs
{

/////////////////////////////////////////////////////////////////////////////
CApfsFileSystem::CApfsFileSystem(
  IN api::IBaseMemoryManager* mm,
  IN api::IBaseStringManager* ss,
  IN api::ITime*              tt,
  IN api::IBaseLog*           log
  )
  : CUnixFileSystem(mm, ss, tt, log)
  , m_pApfsSuper(NULL)
{
}


/////////////////////////////////////////////////////////////////////////////
int CApfsFileSystem::InitFS(
  OUT size_t* Flags
  )
{
  if (m_pSuper == NULL)
  {
    CHECK_PTR(m_pApfsSuper = new(m_Mm) CApfsSuperBlock(m_Mm, GetVcbLog( this )));
    m_pSuper = m_pApfsSuper;
  }
  CHECK_CALL_SILENT(m_pApfsSuper->Init(this, Flags));

#if __WORDSIZE < 64 && defined UFSD_DRIVER_LINUX
  if (m_pApfsSuper->GetMountedVolumesCount() > 1)
  {
    UFSDTracek((m_Sb, "Only the main subvolume can be mounted on the x86 platforms"));
    m_pApfsSuper->m_MountedVolumesCount = 1;
  }
#endif

  ULOG_INFO((GetVcbLog( this ), "Volume inited as APFS (%s)", m_pSuper->IsReadOnly() ? "ro" : "rw"));

  if (m_pApfsSuper->GetMountedVolumesCount() == 0)
    return ERR_NOROOTDIR;

  return OpenRoot<CApfsDir>(APFS_ROOT_INO);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsFileSystem::FindCSBBlock(const apfs_sb* pMSB, UINT64& Block) const
{
  //Find first checkpoint superblock which pointed from main superblock
  UINT64 CSBNumber = pMSB->sb_first_sb + pMSB->sb_current_sb + pMSB->sb_current_sb_len - 1;

  if ( CSBNumber < pMSB->sb_first_sb )
    CSBNumber = pMSB->sb_first_sb;

  //Find last checkpoint superblock
  apfs_sb* pCSB;
  CHECK_PTR(pCSB = reinterpret_cast<apfs_sb*>(Malloc2(pMSB->sb_block_size)));

  Block = 0;

  UINT64 CurCheckpointID = 0;

  for (;;)
  {
    for (;; ++CSBNumber)
    {
      if (CSBNumber >= pMSB->sb_first_sb + pMSB->sb_number_of_sb)
        //SB area loop -> first entry
        CSBNumber = pMSB->sb_first_sb;

      int Status = m_Rw->ReadBytes(CSBNumber * pMSB->sb_block_size, pCSB, pMSB->sb_block_size);
      if (!UFSD_SUCCESS(Status))
      {
        Free2(pCSB);
        return Status;
      }

      BE_ONLY(ConvertSB(pCSB));
      if (pCSB->header.block_type == APFS_TYPE_SUPERBLOCK)
        break;

      if (pCSB->header.block_type == APFS_TYPE_EMPTY)
        break;

      if (pCSB->header.block_type != APFS_TYPE_SUPERBLOCK_MAP)
      {
        assert(0);
        Free2(pCSB);
        UFSDTracek((m_Sb, "No valid checkpoint"));
        return ERR_NOFSINTEGRITY;
      }
    }

    if (pCSB->header.block_type == APFS_TYPE_EMPTY)
      break;

    if (CurCheckpointID != 0 && pCSB->header.checkpoint_id <= CurCheckpointID)
      break;  // got it!

    //save current SB
    Block = CSBNumber;
    CurCheckpointID  = pCSB->header.checkpoint_id;
    //next SB
    CSBNumber = pMSB->sb_first_sb + pCSB->sb_next_sb + 1;
  }

  Free2(pCSB);

  if (Block == 0)
  {
    assert(0);
    UFSDTracek((m_Sb, "No valid checkpoint"));
    return ERR_NOFSINTEGRITY;
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
char* CApfsFileSystem::GetVolumeName(OUT size_t* Bytes) const
{
  if (m_pApfsSuper->GetMountedVolumesCount() == 0)
  {
    if (Bytes)
      *Bytes = 0;
    return NULL;
  }
  char* VolName = m_pApfsSuper->GetVolume(0)->GetName();
  if (Bytes)
    *Bytes = m_Strings->strlen(api::StrUTF8, VolName);
  return VolName;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsFileSystem::Init(
    IN  api::IDeviceRWBlock** RwBlock,
    IN  unsigned int          RwBlockCount,
    IN  size_t                Options,
    OUT size_t*               Flags,
    IN const PreInitParams*   Params
    )
{
  if (Params != NULL)
  {
    if(Params->FsType != FS_APFS)
      return ERR_BADPARAMS;

    Memcpy2( &m_Params, Params, sizeof(PreInitParams) );
  }

  return CUnixFileSystem::Init( RwBlock, RwBlockCount, Options, Flags, Params );
}

int CApfsFileSystem::ReInit(
    IN size_t     Options,
    OUT size_t*   Flags,
    IN const PreInitParams* Params
    )
{
  if (Params != NULL)
  {
    if(Params->FsType != FS_APFS)
      return ERR_BADPARAMS;

    Memcpy2( &m_Params, Params, sizeof(PreInitParams) );
  }

  return CUnixFileSystem::ReInit( Options, Flags, Params );
}

int CApfsFileSystem::OnGetApfsInfo()
{
    size_t BytesReturned = 0;
    struct UFSD_VOLUME_APFS_INFO *info = (struct UFSD_VOLUME_APFS_INFO *) m_IO.OutBuffer;
    for (unsigned int Idx = 0; Idx < m_pApfsSuper->GetMountedVolumesCount(); Idx++) {
        if (BytesReturned + sizeof(struct UFSD_VOLUME_APFS_INFO) > m_IO.OutBufferSize)
        {
            if (m_IO.BytesReturned)
                *m_IO.BytesReturned = BytesReturned;
            return Idx ? ERR_MORE_DATA : ERR_INSUFFICIENT_BUFFER;
        }
        CApfsVolumeSb *vol = m_pApfsSuper->GetVolume(Idx);
        apfs_vsb *volSb = vol->GetVolumeSb();
        info[Idx] = {
            .BlocksUsed = volSb->vsb_blocks_used,
            .BlocksReserved = volSb->vsb_blocks_reserved,
            .FilesCount = volSb->vsb_files_count,
            .DirsCount = volSb->vsb_dirs_count,
            .SymlinksCount = volSb->vsb_symlinks_count,
            .SpecFilesCount = volSb->vsb_spec_files_count,
            .SnapshotsCount = volSb->vsb_snapshots_count,
            .Index = Idx,
            .Encrypted = vol->IsEncrypted(),
            .CaseSensitive = vol->IsCaseSensitive()
        };
        Memcpy2(info[Idx].SerialNumber, volSb->vsb_uuid, 0x10);
        Memcpy2(info[Idx].VolumeName, volSb->vsb_volname, 0x100);
        info[Idx].Creator[0] = '\0';
        BytesReturned += sizeof(struct UFSD_VOLUME_APFS_INFO);
    }
    if (m_IO.BytesReturned)
        *m_IO.BytesReturned = BytesReturned;
    return ERR_NOERROR;
}

#ifdef UFSD_APFS_RO
/////////////////////////////////////////////////////////////////////////////
int CApfsFileSystem::SetVolumeInfo(
  IN const void*     /*VolumeSerial*/,
  IN size_t          /*BytesPerVolumeSerial*/,
  IN api::STRType    /*VolumeNameType*/,
  IN const void*     /*VolumeNameBuffer*/,
  IN VolumeState     /*NewState*/
  )
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsFileSystem::OpenByID(
  IN  UINT64      /*ID*/,
  OUT CFSObject** /*Fso*/,
  IN  int         /*fFollowSymlink*/
  )
{
  return ERR_NOTIMPLEMENTED;
}

#endif // UFSD_APFS_RO

}  //namespace apfs


#if defined UFSD_APFS_RO && defined UFSD_APFS_GET_VOLUME_META_BITMAP
//////////////////////////////////////////////////////////////////////////
int ApfsGetVolumeMetaBitmap(
  IN  api::IBaseMemoryManager* /*m_Mm*/,
  IN  api::IDeviceRWBlock*     /*Rw*/,
  IN  api::IMessage*           /*Msg*/,
  IN  size_t                   /*FsDumpOpt*/,
  OUT unsigned char**          /*Bitmap*/,
  OUT UINT64*                  /*ClustersPerVolume*/,
  OUT unsigned int*            /*Log2OfCluster*/,
  OUT unsigned int*            /*Errors*/
)
{
  return ERR_NOTIMPLEMENTED;
}
#endif


//////////////////////////////////////////////////////////////////////////
CFileSystem* CreateFsApfs(
  IN api::IBaseMemoryManager* mm,
  IN api::IBaseStringManager* ss,
  IN api::ITime*              tt,
  IN api::IBaseLog*           log
)
{
  return new(mm) apfs::CApfsFileSystem(mm, ss, tt, log);
}


} // namespace UFSD


#endif
