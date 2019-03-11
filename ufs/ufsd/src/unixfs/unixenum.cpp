// <copyright file="unixenum.cpp" company="Paragon Software Group">
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
//     11-September-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#if defined UFSD_BTRFS || defined UFSD_EXTFS2 || defined UFSD_XFS || defined UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331860 $";
#endif


#include <ufsd.h>

#include "../h/assert.h"
#include "../h/utrace.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"

#include "unixfs.h"
#include "unixenum.h"

namespace UFSD
{

/////////////////////////////////////////////////////////////////////////////
CUnixEnumerator::CUnixEnumerator(IN api::IBaseMemoryManager* Mm)
  : CEntryNumerator(Mm)
  , m_Strings(NULL)
  , m_pSuper(NULL)
  , m_Mask(NULL)
  , m_Len(0)
  , m_Hash(0)
  , m_bMatchAll(true)
  , m_bCaseSensitive(true)
  , m_Position2(0)
  , m_Options(0)
  , m_pInode(NULL)
  , m_bDirty(false)
  , m_bRestarted(true)
  , m_BlockBuff(NULL)
  , m_BlockSize(0)
  , m_CurBlock(0)
  , m_Status(ERR_NOERROR)
{
}


/////////////////////////////////////////////////////////////////////////////
CUnixEnumerator::~CUnixEnumerator()
{
  Free2(m_Mask);
  Free2(m_BlockBuff);
}


/////////////////////////////////////////////////////////////////////////////
int CUnixEnumerator::Init(
  IN CUnixFileSystem*    Fs,
  IN size_t              Options,
  IN CUnixInode*         pInode,
  IN api::STRType        Type,
  IN const void*         Mask,
  IN size_t              Len
  )
{
  char name[MAX_FILENAME];
  size_t len = 0;

  m_pSuper = Fs->m_pSuper;
  m_Strings = m_pSuper->m_Strings;

  if (Mask)
  {
    if (!IsNameValid(Type, Mask, Len))
    {
      ULOG_ERROR((GetLog(), ERR_BADNAME, "Invalid name"));
      ULOG_DUMP((GetLog(), LOG_LEVEL_ERROR, Mask, Len * m_Strings->GetCharSize(Type)));
      return ERR_BADNAME;
    }

    if (m_Strings && Type != api::StrUTF8)
      CHECK_CALL(m_Strings->Convert(Type, Mask, Len, api::StrUTF8, name, MAX_FILENAME, &len));
    else
    {
      if (Len <= MAX_FILENAME)
        Memcpy2(name, Mask, Len);
      len = Len;
    }

    if (len == 0 || len > Fs->GetMaxFileNameLen())
      return ERR_BADNAME_LEN;

    CHECK_CALL(SetMask(name, static_cast<unsigned char>(len)));
  }

  m_Position2 = 0;

  m_Options = Options;
  m_Status = ERR_NOERROR;
  m_BlockSize = m_pSuper->GetBlockSize();

  assert(m_pInode == NULL || m_pInode->Id() == pInode->Id());
  m_pInode = pInode;

  if (m_BlockBuff == NULL)
    CHECK_PTR(m_BlockBuff = Malloc2(m_BlockSize));

  if (!m_bMatchAll)
    m_Hash = GetNameHash(reinterpret_cast<const unsigned char*>(m_Mask), m_Len);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
void
CUnixEnumerator::SetPosition(const UINT64& Position)
{
  if (m_pInode->m_bHaveChanges)
  {
    m_pInode->m_bHaveChanges = false;
    ULOG_TRACE((GetLog(), "Directory have changes, force enumeration from start"));
  }
  else
  {
    m_Position = Position;
    m_bRestarted = false;
  }
}


/////////////////////////////////////////////////////////////////////////////
int CUnixEnumerator::SetMask(
  IN const void*   Name,
  IN unsigned char Len
  )
{
  if (Name == NULL)
    m_bMatchAll = true;
  else
  {
    m_bMatchAll = false;

    if (m_Mask == NULL || Len > m_Len)
    {
      Free2(m_Mask);
      CHECK_PTR(m_Mask = reinterpret_cast<char*>(Malloc2(Len)));
    }

    Memcpy2(m_Mask, Name, Len);
    m_Len = Len;
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
#ifdef UFSD_DRIVER_LINUX
bool CUnixEnumerator::EmulateDots(CDir* pParent, FileInfo& Info)
{
#ifndef UFSD_EMULATE_DOTS
  if (!m_bMatchAll || m_Position > 1)
    return false;

  if (m_Position == 0)
  {
    // emulate dot
    Info.Id      = m_pInode->Id();
    Info.Attrib  = UFSD_SUBDIR;
    Info.NameLen = 1;
    Info.Name[0] = '.';
    m_Position   =
    m_Position2  = 1;
  }
  else
  {
    // emulate dot-dot
    if (pParent)
      Info.Id    = pParent->GetObjectID();
    else
      Info.Id    = m_pInode->Id();
    Info.Attrib  = UFSD_SUBDIR;
    Info.NameLen = 2;
    Info.Name[0] =
    Info.Name[1] = '.';
    m_Position   =
    m_Position2  = 2;
  }

  return true;
#endif
  return false;
}
#endif


/////////////////////////////////////////////////////////////////////////////
int CUnixEnumerator::ReadWriteCurBlock(
  IN bool bWrite
  )
{
  if (bWrite)
    m_bDirty = true;

  return m_pInode->ReadWriteData(m_CurBlock << m_pSuper->m_Log2OfCluster, NULL, m_BlockBuff, m_BlockSize, bWrite);
}


//////////////////////////////////////////////////////////////////////////
int CUnixEnumerator::ReadWriteDirBlock(
  IN OUT void*  pBuffer,
  IN UINT64     Block,
  IN bool       bWrite
  )
{
  if (bWrite)
    m_bDirty = true;

  return m_pInode->ReadWriteData(Block << m_pSuper->m_Log2OfCluster, NULL, pBuffer, m_BlockSize, bWrite);
}


} //namespace UFSD

#endif
