// <copyright file="cipherfactory.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#include "cipherfactory.hpp"
#include "../hash/hash.hpp"
#include "aescipher.hpp"
#include <api/cipher.hpp>
#include <api/crypt.hpp>
#include <api/assert.hpp>
#include <api/errors.hpp>

#include <openssl/hmac.h>

#define REPORT_ERROR(log, e)  { if(log) (log)->Error((e), __FILE__, __LINE__); }

namespace cipher {

int CCipherFactory::CreateCipherProvider(
    unsigned int        Method,                       //I_CIPHER_ XXX
    api::ICipher      **ppICipher)
{
    int err = ERR_NOERROR;

    if (!ppICipher)
    {
        err = ERR_BADPARAMS;
        REPORT_ERROR(m_pLog, err);
        return err;
    }

    *ppICipher = new(std::nothrow) COpenSSLCipher(Method);
    if (!*ppICipher)
    {
        err = ERR_NOMEMORY;
        REPORT_ERROR(m_pLog, err);
        return err;
    }

    return err;
}


int CCipherFactory::CreateHashProvider(
    unsigned int        Method,                       //I_CIPHER_ XXX
    api::IHash        **ppIHash)
{
    int err = ERR_NOERROR;
    if (!m_pHashFactory)
    {
        err = hash::CreateHashFactory(&m_pHashFactory);
    }
    if (ERR_NOERROR == err)
    {
        err = m_pHashFactory->CreateProvider(Method, ppIHash);
    }
    return err;
}

int CCipherFactory::Hmac(
    unsigned int         Method,                      //I_CIPHER_ XXX
    const unsigned char *key,
    size_t               key_len,
    const unsigned char *data,
    size_t               data_len,
    unsigned char *mac)
{
    unsigned char *pDigest = NULL;
    switch (Method)
    {
    case I_CIPHER_SHA256:
        pDigest = HMAC(EVP_sha256(), key, key_len, data, data_len, mac, NULL);
        break;

    case I_CIPHER_AES128:
    case I_CIPHER_AES192:
    case I_CIPHER_AES256:
    case I_CIPHER_AES_XTS:
    case I_CIPHER_HMAC_SHA256:
        return ERR_NOTIMPLEMENTED;

    default:
        return ERR_BADPARAMS;
    }
    return pDigest ? ERR_NOERROR : ERR_NOT_SUCCESS;
}

// create provider factory
int CreateCipherFactory(
    api::ILog *pLog,
    api::ICipherFactory **ppCipherIo)
{
    int e = ERR_NOERROR;

    if (!pLog || !ppCipherIo)
    {
        e = ERR_BADPARAMS;
        REPORT_ERROR(pLog, e)
    }

    CCipherFactory *pCipherIo = 0;
    if (BASE_SUCCESS(e))
    {
        pCipherIo = new(std::nothrow) CCipherFactory(pLog);
        if (!pCipherIo)
        {
            e = ERR_NOMEMORY;
            REPORT_ERROR(pLog, e)
        }
    }

    if (BASE_SUCCESS(e))
    {
        *ppCipherIo = pCipherIo;
    }
    else if (pCipherIo)
    {
        pCipherIo->Destroy();
    }

    return e;
}

} // namespace cipher

