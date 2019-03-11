// <copyright file="fileio.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __FILEIO_H__
#define __FILEIO_H__

#pragma once

#include <api/types.hpp>
#include <api/fsattribs.hpp>

namespace api {

//Creation disposition
#define F_FILE_CREATE                   0x00000001     // create truncated
#define F_FILE_ALWAYS                   0x00000002     // create if not exists
#define F_FILE_OPEN                     0x00000004     // open existing

//File mode
#define F_FILE_READ                     0x00000100     // read
#define F_FILE_WRITE                    0x00000200     // write

#define F_FILE_SHARE_READ               0x00000400     // share read
#define F_FILE_SHARE_WRITE              0x00000800     // share write
#define F_FILE_SHARE_DELETE             0x00001000     // share delete

//flags
#define F_FILE_CACHED                   0x00010000     // for working with cachemgm module
#define F_FILE_SPARSE                   0x00100000     // create as sparse file
#define F_FILE_WRITE_THROUGH            0x00200000     // FILE_FLAG_WRITE_THROUGH for not block drivers

#define F_FLAGS_PREFETCH                0x40000000     // Used with ReadBytes
#define F_FLAGS_ZERO                    0x80000000     // Used with WriteBytes


///////////////////////////////////////////////
//
//   File  i/o
//
///////////////////////////////////////////////
#define MAX_FILE_NAME   256

struct FileInfo
{
    UINT64          CrTime;
    UINT64          ModTime;
    UINT64          ReffTime;
    UINT64          Size;
    unsigned int    Attrib; //UFSD
    unsigned short  NameLen;
    char            Name[MAX_FILE_NAME*4];//utf8
};

struct VolumeInfo
{
    UINT64          FreeClusters;
    UINT64          TotalClusters;
    size_t          BytesPerCluster;
    size_t          BytesPerSector;
    unsigned char   Serial[16];
    unsigned short  NameLen;
    char            Name[MAX_FILE_NAME];//utf8
};


class BASE_ABSTRACT_CLASS IFile
{
public:

    // destroy object
    virtual int   Close()                               = 0;

    // get file size in bytes
    virtual int   GetSize(UINT64* cSize)                = 0;

    // set file size in bytes
    virtual int   SetSize(const UINT64&  newSize)       = 0;

    // read file bytes from offset
    virtual int   ReadBytes(
      const UINT64&      Start,
      void*              Buf,
      size_t             Len
    )
    {
      size_t r;
      return ReadBytes(Start, Buf, Len, &r, 0);
    }

    virtual int   ReadBytes(
      const UINT64&      Start,
      void*              Buf,
      size_t             Len,
      size_t*            Read,
      unsigned int       Flags
    )                                                   = 0;

    virtual int   Prefetch(
      const UINT64&      Start,
      size_t             Len,
      unsigned int       Flags
    )
    {
      size_t r;
      return ReadBytes(Start, NULL, Len, &r, Flags | F_FLAGS_PREFETCH);
    }

    // write file bytes from offset
    virtual int   WriteBytes(
      const UINT64&      Start,
      const void*        Buf,
      size_t             Len
    )
    {
      size_t w;
      return WriteBytes(Start, Buf, Len, &w, 0);
    }

    virtual int   WriteBytes(
      const UINT64&      Start,
      const void*        Buf,
      size_t             Len,
      size_t*            Written,
      unsigned int       Flags
    )                                                   = 0;

    virtual int   Zero(
        const UINT64&    Start,
        const size_t     Len)
    {
      size_t w;
      return WriteBytes(Start, NULL, Len, &w, F_FLAGS_ZERO);
    }

    // flush file
    virtual int   Flush(unsigned int Flags)             = 0;

    // get file times
    virtual int   GetTimes(
      UINT64*            CrTime,
      UINT64*            ModTime,
      UINT64*            ReffTime
    )                                                   = 0;

    virtual int   GetFileInfo(FileInfo* pInfo)          = 0;
};


// factory
class BASE_ABSTRACT_CLASS IFileIo
{
public:

    virtual int AddNetConnection(
        const char*     aNetResource,
        const char*     aUser,
        const char*     aPassword,
        unsigned int&   aConnectionId) = 0;

    virtual int RemoveNetConnection(
        const unsigned int  aConnectionId,
        const bool          aForce) = 0;

   // open or create file
   virtual int  OpenCreate(
     const char*        Path,                                   //utf-8
     const char*        Params,                                 //utf-8 example: "encrypt_alg=aes256;..."
     unsigned int       ModeAndFlags,                           //read/write/share_read/share_write //open/create
     IFile**            pFile)                          = 0;

   // check if file exists
   virtual int  Exists(
     const char*        Path,
     int*               Ret)                            = 0;    //utf-8

   // delete file or subdirectory
   virtual int  Delete(const char* Path)                = 0;    //utf-8

   // make directory
   virtual int  MkDir(const char* Path)                 = 0;    //utf-8

   virtual int  Rename(
     const char*        pSrcName,
     const char*        pDstName
    )                                                   = 0;

   virtual int  FindFirst(
     const char*        pMask,
     size_t*            Handle,
     FileInfo*          pInfo
   )                                                     = 0;
   virtual int  FindNext(size_t Handle, FileInfo* pInfo) = 0;
   virtual int  FindClose(size_t Handle)                 = 0;

   virtual int  GetFileInfo(const char* Path, FileInfo* pInfo) = 0;

   virtual int  GetVolumeInfo(const char* pRootPath, VolumeInfo* pInfo) = 0;

   virtual void Destroy() = 0;

   // get pure path
   // extract pure path without any credentials
   virtual const char* GetPurePath(
     const char*        Path)                            = 0;

   // Set the default settings for all files (ex.: compressing, crypting, caching).
   // The parameters can be overwritten with 'OpenCreate' call.
   virtual int SetParameters(const char* aParams) = 0;
};

} // namespace api

#endif //__FILEIO_H__
