// <copyright file="unixinode.h" company="Paragon Software Group">
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


#ifndef __UFSD_UNIX_INODE_H
#define __UFSD_UNIX_INODE_H

namespace UFSD {

struct CUnixFileSystem;
struct CUnixSuperBlock;
struct CUnixExtent;

class CUnixInode : public UMemBased<CUnixInode>
{
  friend class CUnixFile;

protected:
  UINT64            m_Id;
  unsigned int      m_NfsId;
  bool              m_bDirty;
  bool              m_bDelOnClose;
  size_t            m_RefCount;

  bool              m_bSizeValid;         // false if CUnixFile::Allocate with KEEPSIZE flag called, otherwise true
  UINT64            m_SizeInBytes;        // for CUnixFile::Allocate with KEEPSIZE

public:
  api::IBaseStringManager*  m_Strings;
  CUnixSuperBlock*          m_pSuper;
  bool                      m_bHaveChanges;  //directory has changes (used in enum & unlink in linux)
  avl_link64                m_TreeEntry;

  CUnixInode(IN api::IBaseMemoryManager* Mm);
  virtual ~CUnixInode() {}

  virtual void Destroy() const { delete this; }
  virtual UINT64 Id() const { return m_Id; }
  void SetNfsID(unsigned int Id) { m_NfsId = Id; }
  unsigned int GetNfsID() const { return m_NfsId; }

  //=================================================================
  //              Common functions
  //=================================================================

  api::IBaseLog* GetLog() const;
  bool IsDelOnClose() const { return m_bDelOnClose; }
  void SetDelOnClose(bool f) { m_bDelOnClose = f; }
  bool IsDirty() const { return m_bDirty; }
  void SetDirty(bool bDirty = true) { m_bDirty = bDirty; }
  void IncRefCount(size_t n = 1) { m_RefCount += n; }
  void DecRefCount(size_t n = 1) { assert(m_RefCount >= n); m_RefCount -= n; }
  size_t GetRefCount() const { return m_RefCount; }
  virtual UINT64 GetAllocatedSize(bool bFork = false) const;

  virtual void Release();

  //=================================================================
  //              The End of Common functions
  //=================================================================

  virtual bool IsCompressed() const { return false; }
  virtual bool IsCloned() const { return false; }
  virtual bool IsSparsed() const { return false; }

  virtual int LoadBlocks(
    IN  UINT64        Vcn,
    IN  size_t        Len,
    OUT CUnixExtent*  pOutExtent,
    IN  bool          bFork = false,
    IN  bool          Allocate = false,
    OUT UINT64*       VcnBase = NULL
  );

  virtual int DeCloneExtents() { return ERR_NOERROR; }
  virtual int DecompressExtents() { return ERR_NOERROR; }

  //=================================================================
  //                   Pure virtual functions
  //=================================================================

  //read or write inode data
  virtual int ReadWriteData(UINT64 Offset, size_t* OutLen, void* pBuffer, size_t Size, bool bWrite, bool NotInlineData = false) = 0;

  //Load blocks from inode
  virtual int LoadBlocks(
    IN  UINT64  Vcn,
    IN  size_t  Len,
    OUT UINT64* Lcn,
    OUT size_t* OutLen,
    IN  bool    bFork       = false,
    IN  bool    Allocate    = false,
    OUT bool*   IsAllocated = NULL,
    OUT UINT64* VcnBase     = NULL
    ) = 0;

  //Get and set object info
  virtual int GetObjectInfo(FileInfo* Info, bool bReset = true) const = 0;
  virtual int SetObjectInfo(const CUnixFileSystem* pFS, const FileInfo* Info, size_t Flags) = 0;

  //get fs object type + rwx attributes
  virtual unsigned short GetMode() const = 0;

  //is data inline
  virtual bool IsInlineData() const = 0;

  // inode flags
  virtual UINT64 GetFlags() const = 0;
  virtual void SetFlags(UINT64 Flags) = 0;

  virtual void SetUid(unsigned int uid) = 0;
  virtual void SetGuid(unsigned int gid) = 0;

  virtual unsigned int GetLinksCount() const = 0;
  virtual void IncLinksCount(unsigned int N = 1) = 0;
  virtual void DecLinksCount(unsigned int N = 1) = 0;

  //=================================================================
  //          Allocating blocks
  //=================================================================

  virtual UINT64 GetAllocatedBlocks(bool bFork = false) const = 0;

  virtual UINT64 GetSize(bool bFork = false) const = 0;
#if !defined UFSD_APFS || !defined UFSD_APFS_RO
  virtual int SetSize(UINT64 Size, bool bFork = false);
  virtual int SetSizeValue(UINT64 Size) = 0;
#endif

  virtual int Flush() = 0;

  virtual int Truncate(UINT64 Size) = 0;

  //=================================================================
  // Working with extended attributes
  //=================================================================

  // Copy a list of extended attribute names into the buffer
  // provided, or compute the buffer size required
  virtual int ListEa(
    OUT void*     Buffer,
    IN  size_t    BytesPerBuffer,
    OUT size_t*   Bytes
  ) = 0;

  // Reads extended attribute
  virtual int GetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    OUT void*       Value,
    IN  size_t      BytesPerValue,
    OUT size_t*     Len
  ) = 0;

  // Creates/Writes extended attribute
  virtual int SetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    IN  const void* Value,
    IN  size_t      BytesPerValue,
    IN  size_t      Flags
  ) = 0;

private:
  virtual int SetForkSize(size_t /*Size*/) { return 0; }
};

} //namespace UFSD

#endif
