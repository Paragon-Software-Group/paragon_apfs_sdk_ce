// <copyright file="cipher.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __CIPHER_H__
#define __CIPHER_H__

#pragma once

#include <api/types.hpp>

namespace api {

#define I_CIPHER_AES128         1
#define I_CIPHER_AES192         2
#define I_CIPHER_AES256         3
#define I_CIPHER_AES_XTS        4

#define I_CIPHER_SHA256         5
#define I_CIPHER_HMAC_SHA256    6

#define I_CIPHER_AES128_CTR     7
#define I_CIPHER_AES192_CTR     8
#define I_CIPHER_AES256_CTR     9

#define I_CIPHER_CRC32          10
#define I_CIPHER_CRC32C         11

class IHash;

class BASE_ABSTRACT_CLASS ICipher
{
public:
  virtual void      Destroy() = 0;
  virtual int       SetKey(const void* Key, unsigned int KeyLen) = 0;
  virtual int       Encrypt(const void* InBuff, unsigned int InSize, void* OutBuff, unsigned int* OutSize = NULL, void* IV = NULL) = 0;
  virtual int       Decrypt(const void* InBuff, unsigned int InSize, void* OutBuff, unsigned int* OutSize = NULL, void* IV = NULL) = 0;
};

class BASE_ABSTRACT_CLASS ICipherFactory
{
public:
  virtual void      Destroy() = 0;
  virtual int       CreateCipherProvider(
    unsigned int        Method,                       //I_CIPHER_ XXX
    ICipher**           ppICipher
  ) = 0;
  virtual int       CreateHashProvider(
    unsigned int        Method,                       //I_CIPHER_ XXX
    IHash**             ppIHash
  ) = 0;

  virtual int Hmac(
    unsigned int         Method,                      //I_CIPHER_ XXX
    const unsigned char* key,
    size_t               key_len,
    const unsigned char* data,
    size_t               data_len,
    unsigned char*       mac
  ) = 0;
};


};

#endif
