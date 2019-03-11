// <copyright file="uzlib.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

////////////////////////////////////////////////////////////////
//
// This file contains wrapper implementation of zlib compression functions
//
////////////////////////////////////////////////////////////////

#include "../h/versions.h"

#ifdef UFSD_ZLIB

#include "zlib.h"
#include "uzlib.h"

namespace UFSD {

int
ZlibDeCompressBuffer(
  IN api::IBaseMemoryManager* m_Mm,
  OUT void*                   UncompressedBuffer,
  IN size_t                   UncompressedBufferSize,
  IN const void*              CompressedBuffer,
  IN size_t                   CompressedBufferSize,
  OUT size_t*                 FinalUncompressedSize
)
{
  if (*(unsigned char*)CompressedBuffer == 0xff)
  {
    if (UncompressedBufferSize < CompressedBufferSize - 1)
      return ERR_INSUFFICIENT_BUFFER;

    Memcpy2(UncompressedBuffer, Add2Ptr(CompressedBuffer, 1), CompressedBufferSize - 1);
    if (FinalUncompressedSize)
      *FinalUncompressedSize = CompressedBufferSize - 1;

    return ERR_NOERROR;
  }

  zlib::uLongf outLen = (zlib::uLongf) UncompressedBufferSize;
  int res = zlib::uncompress(m_Mm,
    (zlib::Bytef *)UncompressedBuffer,
                             &outLen,
                             (zlib::Bytef *)CompressedBuffer,
                             (zlib::uLongf) CompressedBufferSize);
  *FinalUncompressedSize = outLen;
  switch (res)
  {
  case Z_OK:          return ERR_NOERROR;
  case Z_MEM_ERROR:   return ERR_NOMEMORY;
  case Z_BUF_ERROR:   return ERR_INSUFFICIENT_BUFFER;
  case Z_DATA_ERROR:  return ERR_WRONGFORMAT;
  default:            return ERR_NOT_SUCCESS;
  }
}


int CZlibCompressor::Decompress(
  const void* InBuf,
  size_t      InBufLen,
  void*       OutBuf,
  size_t      OutBufLen,
  size_t*     OutSize
  )
{
  return ZlibDeCompressBuffer(m_Mm, OutBuf, OutBufLen, InBuf, InBufLen, OutSize);
}

} // namespace UFSD

#endif      // UFSD_ZLIB
