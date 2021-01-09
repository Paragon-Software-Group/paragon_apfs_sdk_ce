// <copyright file="apfsencryption.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
// Revision History:
//
//     15-June-2018  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331632 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uavl.h"
#include "../h/uerrors.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixinode.h"

#include "apfs_struct.h"
#include "apfssuper.h"
#include "apfs.h"
#include "apfsvolsb.h"

namespace UFSD
{

namespace apfs
{

/////////////////////////////////////////////////////////////////////////////
//Class for parsing disk blob structures to in-memory structures
class CApfsBlobParser : public UMemBased<CApfsBlobParser>
{
public:
  CApfsBlobParser(api::IBaseMemoryManager* Mm, api::IBaseLog* pLog)
    : UMemBased<CApfsBlobParser>(Mm)
    , m_ptr(NULL)
    , m_end(NULL)
    , m_pLog(pLog)
  {}

  void SetKey(void* key, unsigned short key_len)
  {
    m_ptr = reinterpret_cast<apfs_blob_entry*>(key);
    m_end = Add2Ptr(key, key_len);
  }

  //Parse disk structures to in-memory structures
  bool ParseBlobHeader(OUT apfs_blob_header_t* vek_header);
  bool ParseVekBlob(OUT apfs_vek* blob_data);
  bool ParseKekBlob(OUT apfs_kek* blob_data);

private:
  apfs_blob_entry *m_ptr;
  const unsigned char *m_end;
  api::IBaseLog* m_pLog;

  //check tag, len, and get bytes array
  bool GetBytes(unsigned char expected_tag, void* data, unsigned char len);
  //check tag, len, and convert bytes array to UINT64
  bool GetUINT64(unsigned char expected_tag, void* res);

  //Get len of block_header or block_data section
  static size_t GetBlobEntryLen(IN apfs_blob_entry* entry, OUT size_t* BlobEntrySize);

  //Parse tag and len of Blob_header and blob_data
  bool ParseSectionHeader(unsigned char expected_tag);
};


/////////////////////////////////////////////////////////////////////////////
int CApfsSuperBlock::LoadEncryptionKeys(
    IN PreInitParams* params,
    IN size_t*        Flags
    )
{
  if (!params->PwdList)
  {
    if (Flags)
      SetFlag(*Flags, UFSD_FLAGS_ENCRYPTED_VOLUMES);
#ifdef UFSD_DRIVER_LINUX
    UFSDTracek((m_pFs->m_Sb, "Encrypted volume detected, but its password isn't specified. To mount, use the option -o passX or -o passfile"));
#else
    return ERR_NOERROR;
#endif
  }

  m_Cf = params->Cf;
  if (m_Cf == NULL)
  {
    ULOG_ERROR((GetLog(), ERR_BADPARAMS, "Cipher factory isn't specified"));
#ifdef UFSD_DRIVER_LINUX
    return ERR_BADPARAMS;
#else
    return ERR_NOERROR;
#endif
  }

  ULOG_TRACE((GetLog(), "Start loading keybag..."));

  //Read keys keybag
  size_t Volumes = 0;
  int Status;
  size_t KeyBagCount = static_cast<size_t>(m_pCSB->sb_keybag_count);
  apfs_keybag *pKeyBag = reinterpret_cast<apfs_keybag*>(Malloc2(KeyBagCount << m_Log2OfCluster));
  CHECK_PTR(pKeyBag);

  CHECK_CALL_EXIT(ReadEncryptedBlocks(m_pCSB->sb_keybag_block, pKeyBag, KeyBagCount, m_pCSB->sb_uuid));
  CHECK_CALL_EXIT(CheckKeyBag(pKeyBag, KeyBagCount, APFS_TYPE_KEYBAG));

  assert(pKeyBag->keybag_hdr.version == KEYBAG_VERSION);

  //Search vek_blob's and recs pointers, initialize volumes descriptors
  for (unsigned int i = 0; i < m_MountedVolumesCount; i++)
  {
    if (!m_pVolSuper[i].IsEncrypted())
    {
      ++Volumes;
      continue;
    }
    m_pVolSuper[i].m_bReadOnly = true;

    apfs_keys* pKey = &pKeyBag->keys;
    apfs_keys *pFoundVolBlob = NULL, *pFoundRecsBagPtr = NULL;

    for (unsigned short k = 0; k < pKeyBag->keybag_hdr.number_of_keys; ++k)
    {
      if (Memcmp2(pKey->hdr.uuid, m_pVolSuper[i].GetUUID(), sizeof(pKey->hdr.uuid)) == 0)
      {
        if (pKey->hdr.type == APFS_ENCRKEY_TYPE_VEK_BLOB)
          pFoundVolBlob = pKey;
        else if (pKey->hdr.type == APFS_ENCRKEY_TYPE_RECS_BAG_EXTENT)
          pFoundRecsBagPtr = pKey;

        if (pFoundVolBlob && pFoundRecsBagPtr)
          break;
      }

      unsigned short len = (sizeof(apfs_key_hdr) + pKey->hdr.length + APFS_ENCRKEY_LEN_MASK) & ~APFS_ENCRKEY_LEN_MASK;
      pKey = reinterpret_cast<apfs_keys*>(Add2Ptr(pKey, len));
    }

    if (i >= params->PwdSize || !params->PwdList || params->PwdList[i] == NULL)
    {
      UFSDTracek((m_pFs->m_Sb, "Password for volume %u isn't specified", i));
      if (Flags)
        SetFlag(*Flags, UFSD_FLAGS_ENCRYPTED_VOLUMES);
      continue;
    }

    if (pFoundVolBlob && pFoundRecsBagPtr)
    {
      Status = m_pVolSuper[i].InitEncryption(pFoundVolBlob, pFoundRecsBagPtr, (const unsigned char*)params->PwdList[i],
        params->PwdList[i][0] != '\0' ? m_Strings->strlen(api::StrUTF8, params->PwdList[i]) : 0);

      if (Status == ERR_BADPARAMS)
      {
        UFSDTracek((m_pFs->m_Sb, "Wrong password for the volume %u", i));
        Status = ERR_NOERROR;
        if (Flags)
          SetFlag(*Flags, UFSD_FLAGS_BAD_PASSWORD | UFSD_FLAGS_ENCRYPTED_VOLUMES);
        continue;
      }

      if (Status != ERR_NOERROR)
      {
        ULOG_ERROR((GetLog(), Status, "Can't read encryptions structures for the volume %u", i));
        break;
      }
      ++Volumes;
      ULOG_TRACE((GetLog(), "Volume %u: encryption initialized", i));
    }
    else
    {
      //Volume is encrypted but can't find structures for decrypt
      ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "%s not found for volume %u", pFoundVolBlob ? "Vek blob data" : "Recs bag", i));
      Status = ERR_NOFSINTEGRITY;
      break;
    }
  }

Exit:
  if (UFSD_SUCCESS(Status) && Volumes == 0)
    Status = ERR_FSUNKNOWN;
  Free2(pKeyBag);
  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsSuperBlock::ReadEncryptedBlocks(
  IN  UINT64      StartBlock,
  OUT void*       pBuffer,
  IN  size_t      NumBlocks,
  IN  const void* uuid
  ) const
{
  if (m_Cf == NULL)
    return ERR_READFILE;

  CHECK_CALL(ReadBlocks(StartBlock, pBuffer, NumBlocks));

  api::ICipher* pCipher = NULL;
  CHECK_CALL(m_Cf->CreateCipherProvider(I_CIPHER_AES_XTS, &pCipher));
  CHECK_PTR(pCipher);

  unsigned char key[APFS_ENCRYPT_KEY_SIZE];
  int Status;

  Memcpy2(key, uuid, APFS_ENCRYPT_KEY_SIZE / 2);
  Memcpy2(Add2Ptr(key, APFS_ENCRYPT_KEY_SIZE / 2), uuid, APFS_ENCRYPT_KEY_SIZE / 2);

  if (UFSD_SUCCESS(Status = pCipher->SetKey(key, APFS_ENCRYPT_KEY_SIZE)))
    Status = DecryptBlocks(pCipher, StartBlock, pBuffer, NumBlocks);

  pCipher->Destroy();

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsSuperBlock::DecryptSectors(
    IN  api::ICipher* pAes,
    IN  UINT64        StartSector,
    OUT void*         pBuffer,
    IN  size_t        NumSectors
    ) const
{
  size_t size = NumSectors << APFS_ENCRYPT_PORTION_LOG;
  int Status = ERR_NOERROR;

  for (size_t k = 0; k < size; k += APFS_ENCRYPT_PORTION)
  {
    unsigned char iv[16] = { 0 };
    Memcpy2(iv, &StartSector, sizeof(UINT64));

    if (!UFSD_SUCCESS(Status = pAes->Decrypt(Add2Ptr(pBuffer, k), APFS_ENCRYPT_PORTION, Add2Ptr(pBuffer, k), NULL, iv)))
      break;

    ++StartSector;
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsSuperBlock::DecryptBlocks(
    IN  unsigned char Index,
    IN  UINT64        StartBlock,
    OUT void*         pBuffer,
    IN  size_t        NumBlocks
    ) const
{
  if (Index < m_MountedVolumesCount && m_pVolSuper[Index].m_pAES)
    CHECK_CALL(DecryptBlocks(m_pVolSuper[Index].m_pAES, StartBlock, pBuffer, NumBlocks));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsSuperBlock::CheckKeyBag(
    IN apfs_keybag* pKeyBag,
    IN size_t       SizeInBlocks,
    IN unsigned int BlockType
    ) const
{
  if (!CheckFSum(pKeyBag, SizeInBlocks << m_Log2OfCluster))
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Wrong checksum in KeyBag block"));
    return ERR_NOFSINTEGRITY;
  }

  if (pKeyBag->header.block_type != BlockType)
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Unknown block type %#x in the KeyBag block", pKeyBag->header.block_type));
    ULOG_DUMP((GetLog(), LOG_LEVEL_ERROR, pKeyBag, sizeof(apfs_keybag)));
    return ERR_NOFSINTEGRITY;
  }

  if (pKeyBag->header.checkpoint_id > GetCheckPointId())
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Wrong checkpoint %#" PLL "x in the KeyBag block", pKeyBag->header.checkpoint_id));
    return ERR_NOFSINTEGRITY;
  }

  if (pKeyBag->header.id != 0 || pKeyBag->header.block_subtype != 0)
  {
    ULOG_WARNING((GetLog(), "Wrong KeyBag header"));
    return ERR_FSUNKNOWN;
  }
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::InitEncryption(apfs_keys* pVekBlobKey, apfs_keys* pRecsBagPtrKey, const unsigned char* password, size_t PassLen)
{
  //Read rec's bag
  apfs_keybag *pRecsBag;
  int Status;
  apfs_blob_header_t vek_header, kek_header;
  apfs_kek kek_data;
  apfs_vek vek_data;
  CApfsBlobParser parser(m_Mm, GetLog());
  apfs_keys* pKekBlobKey;
  unsigned char vek[APFS_ENCRYPT_KEY_SIZE];

  m_bEncryptionKeyFound = false;

  //Get Recs bag
  size_t RecsBagCount = static_cast<size_t>(pRecsBagPtrKey->key_extent.count);
  CHECK_PTR(pRecsBag = reinterpret_cast<apfs_keybag*>(Malloc2(RecsBagCount << m_pSuper->m_Log2OfCluster)));
  CHECK_CALL_EXIT(m_pSuper->ReadEncryptedBlocks(pRecsBagPtrKey->key_extent.block, pRecsBag, RecsBagCount, pRecsBagPtrKey->hdr.uuid));
  CHECK_CALL_EXIT(m_pSuper->CheckKeyBag(pRecsBag, RecsBagCount, APFS_TYPE_KEYRECS));

  //Get Kek blob key
  pKekBlobKey = &pRecsBag->keys;
  for (unsigned short k = 0; k < pRecsBag->keybag_hdr.number_of_keys; ++k)
  {
    assert(Memcmp2(pKekBlobKey->hdr.uuid, GetUUID(), sizeof(pKekBlobKey->hdr.uuid)) == 0);

    if (pKekBlobKey->hdr.type == APFS_ENCRKEY_TYPE_KEK_BLOB)
      break;

    unsigned short len = (sizeof(apfs_key_hdr) + pKekBlobKey->hdr.length + APFS_ENCRKEY_LEN_MASK) & ~APFS_ENCRKEY_LEN_MASK;
    pKekBlobKey = reinterpret_cast<apfs_keys*>((char*)pKekBlobKey + len);
  }
  if (pKekBlobKey == NULL)
  {
    Free2(pRecsBag);
    return ERR_NOTFOUND;
  }

  //Parse pVekBlobKey to in-memory structures vek_header, vek_data
  parser.SetKey(pVekBlobKey->blob.blob, pVekBlobKey->blob.hdr.length);
  if (!parser.ParseBlobHeader(&vek_header) || !parser.ParseVekBlob(&vek_data))
  {
    ULOG_DUMP((GetLog(), LOG_LEVEL_ERROR, pVekBlobKey->blob.blob, pVekBlobKey->blob.hdr.length));
    Free2(pRecsBag);
    return ERR_NOFSINTEGRITY;
  }

  //Parse pKekBlobKey to in-memory structures kek_header, kek_data
  parser.SetKey(pKekBlobKey->blob.blob, pKekBlobKey->blob.hdr.length);
  if (!parser.ParseBlobHeader(&kek_header) || !parser.ParseKekBlob(&kek_data))
  {
    ULOG_DUMP((GetLog(), LOG_LEVEL_ERROR, pKekBlobKey->blob.blob, pKekBlobKey->blob.hdr.length));
    Free2(pRecsBag);
    return ERR_NOFSINTEGRITY;
  }

Exit:
  Free2(pRecsBag);
  CHECK_STATUS(Status);

  //Calculate vek key by vek_blob and kek_blob
  CHECK_CALL_SILENT(CalculateVek(&vek_data, &kek_data, password, PassLen, vek));

  if (!m_pAES)
    CHECK_CALL(m_pSuper->m_Cf->CreateCipherProvider(I_CIPHER_AES_XTS, &m_pAES));
  CHECK_CALL(m_pAES->SetKey(vek, sizeof(vek)));
  m_bEncryptionKeyFound = true;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
//    PBKDF2_HMAC_SHA256
/////////////////////////////////////////////////////////////////////////////
static int GetDerivedKey(
  IN  api::IBaseMemoryManager*  m_Mm,
  IN  api::IBaseLog*            Log,
  IN  api::ICipherFactory*      Cf,
  IN  const unsigned char*      Pass,
  IN  size_t                    PassSize,
  IN  apfs_kek*                 kek,
  OUT unsigned char*            DerKey,
  IN  size_t                    DerKeySize
  )
{
#ifdef STATIC_MEMORY_MANAGER
  UNREFERENCED_PARAMETER( m_Mm );
#endif
  const size_t sLen = sizeof(kek->salt);                      // 0x10
  const size_t hLen = sizeof(((apfs_blob_header_t*)0)->hmac); // 0x20;

  unsigned char salt[sLen + 4];
  Memcpy2(salt, kek->salt, sLen);

  unsigned int cnt = 1;
  size_t sz = 0;

  unsigned char mac[hLen];
  unsigned char key[hLen];
  UINT64* k = reinterpret_cast<UINT64*>(key);
  UINT64* m = reinterpret_cast<UINT64*>(mac);

  while (sz < DerKeySize)
  {
    *(unsigned int*)(salt + sLen) = GetSwapped(cnt++);

    CHECK_CALL_LOG(Cf->Hmac(I_CIPHER_SHA256, Pass, PassSize, salt, sizeof(salt), mac), Log);
    Memcpy2(key, mac, sizeof(key));

    for (UINT64 i = 1; i < kek->iterations; i++)
    {
      CHECK_CALL_LOG(Cf->Hmac(I_CIPHER_SHA256, Pass, PassSize, mac, sizeof(mac), mac), Log);
      // due to hLen % 8 == 0
      *k       ^= *m;
      *(k + 1) ^= *(m + 1);
      *(k + 2) ^= *(m + 2);
      *(k + 3) ^= *(m + 3);
    }

    Memcpy2(DerKey, key, hLen);

    DerKey += hLen;
    sz += hLen;
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsVolumeSb::CalculateVek(
  IN  apfs_vek*             VekData,
  IN  apfs_kek*             KekData,
  IN  const unsigned char*  Pass,
  IN  size_t                PassLen,
  OUT unsigned char*        Vek
  ) const
{
  unsigned char dk[APFS_ENCRYPT_KEY_SIZE];
  unsigned char kek[APFS_ENCRYPT_KEY_SIZE];
  UINT64 iv = 0;

  //Calculate kek
  CHECK_CALL(GetDerivedKey(m_Mm, GetLog(), m_pSuper->m_Cf, Pass, PassLen, KekData, dk, sizeof(dk)));

  if (KekData->tag82.unk82_00 != APFS_BLOB_AES256 && KekData->tag82.unk82_00 != APFS_BLOB_UNK82_10 && KekData->tag82.unk82_00 != APFS_BLOB_AES128)
  {
    ULOG_ERROR((GetLog(), ERR_NOTIMPLEMENTED, "Wrong value of kek_data->unk_82.unk82_00=0x%x", KekData->tag82.unk82_00));
    return ERR_NOTIMPLEMENTED;
  }

  CHECK_CALL(KeyUnwrap(KekData->wrapped_kek, dk, KekData->tag82.unk82_00 == APFS_BLOB_AES128 ? I_CIPHER_AES128 : I_CIPHER_AES256, kek, iv));

  if (iv != RFC_3394_DEFAULT_IV)
  {
    //It's very possible that password is wrong
    ULOG_WARNING((GetLog(), "Wrong initialization vector value for kek 0x%" PLL "x, should be 0x%" PLL "x", iv, RFC_3394_DEFAULT_IV));
    return ERR_BADPARAMS;
  }

  //Calculate vek
  if (VekData->tag82.unk82_00 == APFS_BLOB_AES256)
  {
    // AES-256. This method is used for wrapping the whole XTS-AES key, and applies to non-FileVault encrypted APFS volumes.
    CHECK_CALL(KeyUnwrap(VekData->wrapped_vek, kek, I_CIPHER_AES256, Vek, iv));
  }
  else if (VekData->tag82.unk82_00 == APFS_BLOB_AES128)
  {
    // AES-128. This method is used for FileVault and CoreStorage encrypted volumes that have been converted to APFS.
    CHECK_CALL(KeyUnwrap(VekData->wrapped_vek, kek, I_CIPHER_AES128, Vek, iv));

    unsigned char sha_result[APFS_ENCRYPT_KEY_SIZE];
    api::IHash* pHash = NULL;

    CHECK_CALL(m_pSuper->m_Cf->CreateHashProvider(I_CIPHER_SHA256, &pHash));

    int Status;

    if (UFSD_SUCCESS(Status = pHash->ReInit())
     && UFSD_SUCCESS(Status = pHash->AddData(Vek, APFS_ENCRYPT_KEY_SIZE / 2))
     && UFSD_SUCCESS(Status = pHash->AddData(VekData->uuid, APFS_ENCRYPT_KEY_SIZE / 2)))
    {
      Status = pHash->GetHash(sha_result);
    }

    pHash->Destroy();
    CHECK_STATUS(Status);

    Memcpy2(Add2Ptr(Vek, APFS_ENCRYPT_KEY_SIZE / 2), sha_result, APFS_ENCRYPT_KEY_SIZE / 2);
  }
  else
  {
    ULOG_ERROR((GetLog(), ERR_NOTIMPLEMENTED, "Wrong value of vek_data->unk_82.unk82_00=0x%x", VekData->tag82.unk82_00));
    return ERR_NOTIMPLEMENTED;
  }

  if (iv != RFC_3394_DEFAULT_IV)
  {
    //It's very possible that password is wrong
    ULOG_WARNING((GetLog(), "Wrong initialization vector value for vek 0x%" PLL "x, should be 0x%" PLL "x", iv, RFC_3394_DEFAULT_IV));
    return ERR_BADPARAMS;
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsVolumeSb::KeyUnwrap(
  IN  const void* InBuf,
  IN  const void* Key,
  IN  unsigned    Cipher,
  OUT void*       OutBuf,
  OUT UINT64&     IV
  ) const
{
  api::ICipher* pCipher = NULL;
  CHECK_CALL(m_pSuper->m_Cf->CreateCipherProvider(Cipher, &pCipher));

  unsigned int size = Cipher == I_CIPHER_AES128 ? 16 : 32;
  int el = size / sizeof(UINT64);
  UINT64 k = 6 * el;
  const UINT64* in = reinterpret_cast<const UINT64*>(InBuf);

  IV = in[0];

  UINT64 out[6];
  Memcpy2(out, in + 1, size);

  int Status = pCipher->SetKey(Key, size);

  while (UFSD_SUCCESS(Status) && k)
  {
    for (int i = el - 1; i >= 0; i--)
    {
      UINT64 u[2] = { IV ^ GetSwapped(k), out[i] };

      if (!UFSD_SUCCESS(Status = pCipher->Decrypt(u, sizeof(u), u)))
        break;

      IV     = u[0];
      out[i] = u[1];
      k--;

      assert(k != 0 || i == 0);
    }
  }

  Memcpy2(OutBuf, out, size);

  pCipher->Destroy();

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsBlobParser::ParseBlobHeader(OUT apfs_blob_header_t* h)
{
  return ParseSectionHeader(APFS_BLOB_HEADER_TAG)
      && GetUINT64(APFS_BLOB_TAG_80, &h->tag80)
      && GetBytes(APFS_BLOB_HEADER_HMAC_TAG, h->hmac, sizeof(h->hmac))
      && GetBytes(APFS_BLOB_HEADER_SALT_TAG, h->salt, sizeof(h->salt));
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsBlobParser::ParseVekBlob(OUT apfs_vek* vek)
{
  return ParseSectionHeader(APFS_BLOB_DATA_TAG)
      && GetUINT64(APFS_BLOB_TAG_80, &vek->tag80)
      && GetBytes(APFS_BLOB_UUID_TAG, vek->uuid, sizeof(vek->uuid))
      && GetBytes(APFS_BLOB_TAG_82, &vek->tag82, sizeof(vek->tag82))
      && GetBytes(APFS_BLOB_WRAPPED_TAG, vek->wrapped_vek, sizeof(vek->wrapped_vek));
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsBlobParser::ParseKekBlob(OUT apfs_kek* kek)
{
  return ParseSectionHeader(APFS_BLOB_DATA_TAG)
      && GetUINT64(APFS_BLOB_TAG_80, &kek->tag80)
      && GetBytes(APFS_BLOB_UUID_TAG, kek->uuid, sizeof(kek->uuid))
      && GetBytes(APFS_BLOB_TAG_82, &kek->tag82, sizeof(kek->tag82))
      && GetBytes(APFS_BLOB_WRAPPED_TAG, kek->wrapped_kek, sizeof(kek->wrapped_kek))
      && GetUINT64(APFS_BLOB_ITERATIONS_TAG, &kek->iterations)
      && GetBytes(APFS_BLOB_SALT_TAG, kek->salt, sizeof(kek->salt));
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsBlobParser::GetBytes(unsigned char expected_tag, void* data, unsigned char len)
{
  if (m_ptr->tag != expected_tag || m_ptr->len != len || PtrOffset(m_ptr, m_end) < len + sizeof(apfs_blob_entry))
  {
    ULOG_ERROR((m_pLog, ERR_NOFSINTEGRITY, "Error parsing record with tag=0x%x", expected_tag));
    return false;
  }

  Memcpy2(data, Add2Ptr(m_ptr, sizeof(apfs_blob_entry)), len);
  m_ptr = reinterpret_cast<apfs_blob_entry*>(Add2Ptr(m_ptr, sizeof(apfs_blob_entry) + len));

  return true;
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsBlobParser::GetUINT64(unsigned char expected_tag, void* res)
{
  if (m_ptr->tag != expected_tag || PtrOffset(m_ptr, m_end) < m_ptr->len + sizeof(apfs_blob_entry))
  {
    ULOG_ERROR((m_pLog, ERR_NOFSINTEGRITY, "Error parsing record with tag=0x%x", expected_tag));
    return false;
  }

  UINT64 tmp = 0;
  unsigned char* b = Add2Ptr(m_ptr, sizeof(apfs_blob_entry));
  for (unsigned char i = 0; i < m_ptr->len; i++)
    tmp = (tmp << 8) | *b++;
  Memcpy2(res, &tmp, sizeof(UINT64));
  m_ptr = reinterpret_cast<apfs_blob_entry*>(Add2Ptr(m_ptr, sizeof(apfs_blob_entry) + m_ptr->len));

  return true;
}


/////////////////////////////////////////////////////////////////////////////
size_t
CApfsBlobParser::GetBlobEntryLen(IN apfs_blob_entry* entry, OUT size_t* BlobEntrySize)
{
  if (entry->len > APFS_ENCRYPTION_RECORD_LEN_MASK)
  {
    //blob entry len stored in several bytes
    size_t len = 0;
    unsigned short num_bytes = entry->len & APFS_ENCRYPTION_RECORD_LEN_MASK;
    unsigned char* b = Add2Ptr(entry, sizeof(apfs_blob_entry));

    for (unsigned k = 0; k < num_bytes; k++)
      len = (len << 8) | *b++;

    *BlobEntrySize = sizeof(apfs_blob_entry) + num_bytes;
    return len;
  }

  //blob entry len stored in 1 byte
  *BlobEntrySize = sizeof(apfs_blob_entry);
  return entry->len;
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsBlobParser::ParseSectionHeader(unsigned char expected_tag)
{
  size_t BlobEntrySize = 0;
  size_t len = GetBlobEntryLen(m_ptr, &BlobEntrySize);
  if (m_ptr->tag != expected_tag || PtrOffset(m_ptr, m_end) < len + BlobEntrySize)
  {
    ULOG_ERROR((m_pLog, ERR_NOFSINTEGRITY, "Error parsing section header with tag=0x%x", expected_tag));
    return false;
  }
  m_ptr = reinterpret_cast<apfs_blob_entry*>(Add2Ptr(m_ptr, BlobEntrySize));
  return true;
}

} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
