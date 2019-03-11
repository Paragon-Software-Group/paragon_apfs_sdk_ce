// <copyright file="crypt.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __CRYPT_H__
#define __CRYPT_H__

#pragma once

#include <api/types.hpp>

namespace api {


///////////////////////////////////////////////
//Crypt algorithms

#define I_CRYPT_NONE            0                       /* without cryption */

#define I_CRYPT_OLHA            1                       /* crypt Olha */

#define I_CRYPT_AES128          2                       /* crypt AES 128 */
#define I_CRYPT_AES192          3                       /* crypt AES 192 */
#define I_CRYPT_AES256          4                       /* crypt AES 256 */

#define I_CRYPT_MASK            0xffff

#define I_CRYPT_USE_IV          0x10000

///////////////////////////////////////////////
#define MIN_KEY_BLOB_LENGTH     100

///////////////////////////////////////////////
//
//   Cryptions
//
///////////////////////////////////////////////

// provider
class BASE_ABSTRACT_CLASS ICrypt
{
public:
  virtual void          Destroy() = 0;

  //Generate new operation key
  virtual int           GenKey(
    const void*             pPwd,                         //Password
    unsigned int            PwdLen,
    unsigned int            KeyLen = 0,                   //0-default key len (minimal)
    unsigned int            Rnd = 0                       //initial random number, 0-not changed
  ) = 0;

  //Change password
  virtual int           ChangePwd(
    const void*             pPwd,                         //Password
    unsigned int            PwdLen
  ) = 0;

  //Check and set operation key
  virtual int           SetBlobKey(
    const void*             pPwd,                         //Password
    unsigned int            PwdLen,
    const void*             pBlobKeyBuff,                 //Encrypted key
    unsigned int            BlobKeyLen
  ) = 0;

  //Get Blob operation key
  virtual int           GetBlobKey(
    void*                   pBlobKeyBuff,                 //Returned encrypted key
    unsigned int            BuffLen,
    unsigned int*           pBlobKeyLen
  ) = 0;

  virtual int           Encrypt(
    void*                   pBuff,
    unsigned int            BuffLen,
    unsigned int*           pDataLen                      //crypted and encripted size can be differ
  ) = 0;

  virtual int           Decrypt(
    void*                   pBuff,
    unsigned int            BuffLen,
    unsigned int*           pDataLen                      //crypted and encripted size can be differ
  ) = 0;

  //Change IV
  virtual int           SetIV(
    const void*             pIV,                          //IV
    unsigned int            IVLen
  ) = 0;

///////////////////////////////////////////////
  int                   MakeBlobKey(
    const void*             pPwd,                         //Password
    unsigned int            PwdLen,
    void*                   pBlobKeyBuff,                 //Encrypted key
    unsigned int            BlobKeyLen
  )
  {
    int rc = GenKey(pPwd, PwdLen, BlobKeyLen);
    if(0 != rc)
      return rc;

    return GetBlobKey(pBlobKeyBuff, BlobKeyLen, NULL);
  }

  int                   CheckBlobKey(
    const void*             pPwd,                         //Password
    unsigned int            PwdLen,
    const void*             pBlobKeyBuff,                 //Encrypted key
    unsigned int            BlobKeyLen
  )
  {
    return SetBlobKey(pPwd, PwdLen, pBlobKeyBuff, BlobKeyLen);
  }
};


// factory
class BASE_ABSTRACT_CLASS ICryptFactory
{
public:

  virtual void          Destroy() = 0;

  virtual int           CreateProvider(
    unsigned int            Method,                       //I_CRYPT_XXX
    ICrypt**                ppICrypt
  ) = 0;

  virtual void          GetRandomNumber(
    void*                   Buff,
    size_t                  BuffLen,
    size_t                  Seed = 0
  ) = 0;
};

} // namespace api

#endif //__CRYPT_H__
