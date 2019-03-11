// <copyright file="aescipher.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#include "aescipher.hpp"
#include <api/errors.hpp>

#include <openssl/aes.h>
#include <openssl/evp.h>

namespace cipher {
int COpenSSLCipher::SetKey(const void *pKey, unsigned int KeyLen)
{
    m_keyLen = KeyLen;
    m_pKey = new unsigned char[KeyLen];
    if (m_pKey)
    {
        Memcpy2(m_pKey, pKey, KeyLen);
        return ERR_NOERROR;
    }

    return ERR_NOMEMORY;
}

int COpenSSLCipher::CryptInternal(const void *pInBuff, unsigned int InSize, void *pOutBuff, unsigned int *pOutSize, void *pIV, int enc) const
{
    EVP_CIPHER_CTX *pCtx = EVP_CIPHER_CTX_new();

    if (!pCtx)
    {
        return ERR_NOMEMORY;
    }

    int sslErr = 1;

    switch (m_method)
    {
    case I_CIPHER_AES128:
        sslErr = EVP_CipherInit_ex(pCtx, EVP_aes_128_cbc(), NULL, m_pKey, (const unsigned char*)pIV, enc);
        break;

    case I_CIPHER_AES192:
        sslErr = EVP_CipherInit_ex(pCtx, EVP_aes_192_cbc(), NULL, m_pKey, (const unsigned char*)pIV, enc);
        break;

    case I_CIPHER_AES256:
        sslErr = EVP_CipherInit_ex(pCtx, EVP_aes_256_cbc(), NULL, m_pKey, (const unsigned char*)pIV, enc);
        break;

    /*case I_CIPHER_SHA256:
        sslErr = EVP_CipherInit_ex(pCtx, EVP_sha256(), NULL, (const unsigned char*)m_pKey, (const unsigned char*)pIV, enc);
        break;*/
    //I_CIPHER_HMAC_SHA256

    case I_CIPHER_AES_XTS:
        sslErr = EVP_CipherInit_ex(pCtx, EVP_aes_128_xts(), NULL, m_pKey, (const unsigned char*)pIV, enc);
        break;

    default:
        EVP_CIPHER_CTX_free(pCtx);
        return ERR_BADPARAMS;
    }

    if (1 != sslErr)
    {
        EVP_CIPHER_CTX_free(pCtx);
        return ERR_ENCRYPTION;
    }

    int outLen = 0;
    sslErr = EVP_CipherUpdate(pCtx, (unsigned char*)pOutBuff, &outLen, (unsigned char*)pInBuff, InSize);
    if (1 != sslErr)
    {
        EVP_CIPHER_CTX_free(pCtx);
        return ERR_ENCRYPTION;
    }

    int finalLen = 0;
    if (0 != outLen)
    {
        sslErr = EVP_CipherFinal_ex(pCtx, (unsigned char*)pOutBuff + outLen, &finalLen);
    }
    EVP_CIPHER_CTX_free(pCtx);

    if (pOutSize)
    {
        *pOutSize = outLen + finalLen;
    }

    return (1 == sslErr) ? ERR_NOERROR : ERR_ENCRYPTION;
}

int COpenSSLCipher::Encrypt(const void *pInBuff, unsigned int InSize, void *pOutBuff, unsigned int *pOutSize, void *pIV)
{
    return CryptInternal(pInBuff, InSize, pOutBuff, pOutSize, pIV, 1/*encrypt*/);
}

int COpenSSLCipher::Decrypt(const void *pInBuff, unsigned int InSize, void *pOutBuff, unsigned int *pOutSize, void *pIV)
{
    return CryptInternal(pInBuff, InSize, pOutBuff, pOutSize, pIV, 0/*decrypt*/);
}

} // namespace cipher
