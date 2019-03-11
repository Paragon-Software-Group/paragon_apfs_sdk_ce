// <copyright file="dirapfs.cpp" company="Paragon Software Group">
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
//     05-October-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331860 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixdir.h"
#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfsvolsb.h"
#include "apfssuper.h"
#include "apfstable.h"
#include "apfsbplustree.h"
#include "dirapfs.h"
#include "apfs.h"
#include "apfsinode.h"
#include "apfsenum.h"

namespace UFSD
{

namespace apfs
{

/////////////////////////////////////////////////////////////////////////////
CApfsDir::CApfsDir(CUnixFileSystem* fs)
    : CUnixDir(fs)
    , m_pEntryEnum(NULL)
{}


/////////////////////////////////////////////////////////////////////////////
CApfsDir::~CApfsDir()
{
  delete m_pEntryEnum;
}


/////////////////////////////////////////////////////////////////////////////
UINT64 CApfsDir::GetObjectID() const
{
  return reinterpret_cast<CApfsInode*>(m_pInode)->GetExternalId();
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::CheckForEmpty(
    IN CUnixInode* pInode
    )
{
  assert(U_ISDIR(pInode->GetMode()));

  CApfsInode* aInode = reinterpret_cast<CApfsInode*>(pInode);
  CSimpleSearchKey Key(m_Mm, aInode->GetInodeId(), ApfsDirEntry);
  int Status = aInode->GetTree()->GetData(&Key, SEARCH_KEY_EQ, NULL, NULL);

  if (Status == ERR_NOERROR)
    return ERR_DIRNOTEMPTY;
  if (Status == ERR_NOTFOUND)
    return ERR_NOERROR;
  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::InitEnumerator(
    IN  size_t            Options,
    OUT CUnixEnumerator** pEnum,
    IN  CUnixInode*       pInode
    ) const
{
  CHECK_PTR(*pEnum = new(m_Mm) CApfsEntryNumerator(m_Mm));

  if (FlagOn(m_pFS->m_Options, UFSD_OPTIONS_MOUNT_ALL_VOLUMES))
    Options |= APFS_ENUM_ALL_VOLUMES;

  if (pInode == NULL)
    pInode = m_pInode;

  return (*pEnum)->Init(m_pFS, Options, pInode);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::FindNext(
    IN CEntryNumerator* pEnum,
    OUT FileInfo&       Info,
    OUT CUnixInode**    pInode
    )
{
  CApfsEntryNumerator* e = reinterpret_cast<CApfsEntryNumerator*>(pEnum);
  int Status = ERR_NOERROR;

  Memzero2(&Info, sizeof(FileInfo));

  if (e->m_bMatchAll)
  {
    if (!m_pEntryEnum)
    {
      CApfsTree* pRootTree = reinterpret_cast<CApfsInode*>(m_pInode)->GetTree();

      if (pRootTree)
      {
        CHECK_PTR(m_pEntryEnum = new(m_Mm) CEntryTreeEnum(reinterpret_cast<CApfsSuperBlock*>(m_pFS->m_pSuper), reinterpret_cast<CApfsInode*>(m_pInode)->GetInodeId()));
        CHECK_CALL(m_pEntryEnum->Init(pRootTree));
        CHECK_CALL(m_pEntryEnum->StartEnum());
      }
    }

    if (m_pEntryEnum)
      CHECK_CALL(m_pEntryEnum->SetPosition(e->m_Position2));
  }

  while (UFSD_SUCCESS(Status))
  {
    Status = e->FindEntry(Info, e->m_bMatchAll ? m_pEntryEnum : NULL);

    if (Status == ERR_FILEEXISTS)
    {
      Status = ERR_NOERROR;

      if (!FlagOn(e->m_Options, UFSD_ENUM_NAME_ID_ATTR_ONLY))
      {
        CUnixInode* i;

        if (pInode == NULL)
          pInode = &i;

        CHECK_CALL_EXIT(m_pFS->m_pSuper->GetInode(Info.Id, pInode));

        Status = (*pInode)->GetObjectInfo(&Info, false);

        if (pInode == &i)
          (*pInode)->Release();
      }

      break;
    }
  }

Exit:
  if (e->m_bMatchAll && !UFSD_SUCCESS(Status))
    e->SetPosition(EOD_POS);

  if (Status == ERR_NOTFOUND) // error while search in the tree
    return ERR_NOFILEEXISTS;

  if (!UFSD_SUCCESS(Status))
  {
    delete m_pEntryEnum;
    m_pEntryEnum = NULL;
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::FindDirEntry(
    IN  api::STRType  Type,
    IN  const void*   Name,
    IN  size_t        NameLen,
    OUT FileInfo*     pInfo,
    OUT CUnixInode**  pInode
    )
{
  size_t EnumOptions = FlagOn(m_pFS->m_Options, UFSD_OPTIONS_MOUNT_ALL_VOLUMES) ? APFS_ENUM_ALL_VOLUMES : 0;
  CHECK_CALL_SILENT(m_pEnum->Init(m_pFS, EnumOptions, m_pInode, Type, Name, NameLen));
  return FindNext(m_pEnum, *pInfo, pInode);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::GetSubDirAndFilesCount(
    OUT bool*   IsEmpty,
    OUT size_t* SubDirsCount,
    OUT size_t* FilesCount
  )
{
  if ( FilesCount )
    *FilesCount= 0;

  CApfsInode* aInode = reinterpret_cast<CApfsInode*>(m_pInode);

  if ( SubDirsCount )
    *SubDirsCount = aInode->GetInode()->ai_nchildren;

  if (IsEmpty)
    *IsEmpty = (0 == aInode->GetInode()->ai_nchildren);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
void CApfsDir::CreateFsObject(
    IN  const FileInfo* pInfo,
    OUT CUnixDir**      pDir
    ) const
{
  if (pInfo->Id != APFS_VOLUMES_DIR_ID)
  {
#ifdef UFSD_CacheAlloc
    void* p = UFSD_CacheAlloc(g_ApfsDirCache, true);
    *pDir = new(p) CApfsDir(m_pFS);
#else
    *pDir = new(m_Mm) CApfsDir(m_pFS);
#endif
  }
  else
    *pDir = new(m_Mm) CApfsVolumeDir(m_pFS);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsVolumeDir::Init(
    IN UINT64        /*Inode*/,
    IN CDir*         pParent,
    IN CUnixInode*   pInode,
    IN api::STRType  type,
    IN const void*   Name,
    IN unsigned      NameLength
    )
{
  if (pInode)
    m_pInode = pInode;

  CHECK_CALL(InitEnumerator(0, &m_pEnum, pInode));

#ifdef UFSD_DRIVER_LINUX
  CHECK_CALL(SetVolumeDirName(pParent, type, Name, NameLength));
  m_pInode->SetNfsID(m_pFS->GetNfsID());
#else
  CHECK_CALL(CFSObject::Init(pParent, type, Name, FSObjDir, NameLength));
#endif
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsVolumeDir::GetSubDirAndFilesCount(
    OUT bool*   IsEmpty,
    OUT size_t* SubDirsCount,
    OUT size_t* FilesCount
    )
{
  //No files in this dir
  if (FilesCount)
    *FilesCount = 0;

  //Volumes are displayed as dir
  unsigned char NumberOfSubVolumes = reinterpret_cast<CApfsSuperBlock*>(m_pFS->m_pSuper)->GetMountedVolumesCount();
  assert(NumberOfSubVolumes > 1);

  if (SubDirsCount)
    *SubDirsCount = NumberOfSubVolumes - 1;

  //If this dir is emulated it is not empty
  if (IsEmpty)
    *IsEmpty = false;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsVolumeDir::InitEnumerator(
    IN  size_t            Options,
    OUT CUnixEnumerator** pEnum,
    IN  CUnixInode*       pInode
    ) const
{
  CHECK_PTR(*pEnum = new(m_Mm) CApfsVolumeNumerator(m_Mm));
  if (pInode == NULL)
    pInode = m_pInode;

  return (*pEnum)->Init(m_pFS, Options, pInode);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsVolumeDir::GetObjectInfo(
    OUT t_FileInfo& Info
    )
{
  reinterpret_cast<CApfsEntryNumerator*>(m_pEnum)->EmulateVolumesDirEntry(Info);
  return ERR_NOERROR;
}


#ifdef UFSD_APFS_RO
/////////////////////////////////////////////////////////////////////////////
bool CApfsDir::IsReadOnly()
{
  return true;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::Create(
    IN  api::STRType    /*Type*/,
    IN  const void*     /*Name*/,
    IN  size_t          /*NameLen*/,
    IN  unsigned short  /*Mode*/,
    IN  unsigned int    /*Uid*/,
    IN  unsigned int    /*Gid*/,
    IN  const void*     /*Data*/,
    IN  size_t          /*DataLen*/,
    OUT CFSObject**     /*Fso*/
    )
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::UnlinkInternal(
    IN CUnixInode* /*pInode*/,
    IN CFSObject*  /*Fso*/,
    IN FileInfo    /*Info*/
    ) const
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsDir::Rename(
    IN api::STRType /*Type*/,
    IN const void*  /*SrcName*/,
    IN size_t       /*SrcLen*/,
    IN CFSObject*   /*Src*/,
    IN CDir*        /*TgtDir*/,
    IN const void*  /*DstName*/,
    IN size_t       /*DstLen*/
    )
{
  return ERR_WPROTECT;
}
#endif // UFSD_APFS_RO


} // namespace apfs

} // namespace UFSD

#endif
