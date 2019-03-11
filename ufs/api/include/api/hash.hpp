// <copyright file="hash.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __HASH_H__
#define __HASH_H__

#pragma once

#include <api/types.hpp>

namespace api {


#define I_HASH_MD5              1
#define I_HASH_CRC32            2                               // default submethod
#define I_HASH_SHA1             3
#define I_HASH_SHA256           4
#define I_HASH_SHA3             5                               // default submethod
#define I_HASH_MURMUR3          6
#define I_HASH_XXHASH           7
#define I_HASH_CRC32C           8                               //crc32 algorythm, polynom castagnoly

class BASE_ABSTRACT_CLASS IHash
{
public:

  /// Calculates hash on the spot.
  /// Is equivalent to ReInit(); AddData(Buff, Len); GetHash(HashValue);
  virtual int           CalcHash(
    IN  const void*         Buff,
    IN  size_t              Len,
    OUT void*               HashValue
  ) = 0;

  /// Resets state
  virtual int           ReInit() = 0;

  /// Adds data for the subsequent hash calulation
  virtual int           AddData(
    IN  const void*         Buff,
    IN  size_t              Len
  ) = 0;

  /// Returns hash of currently added data
  virtual int           GetHash(
    OUT void*               HashValue
  ) = 0;

  /// Returns size of hash in bytes
  virtual size_t        GetKeySize() const = 0;

  virtual unsigned int  GetMethod() const = 0;

  // destroy
  virtual void Destroy() = 0;
};


class BASE_ABSTRACT_CLASS IHashFactory
{
public:
  /// Creates has providerwith default submethod
  virtual int           CreateProvider(
      IN  unsigned int        Method,      /// Hashing method (like I_HASH_CRC32_STANDARD)
      OUT IHash**             ppIHash
  ) = 0;

  virtual void Destroy() = 0;
};

} // namespace api

#endif //__HASH_H__
