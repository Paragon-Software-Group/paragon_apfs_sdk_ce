// <copyright file="cipherfactory.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#pragma once

#include <api/cipher.hpp>
#include <api/hash.hpp>
#include <api/log.hpp>
#include <api/memory_mgm.hpp>

namespace cipher {

// encryption provider
class CCipherFactory : public api::ICipherFactory
{
public:
    // constructor
    CCipherFactory(
        api::ILog* pLog
        )
        : m_pLog(pLog)
        , m_pHashFactory(NULL)
    {
    }

    // destructor
    virtual ~CCipherFactory()
    {
        if (m_pHashFactory)
        {
            m_pHashFactory->Destroy();
            m_pHashFactory = NULL;
        }
    }

    // destroy
    virtual void Destroy()
    {
        delete this;
    }

    virtual int CreateCipherProvider(
        unsigned int        Method,                       //I_CIPHER_ XXX
        api::ICipher      **ppICipher);

    virtual int       CreateHashProvider(
        unsigned int        Method,                       //I_CIPHER_ XXX
        api::IHash        **ppIHash);

    virtual int       Hmac(
        unsigned int         Method,                      //I_CIPHER_ XXX
        const unsigned char *key,
        size_t               key_len,
        const unsigned char *data,
        size_t               data_len,
        unsigned char       *mac
        );

protected:
    api::ILog               *m_pLog;
    api::IHashFactory       *m_pHashFactory;
};

} // namespace cipher
