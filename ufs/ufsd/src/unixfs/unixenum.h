// <copyright file="unixenum.h" company="Paragon Software Group">
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


#ifndef __UNIXENUM_H
#define __UNIXENUM_H

#include "unixsuperblock.h"

namespace UFSD
{

struct CUnixEnumerator : CEntryNumerator
{
  api::IBaseStringManager*    m_Strings;

  CUnixSuperBlock*    m_pSuper;

  char*               m_Mask;
  unsigned char       m_Len;
  unsigned int        m_Hash;
  bool                m_bMatchAll;
  bool                m_bCaseSensitive;
  UINT64              m_Position2;        // for internal use only
  size_t              m_Options;
  CUnixInode*         m_pInode;
  bool                m_bDirty;
  bool                m_bRestarted;       //Is enumerator restarted (new enumerator created)

  void*               m_BlockBuff;
  unsigned int        m_BlockSize;
  size_t              m_CurBlock;

  int                 m_Status;

  CUnixEnumerator(IN api::IBaseMemoryManager* Mm);
  virtual ~CUnixEnumerator();

  virtual int Init(
    IN CUnixFileSystem*    Fs,
    IN size_t              Options,
    IN CUnixInode*         pInode = NULL,
    IN api::STRType        Type = api::StrUTF8,
    IN const void*         Mask = NULL,
    IN size_t              Len = 0
  );

  virtual void Destroy() { delete this; }

  virtual void SetPosition(const UINT64& Position);

  virtual unsigned int GetNameHash(const unsigned char* Name, size_t NameLen) const = 0;

  virtual int SetNewEntry(
    IN UINT64         Id,
    IN const char*    Name,
    IN unsigned char  Len,
    IN unsigned char  Type
    ) = 0;

  int SetMask(
    IN const void*    Name,
    IN unsigned char  Len
    );

  api::IBaseLog* GetLog() const { return m_pSuper->GetLog(); }

  bool IsNameValid(const api::STRType Type, const void* Name, const size_t NameLen) const
  {
    // '.' and '..' entries are restricted
    if (m_Strings->GetCharSize(Type) == 1)
      return ((char*)Name)[0] != '.' || NameLen > 2 || (NameLen == 2 && ((char*)Name)[1] != '.');

    if (m_Strings->GetCharSize(Type) == 2)
      return ((unsigned short*)Name)[0] != '.' || NameLen > 2 || (NameLen == 2 && ((unsigned short*)Name)[1] != '.');

    assert(!"unknown string type");
    return false;
  }

  virtual int ReadWriteCurBlock(
    IN bool bWrite
    );

  int ReadWriteDirBlock(
    IN OUT void*  pBuffer,
    IN UINT64     Block,
    IN bool       bWrite
    );

  DRIVER_ONLY(bool EmulateDots(CDir* pParent, FileInfo& Info));
};


};    // namespace UFSD

#endif
