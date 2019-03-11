// <copyright file="unixfs.h" company="Paragon Software Group">
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


#ifndef __UFSD_UNIX_FS_H
#define __UFSD_UNIX_FS_H

#include "unixsuperblock.h"

namespace UFSD
{


struct CUnixFileSystem : CFileSystem
{
  unsigned int        m_umask;
  CUnixSuperBlock*    m_pSuper;

  unsigned int        m_NfsId;

  //
  // These members are the copy of parameters of IoControl
  // They are used to reduce code size and to simplify
  // I/O processing
  //
  struct {
    const void*       InBuffer;
    size_t            InBufferSize;
    void*             OutBuffer;
    size_t            OutBufferSize;
    size_t*           BytesReturned;
    CFSObject*        pObject;
    CDir*             pParent;
  } m_IO;

  CUnixFileSystem(
    IN api::IBaseMemoryManager* mm,
    IN api::IBaseStringManager* ss,
    IN api::ITime*              tt,
    IN api::IBaseLog*           log
  );
  virtual ~CUnixFileSystem() { Dtor(); }

  int Dtor();

  //Is read only (for file or dir if Id != 0, or for whole fs if Id == 0)
  bool IsReadOnly(UINT64 Id = 0) const { return m_pSuper->IsReadOnly(Id); }

  unsigned int GetBlockSize() const { return m_pSuper->GetBlockSize(); }
  UINT64 GetFreeBlocksCount() const { return m_pSuper->GetFreeBlocksCount(); }
  UINT64 GetBlocksCount() const { return m_pSuper->GetBlocksCount(); }

  //=================================================================
  //          CFileSystem virtual functions
  //=================================================================

  virtual void Destroy() { delete this; }

  //Initializes File System
  //  RwBlock - pointer to Read Write Block  object
  //  Option  - FS specific options, see below
  //Returns:
  //  0 - if success;desc non zero;
  //  error code otherwise.
  virtual int Init(
    IN  api::IDeviceRWBlock** RwBlock,
    IN  unsigned int          RwBlockCount,
    IN  size_t                Options = 0,
    OUT size_t*               Flags   = NULL,
    IN const PreInitParams*   Params  = NULL
  );

  //Reinitializes File System
  //  Option  - FS specific options, see below
  //Returns:
  //  0 - if success;desc non zero;
  //  error code otherwise.
  virtual int ReInit(
    IN size_t     Options = 0,
    OUT size_t*   Flags = NULL,
    IN const PreInitParams* Params  = NULL
  );

  //Flushes directory tree
  virtual int Flush(
    IN bool bWait = false
  );

  //Gets allocation information
  virtual int GetVolumeInfo(
    OUT UINT64*        FreeClusters,
    OUT UINT64*        TotalClusters,
    OUT size_t*        BytesPerCluster,
    OUT void*          VolumeSerial = NULL,
    IN  size_t         BytesPerVolumeSerial = 0,
    OUT size_t*        RealBytesPerSerial = NULL,
    IN  api::STRType   VolumeNameType = api::StrUnknown,
    OUT void*          VolumeNameBuffer = NULL,
    IN  size_t         VolumeNameBufferLength = 0,
    OUT VolumeState*   VolState = NULL,
    OUT size_t*        BytesPerSector = NULL
  );

  // Set volume information.
  virtual int SetVolumeInfo(
    IN const void*    VolumeSerial,
    IN size_t         BytesPerVolumeSerial,
    IN api::STRType   VolumeNameType,
    IN const void*    VolumeNameBuffer,
    IN VolumeState    NewState
  );

  virtual const char* GetFsName() const = 0;

#ifdef UFSD_DRIVER_LINUX
      // Returns 0 if names are not equal
      //        +1 if names are equal
      //        -1 if some error occurs
  virtual int IsNamesEqual(
    IN const unsigned short*  Name1,
    IN size_t                 Len1,
    IN const unsigned short*  Name2,
    IN size_t                 Len2
  );

  // Returns hash of name
  virtual unsigned int NameHash(
    IN const unsigned short*  Name,
    IN size_t                 Len
  );

  virtual int GetParent(
    IN  CFSObject*  Fso,
    OUT CFSObject** Parent
  );
#endif

  //=================================================================
  //          END OF CFileSystem virtual functions
  //=================================================================

  virtual size_t GetFsType() const = 0;
  virtual void GetFsVersion(unsigned char* Major, unsigned char* Minor) const = 0;
  virtual unsigned char* GetVolumeSerial(OUT size_t* Bytes) const = 0;
  virtual char* GetVolumeName(OUT size_t* Bytes) const = 0;
  virtual bool IsFsDirty() const { return false; }
  virtual void SetFsDirty(bool /*bDirty*/ = true) const {}

  virtual UINT64 GetMaxFileSize() const { return MINUS_ONE_64; }
  virtual unsigned int GetMaxFileNameLen() const = 0;
  virtual size_t GetMaxVolumeNameLen() const = 0;

  DRIVER_ONLY(virtual UINT64 GetEod() const = 0);

#ifdef UFSD_CacheAlloc
  virtual void* GetFileCache() const { return NULL; }
  virtual void* GetDirCache() const { return NULL; }
#endif

  //Opens Root directory and assigns m_pRootDir a value.
  //Called when File System is mounted
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  template<typename T>
  int OpenRoot(UINT64 RootID)
  {
    ULOG_MESSAGE(( GetVcbLog( this ), "CUnixFileSystem::OpenRoot"));

    if (m_RootDir != NULL)
    {
      ULOG_WARNING(( GetVcbLog( this ), "Root is already opened"));
      return ERR_NOERROR;
    }

#ifdef UFSD_CacheAlloc
    void* p = UFSD_CacheAlloc(GetDirCache(), true);
    CHECK_PTR(p);
    T* root = new(p) T(this);
#else
    T* root = new(m_Mm) T(this);
    CHECK_PTR(root);
#endif

    int Status = root->Init(RootID, NULL, NULL, api::StrUTF8, "/", 1);

    if (UFSD_SUCCESS(Status))
      m_RootDir = root;
    else
#ifdef UFSD_CacheAlloc
      root->Destroy();
#else
      delete root;
#endif

    return Status;
  }

  int OpenFsoByID(UINT64& id, int fFollowSymlink, unsigned short& pMode, CUnixInode** pInode) const;

#if !defined UFSD_NO_USE_IOCTL && defined UFSD_DRIVER_LINUX
  int OpenByNfsID(
    IN unsigned int ID,
    OUT CFSObject** Fso
  ) const;

  unsigned int GetNfsID() { return ++m_NfsId; }
#endif

  //=================================================================
  //          UnixFs I/O handlers
  //=================================================================

  // Function a-la DeviceIoControl
  virtual int IoControl(
      IN  size_t      FsIoControlCode,
      IN  const void* InBuffer      = NULL,
      IN  size_t      InBufferSize  = 0,
      OUT void*       OutBuffer     = NULL,
      IN  size_t      OutBufferSize = 0,
      OUT size_t*     BytesReturned = NULL
      );

  // Handler for IOCTL_GET_RETRIEVAL_POINTERS2
  virtual int OnGetRetrievalPointers();

  // Handler for IOCTL_GET_VOLUME_BITMAP
  virtual int OnGetVolumeBitmap();

  // Handler for IOCTL_GET_VOLUME_LCN2LBO
  virtual int OnGetVolumeLcn2Lbo();

  // Handler for IOCTL_GET_VOLUME_BYTES
  virtual int OnGetBytesPerVolume();

  // Handler for IOCTL_GET_VOLUME_LBO2LCN
  virtual int OnGetVolumeLbo2Lcn();

  // Handler for IOCTL_GET_SIZES2
  virtual int OnGetSizes();

  // Handler for IOCTL_GET_FSTYPE
  virtual int OnGetFsType();

  // Handler for IOCTL_GET_FSVERSION
  virtual int OnGetFsVersion();

  // Handler for IOCTL_GET_DIRTY
  virtual int OnGetDirty();

  // Handler for IOCTL_SET_DIRTY
  virtual int OnSetDirty();

  // Handler for IOCTL_CLEAR_DIRTY
  virtual int OnClearDirty();

  // Handler for IOCTL_GET_VOLUMEINFO
  virtual int OnGetVolumeInfo();

  // Handler for IOCTL_GET_ATTRIBUTES2
  virtual int OnGetAttributes();

  // Handler for IOCTL_GET_APFS_INFO
  virtual int OnGetApfsInfo() { return ERR_NOTIMPLEMENTED; }

  // Handler for IOCLT_OPEN_FORK2
  virtual int OnOpenFork() { return ERR_NOERROR; }

  // Handler for IOCTL_FS_GET_BADBLOCKS
  virtual int OnGetBadBlocks() { return ERR_NOTIMPLEMENTED; }

  // Handler for IOCTL_GET_INODE_NAMES
  virtual int OnGetInodeNames() { return ERR_NOERROR; }

  // Handler for IOCTL_ON_GET_INODES_COUNT
  virtual int OnGetInodesCount() { return ERR_NOERROR; }

  // Handler for IOCTL_GET_COMPRESSION2
  virtual int OnGetCompression() { return UFSD_COMPRESSION_FORMAT_NONE; }

  // Handler for IOCTL_FS_QUERY_EXTEND
  virtual int OnQueryFsExtend() { return ERR_NOTIMPLEMENTED; }

  // Handler for IOCTL_FS_EXTEND
  virtual int OnFsExtend() { return ERR_NOTIMPLEMENTED; }

  // Handler for IOCTL_FS_QUERY_SHRINK
  virtual int OnQueryFsShrink() { return ERR_NOTIMPLEMENTED; }

  // Handler for IOCTL_FS_SHRINK
  virtual int OnFsShrink() { return ERR_NOTIMPLEMENTED; }

  // Handler for IOCTL_FS_SIEVE
  virtual int OnFsSieve() { return ERR_NOTIMPLEMENTED; };

#if !defined UFSD_NO_USE_IOCTL && defined UFSD_DRIVER_LINUX
  // Handler for IOCTL_ENCODE_FH
  int OnEncodeFH();

  // Handler for IOCTL_DECODE_FH
  int OnDecodeFH();
#endif

private:
  //Initializes Specific File System (internal function)
  //Returns:
  //  0 - if success;desc non zero;
  //  error code otherwise.
  virtual int InitFS(
    OUT size_t* Flags
  ) = 0;

  DRIVER_ONLY(void SetupBlocks());
};


}

#endif
