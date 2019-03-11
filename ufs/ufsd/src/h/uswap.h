// <copyright file="uswap.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains special functions to swap bytes and values

namespace UFSD{

// Usefull define to swap simple types
#define SWAP(a,b) ( (a^=b),(b^=a),(a^=b) )

#ifdef BASE_BIGENDIAN
#  define LE2CPU(x)   GetSwapped( (x) )
#  define CPU2LE(x)   GetSwapped( (x) )
#  define BE2CPU(x)   (x)
#  define CPU2BE(x)   (x)
#  define LE2CPU_CONST16(x) GetSwappedConst16(x)
#  define LE2CPU_CONST32(x) GetSwappedConst32(x)
#  define LE2CPU_CONST64(x) GetSwappedConst64(x)
#  define CPU2LE_CONST16(x) GetSwappedConst16(x)
#  define CPU2LE_CONST32(x) GetSwappedConst32(x)
#  define CPU2LE_CONST64(x) GetSwappedConst64(x)
#  define BE2CPU_CONST16(x) (x)
#  define BE2CPU_CONST32(x) (x)
#  define BE2CPU_CONST64(x) (x)
#  define CPU2BE_CONST16(x) (x)
#  define CPU2BE_CONST32(x) (x)
#  define CPU2BE_CONST64(x) (x)
#else
#  define LE2CPU(x)   (x)
//#  define LE2CPU(x)   GetSwapped( GetSwapped( (x) ) )
#  define CPU2LE(x)   (x)
#  define BE2CPU(x)   GetSwapped( (x) )
#  define CPU2BE(x)   GetSwapped( (x) )
#  define BE2CPU_CONST16(x) GetSwappedConst16(x)
#  define BE2CPU_CONST32(x) GetSwappedConst32(x)
#  define BE2CPU_CONST64(x) GetSwappedConst64(x)
#  define CPU2BE_CONST16(x) GetSwappedConst16(x)
#  define CPU2BE_CONST32(x) GetSwappedConst32(x)
#  define CPU2BE_CONST64(x) GetSwappedConst64(x)
#  define LE2CPU_CONST16(x) (x)
#  define LE2CPU_CONST32(x) (x)
#  define LE2CPU_CONST64(x) (x)
#  define CPU2LE_CONST16(x) (x)
#  define CPU2LE_CONST32(x) (x)
#  define CPU2LE_CONST64(x) (x)
#endif

//
// Don't use common swap algorithm
// 'cause some compilers are ill
//
//template <class T>
//void swap( T& a, T& b)
//{
//  T t = a;  a = b;  b = t;
//}



#define GetSwappedConst16(x) ((unsigned short)(                 \
    (((unsigned short)(x) & (unsigned short)0x00ffU) << 8) |    \
    (((unsigned short)(x) & (unsigned short)0xff00U) >> 8)))

#define GetSwappedConst32(x) ((unsigned int)(                   \
    (((unsigned int)(x) & (unsigned int)0x000000ffU) << 24) |   \
    (((unsigned int)(x) & (unsigned int)0x0000ff00U) <<  8) |   \
    (((unsigned int)(x) & (unsigned int)0x00ff0000U) >>  8) |   \
    (((unsigned int)(x) & (unsigned int)0xff000000U) >> 24)))

#define GetSwappedConst64(x) ((UINT64)(                         \
    (((UINT64)(x) & (UINT64)PU64(0x00000000000000ff)) << 56) |  \
    (((UINT64)(x) & (UINT64)PU64(0x000000000000ff00)) << 40) |  \
    (((UINT64)(x) & (UINT64)PU64(0x0000000000ff0000)) << 24) |  \
    (((UINT64)(x) & (UINT64)PU64(0x00000000ff000000)) <<  8) |  \
    (((UINT64)(x) & (UINT64)PU64(0x000000ff00000000)) >>  8) |  \
    (((UINT64)(x) & (UINT64)PU64(0x0000ff0000000000)) >> 24) |  \
    (((UINT64)(x) & (UINT64)PU64(0x00ff000000000000)) >> 40) |  \
    (((UINT64)(x) & (UINT64)PU64(0xff00000000000000)) >> 56)))


#define UFSD_USE_HTONL_SYNTAX "Activate this define to use htonl syntax for swapping"

///////////////////////////////////////////////////////////
// GetSwapped
//
//
///////////////////////////////////////////////////////////
static inline unsigned short
GetSwapped(
    IN const unsigned short& v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  return GetSwappedConst16( v );
#else
  // Don't (alignment problem):  return (unsigned short)( (v >> 8) | (v << 8) );
  union {
    unsigned short r;
    unsigned char  t[2];
  };
  const unsigned char* s  = (unsigned char*)&v;
//  unsigned char* t  = (unsigned char*)&r;
  t[0] = s[1]; t[1] = s[0];
  return r;
#endif
}


///////////////////////////////////////////////////////////
// GetSwapped
//
//
///////////////////////////////////////////////////////////
static inline unsigned int
GetSwapped(
    IN const unsigned int& v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  return GetSwappedConst32( v );
#else
  // Don't (alignment problem): return (unsigned int)( (v >> 24) | ((v & 0x00ff0000) >> 8) | ((v & 0x0000ff00) << 8) | (v << 24) );
  union {
    unsigned int r;
    unsigned char t[4];
  };
  const unsigned char* s  = (unsigned char*)&v;
//  unsigned char* t  = (unsigned char*)&r;
  t[0] = s[3]; t[1] = s[2]; t[2] = s[1]; t[3] = s[0];
  return r;
#endif
}


///////////////////////////////////////////////////////////
// GetSwapped
//
//
///////////////////////////////////////////////////////////
static inline UINT64
GetSwapped(
    IN const UINT64& v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  return GetSwappedConst64( v );
#else
  union {
    UINT64 r;
    unsigned char dst[8];
  };
//  unsigned char* dst        = (unsigned char*)&r;
  const unsigned char* src  = (const unsigned char*)&v;
  dst[0] = src[7];
  dst[1] = src[6];
  dst[2] = src[5];
  dst[3] = src[4];
  dst[4] = src[3];
  dst[5] = src[2];
  dst[6] = src[1];
  dst[7] = src[0];
  return r;
#endif
}


///////////////////////////////////////////////////////////
// GetSwapped
//
//
///////////////////////////////////////////////////////////
static inline unsigned short
GetSwapped(
    IN const unsigned short* v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  return GetSwappedConst16( *v );
#else
  // Don't (alignment problem):  return (unsigned short)( (v >> 8) | (v << 8) );
  union{
    unsigned short r;
    unsigned char  t[2];
  };
  const unsigned char* s  = (unsigned char*)v;
//  unsigned char* t  = (unsigned char*)&r;
  t[0] = s[1]; t[1] = s[0];
  return r;
#endif
}


///////////////////////////////////////////////////////////
// GetSwapped
//
//
///////////////////////////////////////////////////////////
static inline unsigned int
GetSwapped(
    IN const unsigned int* v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  return GetSwappedConst32( *v );
#else
  union {
    unsigned int r;
    unsigned char t[4];
  };
  const unsigned char* s  = (unsigned char*)v;
//  unsigned char* t  = (unsigned char*)&r;
  t[0] = s[3]; t[1] = s[2]; t[2] = s[1]; t[3] = s[0];
  return r;
#endif
}


///////////////////////////////////////////////////////////
// GetSwapped
//
//
///////////////////////////////////////////////////////////
static inline UINT64
GetSwapped(
    IN const UINT64* v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  return GetSwappedConst64( *v );
#else
  union{
    UINT64 r;
    unsigned char dst[8];
  };
//  unsigned char* dst        = (unsigned char*)&r;
  const unsigned char* src  = (const unsigned char*)v;
  dst[0] = src[7];
  dst[1] = src[6];
  dst[2] = src[5];
  dst[3] = src[4];
  dst[4] = src[3];
  dst[5] = src[2];
  dst[6] = src[1];
  dst[7] = src[0];
  return r;
#endif
}


///////////////////////////////////////////////////////////
// SwapBytesInPlace
//
//
///////////////////////////////////////////////////////////
static inline void
SwapBytesInPlace(
    IN OUT unsigned short* v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  *v = GetSwappedConst16( *v );
#else
  // Don't (alignment problem): *v = (unsigned short)( (*v >> 8) | (*v << 8) );
  unsigned char* p  = (unsigned char*)v;
  unsigned char t;
  t = p[0]; p[0] = p[1]; p[1] = t;
#endif
}


///////////////////////////////////////////////////////////
// SwapBytesInPlace
//
//
///////////////////////////////////////////////////////////
static inline void
SwapBytesInPlace(
    IN OUT short* v
    )
{
  // Don't (alignment problem): *v = (short)( ((unsigned)*v >> 8) | ((unsigned)*v << 8) );
  unsigned char* p  = (unsigned char*)v;
  unsigned char t;
  t = p[0]; p[0] = p[1]; p[1] = t;
}


///////////////////////////////////////////////////////////
// SwapBytesInPlace
//
//
///////////////////////////////////////////////////////////
static inline void
SwapBytesInPlace(
    IN OUT unsigned int* v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  *v = GetSwappedConst32( *v );
#else
  // Don't (alignment problem): *v = (unsigned int)( (*v >> 24) | ((*v & 0x00ff0000) >> 8) | ((*v & 0x0000ff00) << 8) | (*v << 24) );
  unsigned char* p  = (unsigned char*)v;
  unsigned char t;
  t = p[0]; p[0] = p[3]; p[3] = t;
  t = p[1]; p[1] = p[2]; p[2] = t;
#endif
}


///////////////////////////////////////////////////////////
// SwapBytesInPlace
//
//
///////////////////////////////////////////////////////////
static inline void
SwapBytesInPlace(
    IN OUT UINT64* v
    )
{
#ifdef UFSD_USE_HTONL_SYNTAX
  *v = GetSwappedConst64( *v );
#else
  unsigned char* p  = (unsigned char*)v;
  unsigned char t;
  t = p[0]; p[0] = p[7]; p[7] = t;
  t = p[1]; p[1] = p[6]; p[6] = t;
  t = p[2]; p[2] = p[5]; p[5] = t;
  t = p[3]; p[3] = p[4]; p[4] = t;
#endif
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT bool& a,
    IN OUT bool& b
    )
{
  bool t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT unsigned char& a,
    IN OUT unsigned char& b
    )
{
  unsigned char t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT unsigned short& a,
    IN OUT unsigned short& b
    )
{
  unsigned short t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT unsigned int& a,
    IN OUT unsigned int& b
    )
{
  unsigned int t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT unsigned long& a,
    IN OUT unsigned long& b
    )
{
  unsigned long t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT UINT64& a,
    IN OUT UINT64& b
    )
{
  UINT64 t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// swap
//
//
///////////////////////////////////////////////////////////
static inline void
swap(
    IN OUT void*& a,
    IN OUT void*& b
    )
{
  void* t = a; a = b; b = t;
}


///////////////////////////////////////////////////////////
// SwapStr
//
//
///////////////////////////////////////////////////////////
static inline void
SwapStr(
    OUT unsigned short* d,
    IN const unsigned short* s,
    IN size_t l
    )
{
  unsigned char* db = (unsigned char*)d;
  const unsigned char* sb = (unsigned char*)s;
  while( 0 != l-- )
  {
    db[0] = sb[1];
    db[1] = sb[0];
    db += 2;
    sb += 2;
  }
}


///////////////////////////////////////////////////////////
// SwapStr
//
//
///////////////////////////////////////////////////////////
static inline void
SwapStr(
    IN OUT unsigned short* s,
    IN size_t l
    )
{
  unsigned char* p = (unsigned char*)s;
  while( 0 != l-- )
  {
    unsigned char t;
    t = p[0]; p[0] = p[1]; p[1] = t;
    p += 2;
  }
}


///////////////////////////////////////////////////////////
// IsStrSame
//
// returns true if strings match
///////////////////////////////////////////////////////////
static inline bool
IsStrSame(
    IN const unsigned short* d,
    IN const unsigned short* s,
    IN size_t l
    )
{
  unsigned char* db = (unsigned char*)d;
  const unsigned char* sb = (unsigned char*)s;
  while( 0 != l-- )
  {
    if ( db[0] != sb[1] || db[1] != sb[0] )
      return false;
    db += 2;
    sb += 2;
  }

  return true;
}


} // namespace UFSD{
