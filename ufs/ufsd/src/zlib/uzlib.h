// <copyright file="uzlib.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

////////////////////////////////////////////////////////////////
//
// 2017, This file contains declarations of zlib compression functions
//
////////////////////////////////////////////////////////////////

#ifndef __UZLIB_H__
#define __UZLIB_H__

#include <api/compress.hpp>
#include <api/errors.hpp>
#include <api/assert.hpp>

namespace UFSD{

#define ZLIB_ERROR_NOERROR                  0
#define ZLIB_ERROR_MEM                      -1   //
#define ZLIB_ERROR_TOOSMALL                 -2    // Dst buffer is too small
#define ZLIB_ERROR_BAD_DATA                 -3    //  The specified buffer contains ill-formed data.
#define ZLIB_ERROR_OTHER                    1

///////////////////////////////////////////////////////////
// ZlibDeCompressBuffer
//
// Decompresses CompressedBuffer into UncompressedBuffer
///////////////////////////////////////////////////////////
int
ZlibDeCompressBuffer(
    IN api::IBaseMemoryManager* Mm,
    OUT void*                   UncompressedBuffer,
    IN size_t                   UncompressedBufferSize,
    IN const void*              CompressedBuffer,
    IN size_t                   CompressedBufferSize,
    OUT size_t*                 FinalUncompressedSize
    );

class CZlibCompressor : public UMemBased<CZlibCompressor>, public api::ICompress
{
public:

  CZlibCompressor(api::IBaseMemoryManager* Mm)
    : UMemBased<CZlibCompressor>(Mm)
    {}

  virtual ~CZlibCompressor() {}

  virtual int SetCompressLevel(unsigned int /*CompressLevel*/) { return ERR_NOTIMPLEMENTED; }

  virtual int Compress(
    const void*  /*InBuf*/,
    size_t       /*InBufLen*/,
    void*        /*OutBuf*/,
    size_t       /*OutBufSize*/,
    size_t*      /*OutLen*/
    )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Decompress(
    const void*  InBuf,
    size_t       InBufLen,
    void*        OutBuf,
    size_t       OutBufSize,
    size_t*      OutLen
    );

  virtual void Destroy() { delete this; }
};

}//namespace UFSD

#endif //__UZLIB_H__
