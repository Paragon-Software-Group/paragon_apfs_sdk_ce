// <copyright file="apfs.h" company="Paragon Software Group">
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

#ifndef __UFSD_APFS_H
#define __UFSD_APFS_H

//
// nfs treats negative offsets in dir as error
//
#ifndef EOD_POS
  //#define EOD_POS   0x7ffffff8
  #define EOD_POS   PU64(0x7fffffffffffffff)
#endif


#include "../unixfs/unixfs.h"

namespace UFSD
{

#ifdef UFSD_CacheAlloc

extern void* g_ApfsFileCache;
extern void* g_ApfsDirCache;

#endif

namespace apfs
{

class CApfsFileSystem : public CUnixFileSystem
{
public:
  CApfsSuperBlock*  m_pApfsSuper;
  PreInitParams     m_Params;

  CApfsFileSystem(
    IN api::IBaseMemoryManager* mm,
    IN api::IBaseStringManager* ss,
    IN api::ITime*              tt,
    IN api::IBaseLog*           log
  );

  //=================================================================
  //          CFileSystem virtual functions
  //=================================================================

  virtual int Init(
    IN  api::IDeviceRWBlock** RwBlock,
    IN  unsigned int          RwBlockCount,
    IN  size_t                Options = 0,
    OUT size_t*               Flags   = NULL,
    IN const PreInitParams*   Params  = NULL
  );

  virtual int ReInit(
    IN size_t     Options = 0,
    OUT size_t*   Flags = NULL,
    IN const PreInitParams* Params  = NULL
  );

  virtual int OpenByID(
    IN  UINT64      ID,
    OUT CFSObject** Fso,
    IN  int         fFollowSymlink = 1
  );

  virtual int SetVolumeInfo(
    IN const void*     VolumeSerial,
    IN size_t          BytesPerVolumeSerial,
    IN api::STRType    VolumeNameType,
    IN const void*     VolumeNameBuffer,
    IN VolumeState     NewState
    );

  virtual const char* GetFsName() const { return "apfs"; }

  //=================================================================
  //          END OF CFileSystem virtual functions
  //=================================================================

  //=================================================================
  //          CUnixFileSystem virtual functions
  //=================================================================

private:
  virtual int InitFS(OUT size_t* Flags);

public:
  virtual size_t GetFsType() const { return FS_APFS; }
  virtual void GetFsVersion(unsigned char* /*Major*/, unsigned char* /*Minor*/) const {}

  virtual unsigned char* GetVolumeSerial(OUT size_t* Bytes) const
  {
    if (Bytes)
      *Bytes = sizeof(m_pApfsSuper->m_pMSB->sb_uuid);
    return m_pApfsSuper->m_pMSB->sb_uuid;
  }

  virtual char* GetVolumeName(OUT size_t* Bytes) const;
  virtual size_t GetMaxVolumeNameLen() const { return VOLNAME_LEN; }
  virtual unsigned int GetMaxFileNameLen() const { return MAX_FILENAME - 1; }

#ifdef UFSD_DRIVER_LINUX
  virtual UINT64 GetEod() const { return EOD_POS; }

#ifdef UFSD_CacheAlloc
  virtual void* GetFileCache() const { return g_ApfsFileCache; }
  virtual void* GetDirCache() const { return g_ApfsDirCache; }
#endif
#endif

  // Find cluster with the last checkpoint superblock
  int FindCSBBlock(const apfs_sb* pMSB, UINT64& Block) const;

  // Handler for IOCTL_GET_APFS_INFO
  virtual int OnGetApfsInfo();

#ifndef UFSD_APFS_RO
  //=================================================================
  //          APFS I/O handlers
  //=================================================================

  // Handler for IOCTL_GET_RETRIEVAL_POINTERS2
  virtual int OnGetRetrievalPointers();

  // Handler for IOCTL_GET_SIZES2
  virtual int OnGetSizes();

  // Handler for IOCTL_OPEN_FORK2
  virtual int OnOpenFork();

  // Handler for IOCTL_GET_INODE_NAMES
  virtual int OnGetInodeNames();

  // Handler for IOCTL_GET_INODES_COUNT
  virtual int OnGetInodesCount();

  // Handler for IOCTL_GET_COMPRESSION2
  virtual int OnGetCompression();

#ifdef UFSD_APFS_SIEVE
  // Handler for IOCTL_FS_SIEVE
  virtual int OnFsSieve();
#endif

private:
  // enumerate 'dead' entries from the 'private-dir' folder
  // Handler for IOCTL_GET_INODE_NAMES with UFSD_INODE_NAMES_READ_DEAD flag
  int EnumDeadInodes() const;
#endif  // UFSD_APFS_RO

#if defined UFSD_APFS_GET_VOLUME_META_BITMAP && !defined UFSD_APFS_RO
  int MarkMetaBlock(
    IN  api::IDeviceRWBlock* Rw,
    IN  UINT64               Block,
    IN  unsigned int         BlockSize,
    OUT unsigned char**      Bitmap
  ) const;

public:
  int GetVolumeMetaBitmap(
    IN  api::IDeviceRWBlock* Rw,
    IN  api::IMessage*       Msg,
    IN  bool                 bDumpData,
    OUT unsigned char**      Bitmap,
    OUT UINT64*              ClustersPerVolume,
    OUT unsigned int*        Log2OfCluster,
    OUT unsigned int*        Errors
  );
#endif

};


} // namespace apfs

} // namespace UFSD

#endif  // __UFSD_APFS_H
