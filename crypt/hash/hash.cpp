// <copyright file="hash.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#include <memory>
#include "hash.hpp"
#include <api/errors.hpp>
#include <api/cipher.hpp>

#include <openssl/evp.h>
#include <openssl/hmac.h>

namespace hash
{

// base provider class
class Sha256Hash : public api::IHash
{
    struct CtxtDeleteter
    {
        void operator()(EVP_MD_CTX *pCtxt)
        {
            if (pCtxt)
            {
                EVP_MD_CTX_destroy(pCtxt);
            }
        }
    };

    EVP_MD_CTX* m_pCtxt;
    size_t m_keyLen;

    virtual ~Sha256Hash()
    {
        CtxtDeleteter a;
        a(m_pCtxt);
    }
public:
    Sha256Hash() : m_pCtxt(NULL), m_keyLen(0)
    {
        ReInit();
    }

    /// Calculates hash on the spot.
    /// Is equivalent to ReInit(); AddData(Buff, Len); GetHash(HashValue);
    int CalcHash(
        const void  *pBuff
        , size_t    Len
        , void      *pHashValue
    )
    {
        int err = ReInit();
        if (!BASE_SUCCESS(err))
            return err;

        err = AddData(pBuff, Len);
        if (!BASE_SUCCESS(err))
            return err;

        err = GetHash(pHashValue);
        return err;
    }

    unsigned int GetMethod() const
    {
        return I_CIPHER_SHA256;
    }

    // destroy
    virtual void Destroy()
    {
        delete this;
    }

    /// Resets state
    int ReInit()
    {
        CtxtDeleteter a;
        a(m_pCtxt);
        m_pCtxt = EVP_MD_CTX_create();
        int err = EVP_DigestInit_ex(m_pCtxt, EVP_sha256(), NULL);
        return err ? ERR_NOERROR : ERR_ENCRYPTION;
    }

    /// Adds data for the subsequent hash calculation
    int AddData(
        const void  *pBuff
        , size_t    Len
    )
    {
        m_keyLen += Len;
        return EVP_DigestUpdate(m_pCtxt, pBuff, Len) ? ERR_NOERROR : ERR_ENCRYPTION;
    }

    /// Returns hash of currently added data
    int GetHash(void *pHashValue)
    {
        return EVP_DigestFinal(
            m_pCtxt
            , static_cast<unsigned char*>(pHashValue)
            , NULL
        ) ? ERR_NOERROR : ERR_ENCRYPTION;
    }

    /// Returns size of hash in bytes
    size_t GetKeySize() const
    {
        return m_keyLen;
    }
};


// Hash factory
class CHashFactory : public api::IHashFactory
{
public:

    // constructor
    CHashFactory() {}

    // destructor
    virtual ~CHashFactory() {}

    // destroy
    virtual void Destroy()
    {
        delete this;
    }

    // create hash provider
    virtual int CreateProvider(
        unsigned int Method
        , api::IHash **ppIHash
    )
    {
        if (I_CIPHER_SHA256 != Method)
        {
            return ERR_NOTIMPLEMENTED;
        }

        int e = ERR_NOERROR;

        if (!ppIHash)
        {
            e = ERR_BADPARAMS;
        }

        Sha256Hash *pHash = new(std::nothrow) Sha256Hash();

        if (BASE_SUCCESS(e))
        {
            if (!pHash)
            {
                e = ERR_NOMEMORY;
            }

            if (BASE_SUCCESS(e))
            {
                *ppIHash = pHash;
            }
        }
        else
        {
            if (pHash)
            {
                pHash->Destroy();
            }
        }

        return e;
    }
};


int CreateHashFactory(
    api::IHashFactory     **ppHashIo
)
{
    int e = ERR_NOERROR;

    if (!ppHashIo)
    {
        e = ERR_BADPARAMS;
    }

    CHashFactory *pHashIo = 0;
    if (BASE_SUCCESS(e))
    {
        pHashIo = new(std::nothrow) CHashFactory();
        if (!pHashIo)
        {
            e = ERR_NOMEMORY;
        }
    }

    if (BASE_SUCCESS(e))
    {
        *ppHashIo = pHashIo;
    }
    else
    {
        if (pHashIo)
        {
            pHashIo->Destroy();
        }
    }

    return e;
}

} // namespace hash
