// <copyright file="rwb.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __RWB_H__
#define __RWB_H__

#pragma once

#include <api/types.hpp>
#include <api/log.hpp>
#include <api/errors.hpp>

namespace api {

//read/write flags
//type of data for operation
#define RWB_FLAGS_ANY_DATA              0x00000000
#define RWB_FLAGS_LAYOUT_DATA           0x00000001
#define RWB_FLAGS_META_DATA             0x00000002
#define RWB_FLAGS_FILE_DATA             0x00000004

#define RWB_FLAGS_VERIFY                0x20000000  // Used with ReadBytes
#define RWB_FLAGS_PREFETCH              0x40000000  // Used with ReadBytes
#define RWB_FLAGS_ZERO                  0x80000000  // Used with WriteBytes

//next operation offset will be start at the end of current one
#define RWB_FLAGS_SEQUENTIAL_ACCESS     0x00000100
//we want to wait real flushing data to storage
#define RWB_FLAGS_SYNCHRONOUS_WRITE     0x00001000


////////////////////////////////////////////////////////////////
struct BASE_ABSTRACT_CLASS IDeviceRWBlock
{
    // returns non zero for read-only mode
    virtual int IsReadOnly() const = 0;

    // Returns size of sector in bytes
    // physical size
    virtual unsigned int GetSectorSize() const = 0;

    // Return size of device in bytes
    virtual UINT64 GetNumberOfBytes() const = 0;

    // Discard range of bytes (known as trim for SSD)
    virtual int DiscardRange(
        IN const UINT64& Offset,
        IN const UINT64& Bytes
        ) = 0;

    virtual int ReadBytes(
        IN const UINT64&  Offset,
        IN void*          Buffer,
        IN size_t         Bytes,
        IN unsigned int   Flags = 0 // RWB_FLAGS_XXX
        ) = 0;

    virtual int WriteBytes(
        IN const UINT64&  Offset,
        IN const void*    Buffer,
        IN size_t         Bytes,
        IN unsigned int   Flags = 0 // RWB_FLAGS_XXX
        ) = 0;

    // flush cache buffers
    // waiting for asynchronous write operation finished
    // returns  zero if OK, else error code.
    virtual int Flush(unsigned int Flags) = 0;

    // Function like DeviceIoControl
    virtual int IoControl(
        IN  size_t          IoControlCode,
        IN  const void*     InBuffer        = NULL, // OPTIONAL
        IN  size_t          InBuffSize      = 0,    // OPTIONAL
        OUT void*           OutBuffer       = NULL, // OPTIONAL
        IN  size_t          OutBuffSize     = 0,    // OPTIONAL
        OUT size_t*         BytesReturned   = NULL  // OPTIONAL
        ) = 0;

    //close object and flush all buffers
    virtual int Close() = 0;

    virtual void Destroy() = 0;
};


#define RWB_DEVTYPE_FILE            1
#define RWB_DEVTYPE_CONTAINER       2
#define RWB_DEVTYPE_DEVICE          3//native
#define RWB_DEVTYPE_DISK            4//biontdrv
#define RWB_DEVTYPE_VDDK            5
#define RWB_DEVTYPE_RAM             6


///////////////////////////////////////////////////////////
// GetLog2OfBlockSize
//
// Simple function that returns log2 for most common BytesPerBlock
///////////////////////////////////////////////////////////
static inline
unsigned char
GetLog2OfBlockSize(
    IN unsigned int BytesPerBlock
    )
{
  switch( BytesPerBlock )
  {
    case 1<<0:  return 0;   // 1
    case 1<<1:  return 1;   // 2
    case 1<<2:  return 2;   // 4
    case 1<<3:  return 3;   // 8
    case 1<<4:  return 4;   // 16
    case 1<<5:  return 5;   // 32
    case 1<<6:  return 6;   // 64
    case 1<<7:  return 7;   // 128
    case 1<<8:  return 8;   // 256
    case 1<<9:  return 9;   // 512
    case 1<<10: return 10;  // 1K
    case 1<<11: return 11;  // 2K
    case 1<<12: return 12;  // 4K
    case 1<<13: return 13;  // 8K
    case 1<<14: return 14;  // 16K
    case 1<<15: return 15;  // 32K
    case 1<<16: return 16;  // 64K
    case 1<<17: return 17;  // 128K
    case 1<<18: return 18;  // 256K
    case 1<<19: return 19;  // 512K
    case 1<<20: return 20;  // 1M
    case 1<<21: return 21;  // 2M
    case 1<<22: return 22;  // 4M
    case 1<<23: return 23;  // 8M
    case 1<<24: return 24;  // 16M
    case 1<<25: return 25;  // 32M
  }
  return 9; // default
}


#ifndef UFSD_DRIVER_LINUX
//
// Helper class to overwrite only some of functions (e.g. ReadBlocks/WriteBlocks)
//
struct CRWBlockA : public api::IDeviceRWBlock NOTCOPYABLE {

  api::IDeviceRWBlock*  m_Rw;
  ILog*                 m_Log;

  CRWBlockA()
  : m_Rw(NULL)
  , m_Log(NULL)
  {}

  virtual int IsReadOnly() const
    { return !m_Rw ? true : m_Rw->IsReadOnly(); }

  virtual unsigned int GetSectorSize() const
    { return !m_Rw ? 1 : m_Rw->GetSectorSize(); }

  virtual UINT64 GetNumberOfBytes() const
    { return !m_Rw ? 1 : m_Rw->GetNumberOfBytes(); }

  virtual int DiscardRange( const UINT64& Offset, const UINT64& Bytes )
    { return !m_Rw ? ERR_BADPARAMS : m_Rw->DiscardRange( Offset, Bytes ); }

  virtual int ReadBytes(const UINT64& Offset, void* pBuff, size_t Bytes, unsigned int Flags)
    { return !m_Rw ? ERR_BADPARAMS : m_Rw->ReadBytes(Offset, pBuff, Bytes, Flags); }

  virtual int WriteBytes(const UINT64& Offset, const void* pBuff, size_t Bytes, unsigned int Flags)
    { return !m_Rw ? ERR_BADPARAMS : m_Rw->WriteBytes(Offset, pBuff, Bytes, Flags); }

  virtual int Flush(unsigned int Flags)
    { return !m_Rw ? ERR_BADPARAMS : m_Rw->Flush(Flags); }

  virtual int IoControl(
        IN  size_t          IoControlCode,
        IN  const void*     InBuffer       = NULL,
        IN  size_t          InBufferSize   = 0,
        OUT void*           OutBuffer      = NULL,
        IN  size_t          OutBufferSize  = 0,
        OUT size_t*         BytesReturned  = NULL
        )
    { return !m_Rw ? ERR_BADPARAMS : m_Rw->IoControl( IoControlCode, InBuffer, InBufferSize, OutBuffer, OutBufferSize, BytesReturned ); }

  //close object and flush all buffers
  virtual int Close()
    { return !m_Rw ? ERR_BADPARAMS : m_Rw->Close(); }

  virtual void Destroy()
    { if(m_Rw) m_Rw->Destroy(); m_Rw = NULL; }

};

#endif

} // namespace api

#endif //__RWB_H__
