// <copyright file="compress.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __COMPRESS_H__
#define __COMPRESS_H__

#pragma once

#include <api/types.hpp>

namespace api {

//Compression algorithms
//!!! DO NOT change ID order
//!!! only add new
#define I_COMPRESS_NONE         0                       /* without compression */
#define I_COMPRESS_DEFLATE      1                       /* DEFLATE (RFC 1951) */
#define I_COMPRESS_LZ4          2                       /* http://cyan4973.github.io/lz4/ */
#define I_COMPRESS_RLEP         3                       /* RLE paragon modificated */
#define I_COMPRESS_LZ4HC        4                       /* http://cyan4973.github.io/lz4/ */
#define I_COMPRESS_LZFSE        5                       /* https://github.com/lzfse/lzfse */
#define I_COMPRESS_LZO          6                       /* http://www.oberhumer.com/opensource/lzo/ */
#define I_COMPRESS_ZSTD         7                       /* https://github.com/facebook/zstd */

///////////////////////////////////////////////
//
//   Compression
//
///////////////////////////////////////////////

// provider
class BASE_ABSTRACT_CLASS ICompress
{
public:

  virtual int           SetCompressLevel(
    unsigned int            CompressLevel       //0-default compress level (fast)
  ) = 0;

  virtual int           Compress(
    const void*             InBuf,
    size_t                  InBufLen,
    void*                   OutBuf,
    size_t                  OutBufSize,
    size_t*                 OutLen
  ) = 0;

  virtual int           Decompress(
    const void*             InBuf,
    size_t                  InBufLen,
    void*                   OutBuf,
    size_t                  OutBufLen,
    size_t*                 OutSize
  ) = 0;

  virtual void          Destroy() = 0;
};


// factory
class ICompressFactory
{
public:

  virtual void          Destroy() = 0;

  virtual int           CreateProvider(
    unsigned int            Method,             //I_COMPRESS_XXX
    ICompress**             pICompress
  ) = 0;
};

} // namespace api

#endif //__COMPRESS_H__
