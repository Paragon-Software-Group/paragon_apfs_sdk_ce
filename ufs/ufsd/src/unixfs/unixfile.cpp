// <copyright file="unixfile.cpp" company="Paragon Software Group">
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
//     07-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#if defined UFSD_BTRFS || defined UFSD_EXTFS2 || defined UFSD_XFS || defined UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331860 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"

#include "unixfile.h"
#include "unixdir.h"

namespace UFSD
{

/////////////////////////////////////////////////////////////////////////////
CUnixFile::CUnixFile(IN CUnixFileSystem* vcb, IN bool bFork)
  : CFile(vcb->m_Mm)
  , m_pFS(vcb)
  , m_pInode(NULL)
  , m_bFork(bFork)
#ifdef UFSD_DRIVER_LINUX
  , m_aName(NULL)
#endif
{
}


/////////////////////////////////////////////////////////////////////////////
CUnixFile::~CUnixFile()
{
  //ULOG_TRACE(( GetLog(), "~CUnixFile %" PZZ "x", m_pInode->Id() ));
  if (m_pInode)
    m_pInode->Release();
#ifdef UFSD_DRIVER_LINUX
  Free2( m_aName );
#endif
}


/////////////////////////////////////////////////////////////////////////////
void CUnixFile::Destroy()
{
#ifdef UFSD_DRIVER_LINUX
  CUnixDir* p = (CUnixDir*)m_Parent;
  if ( NULL != p )
  {
    if ( 0 != p->m_Reff_count )
      p = NULL;
  }
#endif

#ifdef UFSD_CacheAlloc
  this->~CUnixFile();
  UFSD_CacheFree(m_pFS->GetFileCache(), this);
#else
  delete this;
#endif

#ifdef UFSD_DRIVER_LINUX
  if ( NULL != p) {
    p->Flush( true );
  }
#endif
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixFile::Init(
  IN UINT64       Inode,
  IN CDir*        pParent,
  IN CUnixInode*  pInode,
  IN api::STRType Type,
  IN const void*  Name,
  IN unsigned     NameLength
  )
{
  //ULOG_TRACE(( GetLog(), "CUnixFile::Init Inode = 0x%" PLL "x", Inode ));

  if (pInode)
    m_pInode = pInode;
  else
    CHECK_CALL(m_pFS->m_pSuper->GetInode(Inode, &m_pInode));
#ifdef UFSD_DRIVER_LINUX
  CHECK_CALL(reinterpret_cast<CUnixDir*>(pParent)->SetObjectName(this, pParent, Type, Name, FSObjFile, NameLength));
  m_pInode->SetNfsID(m_pFS->GetNfsID());
#else
  CHECK_CALL(CFSObject::Init(pParent, Type, Name, FSObjFile, NameLength));
#endif
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixFile::GetObjectInfo(
  OUT t_FileInfo& Info
  )
{
  CHECK_CALL(m_pInode->GetObjectInfo(&Info));

  if (m_pInode->IsInlineData())
    SetFlag(Info.Attrib, UFSD_RESIDENT);
#ifndef UFSD_DRIVER_LINUX
  Info.NameType = m_tNameType;
#endif
  Info.NameLen = m_NameLen;
  Memcpy2(Info.Name, m_aName, m_NameLen);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixFile::GetSize(
  OUT UINT64& BytesPerFile,
  OUT UINT64* ValidSize,
  OUT UINT64* AllocSize
  )
{
  BytesPerFile = m_pInode->GetSize(m_bFork);
  if (NULL != ValidSize)
    *ValidSize = BytesPerFile;
  if (NULL != AllocSize)
    *AllocSize = m_pInode->GetAllocatedSize(m_bFork);

  ULOG_TRACE((GetLog(), "CUnixFile::GetSize: Id=0x%" PLL "x, BytesPerFile = 0x%" PLL "x bytes", m_pInode->Id(), BytesPerFile));
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixFile::Read(
  IN  const UINT64& Offset,
  OUT size_t&       Bytes,
  OUT void*         pBuffer,
  IN  size_t        Size
  )
{
  ULOG_TRACE((GetLog(), "CUnixFile::Read: Id=0x%" PLL "x, Offset = %#" PLL "x, Size = %#" PZZ "x", m_pInode->Id(), Offset, Size));
  UINT64 FileSize = m_pInode->GetSize(m_bFork);

  if (Offset > FileSize)
    return ERR_NOERROR;

  if (Offset + Size > FileSize)
    Size = static_cast<size_t>(FileSize - Offset);

  Bytes = 0;
  CHECK_CALL(m_pInode->ReadWriteData(Offset, &Bytes, pBuffer, Size, false, m_bFork));
  return ERR_NOERROR;
}


//////////////////////////////////////////////////////////////////////////
int
CUnixFile::ReadSymLink(
  OUT char*       Buffer,
  IN  size_t      BufLen,
  OUT size_t*     BytesRead
  )
{
  if (Buffer == NULL || BufLen == 0)
    return ERR_BADPARAMS;

  if (!U_ISLNK(m_pInode->GetMode()))
    return ERR_BADATTRIBUTES;

  size_t Bytes = 0;
  CHECK_CALL(Read(0, Bytes, Buffer, BufLen));

  if (BytesRead)
    *BytesRead = Bytes;

  return ERR_NOERROR;
}


#if (defined UFSD_APFS && defined UFSD_APFS_RO) && !defined UFSD_BTRFS && !defined UFSD_EXTFS2 && !defined UFSD_XFS
/////////////////////////////////////////////////////////////////////////////
int CUnixFile::SetObjectInfo(
  IN const t_FileInfo&  /*Info*/,
  IN size_t             /*Flags*/
  )
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFile::SetEa(
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
int CUnixFile::Flush(
  IN bool /*bDelete*/
  )
{
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFile::SetSize(
  IN  const UINT64& /*BytesPerFile*/,
  IN  const UINT64* /*ValidSize*/,
  OUT UINT64*       /*AllocSize*/
  )
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFile::Write(
  IN  const UINT64&   /*Offset*/,
  OUT size_t&         /*Bytes*/,
  IN  const void*     /*pBuffer*/,
  IN  size_t          /*Size*/
  )
{
  return ERR_WPROTECT;
}

/////////////////////////////////////////////////////////////////////////////
int CUnixFile::GetMap(
  IN  const UINT64& /*Vbo*/,
  IN  const UINT64& /*Bytes*/,
  IN  size_t        /*Flags*/,
  OUT MapInfo*      /*Map*/
  )
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFile::Allocate(
  IN  const UINT64& /*Offset*/,
  IN  const UINT64& /*Bytes*/,
  IN  size_t        /*Flags*/,
  OUT MapInfo*      /*Map*/
  )
{
  return ERR_WPROTECT;
}

#endif  // UFSD_APFS_RO

}

#endif
