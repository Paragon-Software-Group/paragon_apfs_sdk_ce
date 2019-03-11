// <copyright file="unixfile.h" company="Paragon Software Group">
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


#ifndef __UFSD_UNIX_FILE_H
#define __UFSD_UNIX_FILE_H

#include "unixfs.h"

namespace UFSD
{

//=================================================================
class CUnixFile : public CFile
{
  friend class CUnixDir;
protected:
  CUnixFileSystem* m_pFS;
  CUnixInode*      m_pInode;
  bool             m_bFork;         // file has been opened as a resource fork
#ifdef UFSD_DRIVER_LINUX
  unsigned short*   m_aName;
  unsigned short    m_NameLen;
  char              m_tNameType;
#endif

public:
  CUnixFile(IN CUnixFileSystem* vcb, IN bool bFork = false);
  virtual ~CUnixFile();

  api::IBaseLog* GetLog() const { return GetVcbLog( m_pFS ); }

  CUnixInode* GetInode() const { return m_pInode; }

  bool IsFork() const { return m_bFork; }
  bool IsSparsed() const { return m_pInode->IsSparsed(); }

  virtual int Init(
    IN UINT64       Inode,
    IN CDir*        pParent,
    IN CUnixInode*  pInode,
    IN api::STRType type,
    IN const void*  Name,
    IN unsigned     NameLength
  );

  //=================================================================
  //          CFSObject  virtual functions
  //=================================================================

  virtual void Destroy();

  virtual int GetObjectInfo(
    OUT t_FileInfo& Info
  );

  virtual int SetObjectInfo(
    IN const t_FileInfo&  Info,
    IN size_t             Flags
  );

  virtual int Flush(
    IN bool bDelete = false
  );

  // Returns this object ID
  virtual UINT64 GetObjectID() const { return m_pInode->Id(); }

  // Returns parent unique ID
  virtual UINT64 GetParentID(IN unsigned int Pos) const
  {
    return (!Pos && m_Parent) ? m_Parent->GetObjectID() : MINUS_ONE_64;
  }

#ifdef UFSD_DRIVER_LINUX
  virtual const void* GetNameByPos(
    IN  unsigned int  Pos,
    OUT api::STRType& Type,
    OUT size_t&       NameLen
    ) const
  {
    if (Pos == 0)
    {
      Type = api::StrUTF16;
      NameLen = m_NameLen;
      return m_aName;
    }
    return NULL;
  }
#endif

  virtual int ReadSymLink(
    OUT char*       Buffer,
    IN  size_t      BufLen,
    OUT size_t*     BytesRead
  );

  //=================================================================
  //        Extended Attributes
  //=================================================================

  // Copy a list of extended attribute names into the buffer
  // provided, or compute the buffer size required
  virtual int ListEa(
    OUT void*     Buffer,
    IN  size_t    BytesPerBuffer,
    OUT size_t*   Bytes
  )
  {
    return m_pInode->ListEa(Buffer, BytesPerBuffer, Bytes);
  }

  // Reads extended attribute
  virtual int GetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    OUT void*       Value,
    IN  size_t      BytesPerValue,
    OUT size_t*     Len
  )
  {
    return m_pInode->GetEa(Name, NameLen, Value, BytesPerValue, Len);
  }

  // Creates/Writes extended attribute
  virtual int SetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    IN  const void* Value,
    IN  size_t      BytesPerValue,
    IN  size_t      Flags
  );

  //=================================================================
  //          END OF CFSObject  virtual functions
  //=================================================================

  //=================================================================
  //          CFile  virtual functions
  //=================================================================

  //Retrieves file size
  //Returns:
  //  0 - if ok
  //  error code otherwise
  virtual int GetSize(
    OUT UINT64& BytesPerFile,
    OUT UINT64* ValidSize = NULL,
    OUT UINT64* AllocSize = NULL
  );

  //Truncates or Expands file
  //Returns:
  //  0 if everything is fine
  //  error code - otherwise
  // if ValidSize is not NULL then update 'valid' size too (if FS supports it)
  virtual int SetSize(
    IN  const UINT64&  BytesPerFile,
    IN  const UINT64*  ValidSize = NULL,
    OUT UINT64*        AllocSize = NULL
  );

  //Read len bytes from file from specified position
  //  buffer  - output buffer
  //  len - number of bytes to read
  //  read - number of bytes read from file
  //Returns:
  // 0 if all ok
  // error code otherwise
  virtual int Read(
    IN  const UINT64& Offset,
    OUT size_t&       read,
    OUT void *        buffer,
    IN  size_t        len
  );

  //Write len bytes to file from buffer
  //to file from specified position
  //  buffer  - output buffer
  //  len - number of bytes to read
  //  written - number of bytes written
  //Returns:
  // 0 if all ok
  // error code otherwise
  virtual int Write(
    IN  const UINT64&   Offset,
    OUT size_t&         written,
    IN  const void*     buffer,
    IN  size_t          len
  );

  // Translates Virtual Byte Offset into Logical Byte Offset
  virtual int GetMap(
    IN  const UINT64& Vbo,
    IN  const UINT64& Bytes,
    IN  size_t        Flags,
    OUT MapInfo*      Map
  );

  // Allocates the disk space within the range [Offset,Offset+Bytes)
  virtual int Allocate(
    IN const UINT64&  Offset,
    IN const UINT64&  Bytes,
    IN size_t         Flags = 0,   // see UFSD_ALLOCATE_XXX flags
    OUT MapInfo*      Map = NULL
  );

  //=================================================================
  //          END OF CFile  virtual functions
  //=================================================================
};

}


#endif   // __UFSD_UNIXFILE_H
