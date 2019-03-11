// <copyright file="apfshash.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
//    Decompose functions and tables based on ICU library
//
// Revision History :
//
//     05-October-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331828 $";
#endif

#include <ufsd.h>

#include "../h/crc32.h"
#include "../h/assert.h"

#ifdef BASE_BIGENDIAN
#include "../h/uswap.h"
#endif
#include "apfshash.h"

namespace UFSD
{

namespace apfs
{

///////////////////////////////////////////////////////////
static unsigned int decomp_idx(unsigned int cp)
{
  if (cp >= 195102)
    return 0;
  return decomp_idx_t2[(decomp_idx_t1[cp >> 6] << 6) + (cp & 63)];
}


///////////////////////////////////////////////////////////
static size_t unicode_decompose(unsigned int pt, unsigned int out[4])
{
  // Special-case: Hangul decomposition
  if (pt >= 0xAC00 && pt < 0xD7A4)
  {
    out[0] = 0x1100 +  (pt - 0xAC00) / 588;
    out[1] = 0x1161 + ((pt - 0xAC00) % 588) / 28;

    if ((pt - 0xAC00) % 28)
    {
      out[2] = 0x11A7 + (pt - 0xAC00) % 28;
      return 3;
    }

    return 2;
  }

  // Otherwise, look up in the decomposition table
  unsigned int decomp_start_idx = decomp_idx(pt);
  if (!decomp_start_idx)
  {
    out[0] = pt;
    return 1;
  }

  size_t length = (decomp_start_idx >> 14) + 1;

  if (length > 4)
  {
    assert(!"Number of code points is too big");
    return 0;
  }

  decomp_start_idx &= (1 << 14) - 1;

  for (size_t i = 0; i < length; i++)
    out[i] = xref[decomp_seq[decomp_start_idx + i]];

  return length;
}


///////////////////////////////////////////////////////////
// utf8ToU32Code
//
//
///////////////////////////////////////////////////////////
int utf8ToU32Code(
    IN     int                    cp,
    IN OUT const unsigned char**  next,
    IN     const unsigned char*   end
    )
{
  const unsigned char* n = *next;
  unsigned char v0 = *n ^ 0x80;

  if ( cp - 0xE1 <= 0xB && n + 1 < end && v0 <= 0x3F )
  {
    unsigned char v1 = n[1] ^ 0x80;
    if ( v1 <= 0x3F )
    {
      *next += 2;
      return v1 | (v0 << 6) | ((cp & 0xF) << 12);
    }
  }

  if ( cp - 0xC2 <= 0x1D && n < end && v0 <= 0x3F )
  {
    *next += 1;
    return v0 | ((cp & 0x1F) << 6);
  }

  unsigned int d;

  if ( cp <= 0xEF )
    d = (cp > 0xBF) + (cp > 0xDF);
  else if ( cp > 0xFD )
    d = 0;
  else
    d = (cp > 0xF7) + (cp > 0xFB) + 3;

  int ret = -1;

  if ( n + d <= end )
  {
    int v = cp & ((1 << (6 - d)) - 1);

    if ( d != 1 )
    {
      if ( d != 2 )
      {
        if ( d != 3 || v0 > 0x3F )
          return -1;

        v = v0 | (v << 6);
        if ( v > 271 )
          return -1;

        ++n;
      }

      if ( v0 > 0x3Fu )
        return -1;

      ++n;

      v = v0 | (v << 6);
      if ( (v & 0xFFE0) == 864 )
        return -1;
    }

    v0 = *n++ ^ 0x80;
    if ( v0 > 0x3Fu )
      return -1;

    ret = (v << 6) | v0;
    if ( static_cast<unsigned>(ret) < utf8_minLegal[d] )
      return -1;
  }

  *next = n;
  return ret;
}


///////////////////////////////////////////////////////////
static unsigned int lowercase_offset(unsigned int cp)
{
  if (cp >= 66600)
    return 0;

  int offset_index = lowercase_offset_t2[(lowercase_offset_t1[cp >> 6] << 6) + (cp & 63)];
  return lowercase_offset_values[offset_index];
}


///////////////////////////////////////////////////////////
int NormalizeAndCalcHash(
    IN  const unsigned char* Name,
    IN  size_t               NameLen,
    IN  bool                 CaseSensitive,
    OUT unsigned int*        Hash
    )
{
  DRIVER_ONLY(const unsigned char* n0 = Name);
  const unsigned char* NameEnd = Name + NameLen;
  unsigned int databuf[4] = { 0 };
  size_t bytes;

  while (Name < NameEnd)
  {
    const unsigned char* s = Name++;

    if (*s == 0)
      break;

    if (*s < 0x80)
    {
      databuf[0] = static_cast<unsigned char>(*s - 0x41) >= 0x1A ? *s : CaseSensitive ? *s : (*s + 0x20);
      bytes = 1;
    }
    else
    {
      int uCode = utf8ToU32Code(*s, &Name, NameEnd);

      if (uCode <= 0)
      {
        UFSDTracek(( NULL, "utf8 failed to convert '%.*s' to unicode. Pos %" PZZ "u, error %d, chars %02X %02X %02X",
          (int)NameLen, n0, PtrOffset(n0, Name), uCode, *s, PtrOffset(Name, NameEnd) > 1 ? s[1] : 0, PtrOffset(Name, NameEnd) > 2 ? s[2] : 0));
        return ERR_BADNAME;
      }

      bytes = unicode_decompose(uCode, databuf);

      if (!CaseSensitive)
      {
        // TODO: Check wrong decoded symbols
        if (uCode == 0x3d1)   // GREEK THETA SYMBOL --> GREEK SMALL LETTER THETA
          databuf[0] = 0x3b8;
        else if (uCode == 0x1fc2)   // COMBINING GREEK YPOGEGRAMMENI --> GREEK SMALL LETTER IOTA
          databuf[2] = 0x3b9;
        else if (uCode == 0xdf)     // U+00DF (c3 9f) --> U+0073 LATIN SMALL LETTER S + U+0073 LATIN SMALL LETTER S
        {
          databuf[0] = databuf[1] = 0x73;
          bytes = 2;
        }
        else
          databuf[0] += lowercase_offset(databuf[0]);
      }
    }

#ifdef BASE_BIGENDIAN
    for (size_t i = 0; databuf[i] && (i < 3); ++i)
      SwapBytesInPlace(&databuf[i]);
#endif

    *Hash = crc32c(*Hash, databuf, 4 * bytes);
  }
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
static int NamesCmp(
    IN  const unsigned char*  Name1,
    IN  size_t                Len1,
    IN  const unsigned char*  Name2,
    IN  size_t                Len2,
    IN  bool                  bCaseSensitive
    )
{
  const unsigned char* Name1End = Name1 + Len1;
  const unsigned char* Name2End = Name2 + Len2;

  while (Name1 < Name1End && Name2 < Name2End)
  {
    const unsigned char* s1 = Name1++;
    const unsigned char* s2 = Name2++;
    int u1;
    int u2;

    if (*s1 == 0 || *s2 == 0)
      break;

    if (*s1 < 0x80)
      u1 = static_cast<unsigned char>(*s1 - 0x41) >= 0x1A ? *s1 : bCaseSensitive ? *s1 : (*s1 + 0x20);
    else
    {
      u1 = utf8ToU32Code(*s1, &Name1, Name1End);
      if (!bCaseSensitive)
        u1 += lowercase_offset(u1);
    }

    if (*s2 < 0x80)
      u2 = static_cast<unsigned char>(*s2 - 0x41) >= 0x1A ? *s2 : bCaseSensitive ? *s2 : (*s2 + 0x20);
    else
    {
      u2 = utf8ToU32Code(*s2, &Name2, Name2End);
      if (!bCaseSensitive)
        u2 += lowercase_offset(u2);
    }

    if (u1 <= 0 || u2 <= 0)
    {
      assert(!"Bad name");
      return ERR_BADNAME;
    }

    if (u1 != u2)
      return u1 - u2;
  }

  assert(Len1 != Len2 || (Name1 == Name1End && Name2 == Name2End));
  return static_cast<int>(Len1 - Len2);
}


/////////////////////////////////////////////////////////////////////////////
bool IsNamesEqual(
    IN  const unsigned char*  Name1,
    IN  size_t                NameLen1,
    IN  const unsigned char*  Name2,
    IN  size_t                NameLen2,
    IN  bool                  bCaseSensitive
    )
{
  return NamesCmp(Name1, NameLen1, Name2, NameLen2, bCaseSensitive) == 0;
}


#if 0
#include "../h/utrace.h"
/////////////////////////////////////////////////////////////////////////////
static unsigned int GetNameHash(const unsigned char* Name, size_t NameLen)
{
  unsigned int hash = ~0u;
  int Status = NormalizeAndCalcHash(Name, NameLen, false, &hash);

  if (!UFSD_SUCCESS(Status))
    return 0;

  return (hash & 0x3FFFFF) << 2;
}


/////////////////////////////////////////////////////////////////////////////
void BruteHash(api::IBaseLog* Log, unsigned int Expected)
{
#ifndef UFSD_TRACE
  UNREFERENCED_PARAMETER(Log);
#endif

  unsigned int data[3] = { 0 };

  for (data[0] = 0; data[0] < 0xFFF; ++data[0])
    for (data[1] = 0; data[1] < 0xFFF; ++data[1])
      //for (data[2] = 0; data[2] < 0xFFF; ++data[2])
    {
      unsigned int size = 4;

      if (data[1])
        size += 4;
      if (data[2])
        size += 4;

      unsigned int h = crc32c(~0u, data, size);

      unsigned int hash = (h & 0x3FFFFF) << 2;

      if (hash == Expected)
        ULOG_TRACE((Log, "data: %04x %04x %04x", data[0], data[1], data[2]));
    }

  ULOG_TRACE((Log, "Bruteforcing finished"));
}


/////////////////////////////////////////////////////////////////////////////
int TestHash()
{
  const unsigned char root[] = { 'r', 'o', 'o', 't' };
  unsigned int hash = GetNameHash(root, sizeof(root));
  if (hash != 0xb671e4)
    return ERR_NOT_SUCCESS;

  const unsigned char private_dir[] = { 'p', 'r', 'i', 'v', 'a', 't', 'e', '-', 'd', 'i', 'r' };
  hash = GetNameHash(private_dir, sizeof(private_dir));
  if (hash != 0xaca68c)
    return ERR_NOT_SUCCESS;

  assert(IsNamesEqual(root, sizeof(root), root, sizeof(root), true));
  assert(NamesCmp(root, sizeof(root), private_dir, sizeof(private_dir), true) > 0);
  assert(NamesCmp(private_dir, sizeof(private_dir), root, sizeof(root), true) < 0);

  // 03 50 d6 1e d0 90 00
  const unsigned char rusA[] = { 0xd0, 0x90 };
  hash = GetNameHash(rusA, sizeof(rusA));
  if (hash != 0x1ed650)
    return ERR_NOT_SUCCESS;

  // 03 48 c9 44 d0 99 00
  const unsigned char rusIi[] = { 0xd0, 0x99 };
  hash = GetNameHash(rusIi, sizeof(rusIi));
  if (hash != 0x44c948)
    return ERR_NOT_SUCCESS;

  // 03 a4 53 92 d0 af 00
  const unsigned char rusYa[] = { 0xd0, 0xaf };
  hash = GetNameHash(rusYa, sizeof(rusYa));
  if (hash != 0x9253a4)
    return ERR_NOT_SUCCESS;

  // 05 58 bd f1 d0 90 d0 99 00
  const unsigned char rusAIi[] = { 0xd0, 0x90, 0xd0, 0x99 };
  hash = GetNameHash(rusAIi, sizeof(rusAIi));
  if (hash != 0xf1bd58)
    return ERR_NOT_SUCCESS;

  assert(NamesCmp(rusA, sizeof(rusA), rusIi, sizeof(rusIi), true) < 0);
  assert(NamesCmp(rusYa, sizeof(rusYa), rusIi, sizeof(rusIi), true) > 0);
  assert(NamesCmp(rusA, sizeof(rusA), rusAIi, sizeof(rusAIi), true) < 0);
  assert(NamesCmp(rusAIi, sizeof(rusAIi), rusA, sizeof(rusA), true) > 0);

  const unsigned char jpn[] = { 0xe6, 0x96, 0xb0, 0xe3, 0x81, 0x97, 0xe3, 0x81, 0x84, 0xe3, 0x83, 0x95, 0xe3, 0x82, 0xa1, 0xe3, 0x82, 0xa4, 0xe3, 0x83, 0xab, 0x2e, 0x6a, 0x70, 0x6e };
  hash = GetNameHash(jpn, sizeof(jpn));
  if (hash != 0xB4101C)
    return ERR_NOT_SUCCESS;

  // 03 b4 7b 98 c3 85 00
  const unsigned char Angstrem[] = { 0xc3, 0x85 };
  hash = GetNameHash(Angstrem, sizeof(Angstrem));
  if (hash != 0x987bb4)
    return ERR_NOT_SUCCESS;

  // 04 4c 33 83 e2 92 b6 00
  const unsigned char u24b6[] = { 0xe2, 0x92, 0xb6 };
  hash = GetNameHash(u24b6, sizeof(u24b6));
  if (hash != 0x83334c)
    return ERR_NOT_SUCCESS;

  // 04 d4 bc 92 e2 92 be 00
  const unsigned char u24be[] = { 0xe2, 0x92, 0xbe };
  hash = GetNameHash(u24be, sizeof(u24be));
  if (hash != 0x92bcd4)
    return ERR_NOERROR;

  // 10 c0 08 00 66 69 6c 65 5f 30 78 32 34 62 65 5f e2 92 be 00 - file_0x24be_
  const unsigned char File_24be[] = { 0x66, 0x69, 0x6c, 0x65, 0x5f, 0x30, 0x78, 0x32, 0x34, 0x62, 0x65, 0x5f, 0xe2, 0x92, 0xbe };
  hash = GetNameHash(File_24be, sizeof(File_24be));
  if (hash != 0x0008c0)
    return ERR_NOT_SUCCESS;

  // 03 90 ec 4d cf 91 00
  const unsigned char u3d1[] = { 0xcf, 0x91 };
  hash = GetNameHash(u3d1, sizeof(u3d1));
  if (hash != 0x4dec90)
    return ERR_NOT_SUCCESS;

  // 04 44 c3 2e e1 bf 82 00
  const unsigned char u1fc2[] = { 0xe1, 0xbf, 0x82 };
  hash = GetNameHash(u1fc2, sizeof(u1fc2));
  if (hash != 0x2ec344)
    return ERR_NOT_SUCCESS;

  // 03 ac 79 80 c3 9f 00 - U+00DF  --> c3 9f LATIN SMALL LETTER SHARP S
  const unsigned char udf[] = { 0xc3, 0x9f };
  hash = GetNameHash(udf, sizeof(udf));
  if (hash != 0x8079ac)
    return ERR_NOT_SUCCESS;

  // 04 3c d5 ed ef bc 8b 00 - U+FF0B
  const unsigned char uPlus[] = { 0xef, 0xbc, 0x8b };
  hash = GetNameHash( uPlus, sizeof( uPlus ) );
  if ( hash != 0xedd53c )
    return ERR_NOT_SUCCESS;

  return ERR_NOERROR;
}
#endif

} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
