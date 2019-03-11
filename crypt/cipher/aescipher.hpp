// <copyright file="aescipher.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#pragma once

#include "aescipher.hpp"
#include <api/cipher.hpp>
#include <api/memory_mgm.hpp>
#include <memory>

namespace cipher {

class COpenSSLCipher : public api::ICipher
{
    int                                 m_method;
    unsigned int                        m_keyLen;
    unsigned char*                      m_pKey;

    virtual ~COpenSSLCipher()
    {
        if ( m_pKey )
            delete m_pKey;
    }

    int CryptInternal(const void *pInBuff, unsigned int InSize, void *pOutBuff, unsigned int *pOutSize, void *pIV, int enc) const;
public:
    COpenSSLCipher(int method) : m_method(method), m_keyLen(0), m_pKey(NULL)
    {}

    COpenSSLCipher() : m_method(0), m_keyLen(0), m_pKey(NULL)
    {}

    virtual void Destroy()
    {
        delete this;
    }

    virtual int SetKey(const void *pKey, unsigned int KeyLen);
    virtual int Encrypt(const void *pInBuff, unsigned int InSize, void *pOutBuff, unsigned int *pOutSize, void *pIV);
    virtual int Decrypt(const void *pInBuff, unsigned int InSize, void *pOutBuff, unsigned int *pOutSize, void *pIV);
};

} // namespace cipher
