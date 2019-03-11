// <copyright file="unixdir.cpp" company="Paragon Software Group">
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
static const char s_pFileName [] = __FILE__ ",$Revision: 331860 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"

#include "unixinode.h"
#include "unixfs.h"
#include "unixdir.h"
#include "unixfile.h"


namespace UFSD {

#ifdef UFSD_DRIVER_LINUX
static char* strchr(const char* s, int c)
{
    for (; *s != (char)c; ++s)
        if (*s == '\0')
            return NULL;
    return (char*)s;
}
#endif

/////////////////////////////////////////////////////////////////////////////
CUnixDir::CUnixDir(CUnixFileSystem* vcb)
  : CDir(vcb->m_Mm)
  , m_pFS(vcb)
  , m_pInode(NULL)
  , m_pEnum(NULL)
#ifdef UFSD_DRIVER_LINUX
  , m_aName(NULL)
#endif
{
}


/////////////////////////////////////////////////////////////////////////////
CUnixDir::~CUnixDir()
{
  //ULOG_TRACE(( GetLog(), "~CUnixDir r=%" PLL "x, RefCount=%x", m_pInode->Id(), m_Reff_count ));
#ifndef UFSD_DRIVER_LINUX
  CloseAll();
#endif

  if (m_pInode)
    m_pInode->Release();

  if (m_pEnum)
    m_pEnum->Destroy();
#ifdef UFSD_DRIVER_LINUX
  Free2( m_aName );
#endif
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::Init(
  IN UINT64           Inode,
  IN CDir*            pParent,
  IN CUnixInode*      pInode,
  IN api::STRType     Type,
  IN const void*      Name,
  IN unsigned int     NameLength
  )
{
  if (pInode)
    m_pInode = pInode;
  else
    CHECK_CALL(m_pFS->m_pSuper->GetInode(Inode, &m_pInode));
  CHECK_CALL(InitEnumerator(0, &m_pEnum));

#ifdef UFSD_DRIVER_LINUX
  CHECK_CALL(SetObjectName(this, pParent, Type, Name, FSObjDir, NameLength));
  m_pInode->SetNfsID(m_pFS->GetNfsID());
#else
  CHECK_CALL(CFSObject::Init(pParent, Type, Name, FSObjDir, NameLength));
#endif

#ifdef UFSD_CacheAlloc
  m_DirCache = m_pFS->GetDirCache();
#endif

  return ERR_NOERROR;
}


#ifndef UFSD_DRIVER_LINUX
/////////////////////////////////////////////////////////////////////////////
void CUnixDir::CloseAll()
{
  list_head* pos, *next;

  list_for_each_safe(pos, next, &m_OpenDirs)
  {
    CUnixDir* pDir = list_entry(pos, CUnixDir, m_Entry);
    pos->remove();
    pos->init();

    pDir->Flush(true);
  }

  list_for_each_safe(pos, next, &m_OpenFiles)
  {
    CUnixFile* pFile = list_entry(pos, CUnixFile, m_Entry);
    pos->remove();
    pos->init();

    pFile->Flush(true);
  }

  assert(m_OpenDirs.is_empty());
  assert(m_OpenFiles.is_empty());
}
#endif


/////////////////////////////////////////////////////////////////////////////
void CUnixDir::CreateFsObject(
  IN  const FileInfo* /*Info*/,
  OUT CUnixFile**     Fso
  ) const
{
#ifdef UFSD_CacheAlloc
  void* p = UFSD_CacheAlloc(m_pFS->GetFileCache(), true);
  if (p)
    *Fso = new(p) CUnixFile(m_pFS);
#else
  *Fso = new(m_Mm) CUnixFile(m_pFS);
#endif
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::StartFind(
  IN CEntryNumerator*&  dentry_num,
  IN size_t             options
  )
{
  CUnixEnumerator* Enum;
  int Status = InitEnumerator(options, &Enum);

  if (!UFSD_SUCCESS(Status))
    delete Enum;
  else
    dentry_num = Enum;

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixDir::Open(
  IN  api::STRType    type,
  IN  const void*     name,
  IN  size_t          name_len,
  OUT CFSObject**     Fso,
  OUT FileInfo*       fi
)
{
  if (name_len == 0 || name_len > m_pFS->GetMaxFileNameLen())
    return ERR_BADNAME_LEN;

  ULOG_TRACE((GetLog(), "CUnixDir::Open '%s'", (type == api::StrUTF8) ? (char*)name : m_pFS->U2A((unsigned short*)name, name_len)));
#ifdef UFSD_DRIVER_LINUX
  assert(type == api::StrUTF16);
#endif

  CUnixInode* pInode = NULL;
  FileInfo info;
  CHECK_CALL_SILENT(FindDirEntry(type, name, name_len, &info, &pInode));

  if (FlagOn(info.Attrib, UFSD_SUBDIR))
    CHECK_CALL(OpenEx(info, m_OpenDirs, pInode, (CUnixDir**)Fso));
  else
    CHECK_CALL(OpenEx(info, m_OpenFiles, pInode, (CUnixFile**)Fso));

  if (fi)
    *fi = info;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::GetObjectInfo(
  OUT t_FileInfo&  file_info
)
{
  CHECK_CALL(m_pInode->GetObjectInfo(&file_info));
#ifndef UFSD_DRIVER_LINUX
  file_info.NameType = m_tNameType;
#endif
  file_info.NameLen = m_NameLen;
  Memcpy2(file_info.Name, m_aName, m_NameLen);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
CUnixDir* CUnixDir::CloseParent()
{
#ifdef UFSD_DRIVER_LINUX
  CUnixDir* p = reinterpret_cast<CUnixDir*>(m_Parent);
  if (NULL != p && 0 != p->m_Reff_count)
    p = NULL;
#endif

  DestroyInternal();

#ifdef UFSD_DRIVER_LINUX
  if (NULL != p)
    p->Flush(false);
  return p;
#else
  return NULL;
#endif
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::GetFileInfo(
  IN  api::STRType    type,
  IN  const void*     name,
  IN  size_t          name_len,
  OUT FileInfo&       fi
  )
{
  return FindDirEntry(type, name, name_len, &fi, NULL);
}


/////////////////////////////////////////////////////////////////////////////
#ifdef UFSD_DRIVER_LINUX
int
CUnixDir::CloseFile(
    IN CFile* file
    )
{
  if ( NULL == file )
    return ERR_BADPARAMS;
#ifndef UFSD_DRIVER_LINUX
  assert( 0 == file->GetReffCount() );
#endif
  return file->Flush( true );
}
#endif


//////////////////////////////////////////////////////////////////////////
int
CUnixDir::OpenPath(
  IN  char*       path,
  IN  FileInfo*   Info,
  OUT CUnixDir**  StartDir,
  OUT CUnixDir**  FoundDir
  )
{
  if (FoundDir == NULL || StartDir == NULL)
    return ERR_BADPARAMS;

  CUnixDir *CurrentDir;
  *StartDir = NULL;

  if (*path == '/')
  {
    CurrentDir = reinterpret_cast<CUnixDir*>(m_pFS->m_RootDir);
    path++;
  }
  else
    CurrentDir = this;

  bool firstcycle = true;
#ifndef UFSD_DRIVER_LINUX
  char* slash = (char*)m_pFS->m_Strings->strchr(api::StrUTF8, path, '/');
#else
  char* slash = strchr(path, '/');
#endif

  CDir* d;
  while (slash != NULL)
  {
    CHECK_CALL(CurrentDir->OpenDir(api::StrUTF8, path, PtrOffset(path, slash), d));
    CurrentDir = reinterpret_cast<CUnixDir*>(d);
    path = ++slash;
#ifndef UFSD_DRIVER_LINUX
    slash = (char*)m_pFS->m_Strings->strchr(api::StrUTF8, path, '/');
#else
    slash = strchr(path, '/');
#endif

    if (firstcycle)
    {
      *StartDir = CurrentDir;
      firstcycle = false;
    }
  }

  CHECK_CALL( CurrentDir->FindDirEntry(api::StrUTF8, path, m_pFS->m_Strings->strlen(api::StrUTF8, path), Info, NULL) );
  *FoundDir = CurrentDir;

  return ERR_NOERROR;
}


//////////////////////////////////////////////////////////////////////////
int
CUnixDir::FollowLink(
  IN FileInfo* Info
  )
{
  int     Status = ERR_NOERROR;
  size_t  blocksize = m_pFS->GetBlockSize();
  char*   path = reinterpret_cast<char*>(Malloc2(blocksize));
  size_t  count = 0;
  CUnixDir  *pStartDir = NULL, *pDir = this;

  CHECK_PTR(path);

  while (U_ISLNK(Info->Mode) && UFSD_SUCCESS(Status))
  {
    if (count++ > 5)
    {
      Status = ERR_OPENFILE;
      break;
    }

    size_t    bytes;
#ifndef UFSD_DRIVER_LINUX
    Status = pDir->GetSymLink(Info->NameType, Info->Name, Info->NameLen, path, blocksize, &bytes);
#else
    Status = pDir->GetSymLink(api::StrUTF16, Info->Name, Info->NameLen, path, blocksize, &bytes);
#endif
    if (pStartDir != NULL)
      pStartDir->Destroy();

    if (!UFSD_SUCCESS(Status))
      break;

    path[bytes] = 0;
    Status = OpenPath(path, Info, &pStartDir, &pDir);
  }

  if (pStartDir != NULL)
    pStartDir->Destroy();

  Free2(path);

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixDir::GetFSObjID(
  IN  api::STRType    type,
  IN  const void*     name,
  IN  size_t          name_len,
  OUT UINT64&         ID,
  IN  int             fFollowSymlink
  )
{
  FileInfo fi;

  CHECK_CALL(FindDirEntry(type, name, name_len, &fi, NULL));

  if (fFollowSymlink && U_ISLNK(fi.Mode))
    CHECK_CALL(FollowLink(&fi));

  ID = fi.Id;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixDir::GetSymLink(
  IN  api::STRType    type,
  IN  const void*     name,
  IN  size_t          name_len,
  OUT char*           buffer,
  IN  size_t          buflen,
  OUT size_t*         read
  )
{
  CFile*  pFile;

  CHECK_CALL(OpenFile(type, name, name_len, pFile));
  CHECK_CALL(pFile->ReadSymLink(buffer, buflen, read));

  return pFile->Flush(true);
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixDir::CheckSymLink(
  IN  api::STRType    type,
  IN  const void*     name,
  IN  size_t          name_len
  )
{
  FileInfo  Info;

  CHECK_CALL(FindDirEntry(type, name, name_len, &Info, NULL));

  if (!U_ISLNK(Info.Mode))
    return ERR_OPENFILE;

  return FollowLink(&Info);
}


/////////////////////////////////////////////////////////////////////////////
bool CUnixDir::IsDot(
  IN const FileInfo& fi
  )
{
  if (fi.NameLen == 1)
  {
#ifndef UFSD_DRIVER_LINUX
    unsigned int cs = api::IBaseStringManager::GetCharSize(fi.NameType);
#else
    unsigned int cs = sizeof(unsigned short);
#endif
    if (cs == sizeof(char))
      return (*(char*)fi.Name == '.');
    if (cs == sizeof(unsigned short))
      return (fi.Name[0] == '.');

    assert(!"Unknown STRType");
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////
bool CUnixDir::IsDotDot(
  IN const FileInfo& fi
  )
{
  if (fi.NameLen == 2)
  {
#ifndef UFSD_DRIVER_LINUX
    unsigned int cs = api::IBaseStringManager::GetCharSize(fi.NameType);
#else
    unsigned int cs = sizeof(unsigned short);
#endif
    if (cs == sizeof(char))
      return (*(char*)fi.Name == '.' && ((char*)fi.Name)[1] == '.');
    if (cs == sizeof(unsigned short))
      return (fi.Name[0] == '.' && fi.Name[1] == '.');

    assert(!"Unknown STRType");
  }

  return false;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixDir::Flush(
  IN bool bDelete
  )
{
  int Status = m_pInode->Flush();

  if (bDelete)
    Destroy();

  return Status;
}


#if (defined UFSD_APFS && defined UFSD_APFS_RO) && !defined UFSD_BTRFS && !defined UFSD_EXTFS2 && !defined UFSD_XFS
/////////////////////////////////////////////////////////////////////////////
int CUnixDir::CreateDir(
  IN  api::STRType    /*type*/,
  IN  const void *    /*name*/,
  IN  size_t          /*name_len*/,
  OUT CDir**          /*dir*/
)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::CreateFile(
  IN  api::STRType    /*type*/,
  IN  const void *    /*name*/,
  IN  size_t          /*name_len*/,
  OUT CFile**         /*file*/
)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::Link(
  IN CFSObject*       /*Fso*/,
  IN api::STRType     /*type*/,
  IN const void *     /*name*/,
  IN size_t           /*name_len*/
)
{
  return ERR_WPROTECT;
}

/////////////////////////////////////////////////////////////////////////////
int CUnixDir::SetEa(
  IN  const char* /*Name*/,
  IN  size_t      /*NameLen*/,
  IN  const void* /*Value*/,
  IN  size_t      /*BytesPerValue*/,
  IN  size_t      /*Flags*/
)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::SetObjectInfo(
  IN const t_FileInfo&  /*file_info*/,
  IN size_t             /*Flags*/
)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::SetFileInfo(
  IN api::STRType     /*type*/,
  IN const void *     /*name*/,
  IN size_t           /*name_len*/,
  IN const FileInfo&  /*fi*/,
  IN size_t           /*flags*/
)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::CreateSymLink(
  IN  api::STRType    /*type*/,
  IN  const void*     /*name*/,
  IN  size_t          /*name_len*/,
  IN  const char*     /*symlink*/,
  IN  size_t          /*sym_len*/,
  OUT UINT64*         /*pId*/
)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixDir::Unlink(
  IN api::STRType     ,
  IN const void *     ,
  IN size_t           ,
  IN CFSObject*
  )
{
  return ERR_WPROTECT;
}

int CUnixDir::CheckForEmpty(CUnixInode*) { return ERR_NOTIMPLEMENTED; }
#endif


} //namespace UFSD

#endif
