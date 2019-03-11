// <copyright file="dirapfs.h" company="Paragon Software Group">
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

#ifndef __UFSD_DIRAPFS_H
#define __UFSD_DIRAPFS_H

#include "../unixfs/unixdir.h"

namespace UFSD
{

namespace apfs
{

class CApfsDir : public CUnixDir
{
  CEntryTreeEnum*       m_pEntryEnum;
public:
  CApfsDir(CUnixFileSystem* pFS);
  virtual ~CApfsDir();

  virtual UINT64 GetObjectID() const;

  virtual int Create(
      IN api::STRType     Type,
      IN const void*      Name,
      IN size_t           NameLen,
      IN unsigned short   Mode,
      IN unsigned int     Uid,
      IN unsigned int     Gid,
      IN const void*      Data,
      IN size_t           DataLen,
      OUT CFSObject**     Fso
      );

  virtual int GetSubDirAndFilesCount(
      OUT bool*   IsEmpty,        // may be NULL
      OUT size_t* SubDirsCount,   // may be NULL
      OUT size_t* FilesCount      // may be NULL
     );

  virtual int Rename(
      IN api::STRType     Type,
      IN const void*      SrcName,
      IN size_t           SrcLen,
      IN CFSObject*       Src,
      IN CDir*            TgtDir,
      IN const void*      DstName,
      IN size_t           DstLen
      );

  virtual bool IsReadOnly();

  virtual int CheckForEmpty(
      IN CUnixInode* pInode
      );

private:
  //=================================================================
  //          CUnixDir virtual functions
  //=================================================================
  virtual int FindNext(
      IN CEntryNumerator* pEnum,
      OUT FileInfo&       Info,
      OUT CUnixInode**    pInode
      );

  virtual int FindDirEntry(
      IN  api::STRType    Type,
      IN  const void*     Name,
      IN  size_t          NameLen,
      OUT FileInfo*       pInfo,
      OUT CUnixInode**    pInode
      );

  virtual void CreateFsObject(
      IN  const FileInfo* pInfo,
      OUT CUnixDir**      pDir
      ) const;

  //initialize enumerator
  virtual int InitEnumerator(
      IN  size_t            Options,
      OUT CUnixEnumerator** pEnum,
      IN  CUnixInode*       pInode = NULL
      ) const;

  virtual int UnlinkInternal(
      IN CUnixInode*  pInode,
      IN CFSObject*   Fso,
      IN FileInfo     Info
      ) const;
};


//CDir implementation for volumes dir (emulated directory where all volumes are shown as directories)
class CApfsVolumeDir : public CApfsDir
{
public:
  CApfsVolumeDir(CUnixFileSystem* pFS)
    : CApfsDir(pFS) {}

  //=================================================================
  //          Redefine virtual functions for CApfsVolumeDir
  //=================================================================
  virtual int Init(
      IN UINT64        Inode,
      IN CDir*         pParent,
      IN CUnixInode*   pInode,
      IN api::STRType  Type,
      IN const void*   Name,
      IN unsigned      NameLen
      );

  // Counts subdirectories and/or files
  virtual int GetSubDirAndFilesCount(
      OUT bool*   IsEmpty,        // may be NULL
      OUT size_t* SubDirsCount,   // may be NULL
      OUT size_t* FilesCount      // may be NULL
      );

  // Returns this object ID
  virtual UINT64 GetObjectID() const { return APFS_VOLUMES_DIR_ID; }

  // Returns parent unique ID
  virtual UINT64 GetParentID(IN unsigned int /*Pos*/) const { return APFS_ROOT_INO; }

  virtual int GetObjectInfo(OUT t_FileInfo& Info);

  virtual int Flush(IN bool bDelete)
  {
    if (bDelete)
      Destroy();
    return ERR_NOERROR;
  }

  virtual bool IsReadOnly() { return true; }

  //=================================================================
  //          Functions which using m_pInode and write functions is not implemented
  //=================================================================
  virtual int ListEa(
      OUT void* /*Buffer*/,
      IN  size_t /*BytesPerBuffer*/,
      OUT size_t* Bytes
      )
  {
    *Bytes = 0;
    return ERR_NOERROR;
  }

  virtual int GetEa(
      IN  const char* /*Name*/,
      IN  size_t      /*NameLen*/,
      OUT void*       /*Value*/,
      IN  size_t      /*BytesPerValue*/,
      OUT size_t*     /*Len*/
      )
  {
    return ERR_NOFILEEXISTS;
  }

  virtual int SetEa(
      IN const char*  /*Name*/,
      IN size_t       /*NameLen*/,
      IN const void*  /*Value*/,
      IN size_t       /*BytesPerValue*/,
      IN size_t       /*Flags*/
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int SetObjectInfo(
      IN const t_FileInfo&  /*Info*/,
      IN size_t             /*Flags*/)
  {
#ifdef UFSD_DRIVER_LINUX
    return ERR_NOERROR;
#else
    return ERR_NOTIMPLEMENTED;
#endif
  }

  virtual int Unlink(
      IN api::STRType /*Type*/,
      IN const void * /*Name*/,
      IN size_t       /*NameLen*/,
      IN CFSObject*   /*Fso*/ = NULL
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Create(
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
    return ERR_NOTIMPLEMENTED;
  }

  virtual int CreateDir(
      IN  api::STRType  /*Type*/,
      IN  const void*   /*Name*/,
      IN  size_t        /*NameLen*/,
      OUT CDir**        /*pDir*/
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int CreateFile(
      IN  api::STRType  /*Type*/,
      IN  const void*   /*Name*/,
      IN  size_t        /*NameLen*/,
      OUT CFile**       /*pFile*/
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Rename(
      IN api::STRType /*Type*/,
      IN const void*  /*SrcName*/,
      IN size_t       /*SrcLen*/,
      IN CFSObject*   /*Src*/,
      IN CDir*        /*TgtDir*/,
      IN const void*  /*DstName*/,
      IN size_t       /*DstLen*/
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Link(
      IN CFSObject*   /*Fso*/,
      IN api::STRType /*Type*/,
      IN const void*  /*Name*/,
      IN size_t       /*NameLen*/
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

private:
  virtual int InitEnumerator(
      IN  size_t            Options,
      OUT CUnixEnumerator** pEnum,
      IN  CUnixInode*       pInode = NULL
      ) const;

  //internal function for destroying object called from CUnixDir::CloseParent
  virtual void DestroyInternal() { delete this; }

  //Set name fields
#ifdef UFSD_DRIVER_LINUX
  int SetVolumeDirName(
      IN CDir*        pParent,
      IN api::STRType Type,
      IN const void*  Name,
      IN unsigned int NameLen
      )
  {
    m_tObjectType = FSObjDir;
    m_Parent = pParent;
    Free2(m_aName);
    m_aName = (unsigned short*)Malloc2((NameLen + 1) * sizeof(unsigned short));
#ifndef UFSD_MALLOC_CANT_FAIL
    if (NULL == m_aName)
      return ERR_NOMEMORY;
#endif
    Memcpy2(m_aName, Name, NameLen);
    m_tNameType = (unsigned char)Type;
    m_NameLen = (unsigned short)NameLen;
    reinterpret_cast<unsigned short*>(m_aName)[NameLen] = 0;

    return ERR_NOERROR;
  }
#endif
};


} // namespace apfs

} // namespace UFSD


#endif
