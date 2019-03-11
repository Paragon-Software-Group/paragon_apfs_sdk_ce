// <copyright file="ufsdstr.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file implements CStrings for UFSD library
//
// NOTE: (UFSD uses UTF-16 Unicode)
//
////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <locale.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
static WCHAR (WINAPI * s_RtlUpcaseUnicodeChar)( IN WCHAR  SourceCharacter );
#endif

#ifndef C_ASSERT
#define C_ASSERT(e) typedef char __C_ASSERT__ [(e)?1:-1]
#endif


// Include UFSD specific files
#include <ufsd.h>

#if defined _MSC_VER
  // Let next functions be always inline
  #pragma intrinsic( strcmp, strcat, strcpy, strlen )
#endif


#if defined _MSC_VER
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif

typedef unsigned char  __u8;

#ifndef __ANDROID__
typedef char           __s8;
#else
#define __s8 char
#endif

typedef unsigned short __u16;

C_ASSERT( sizeof(__u16) == 2 );
C_ASSERT( sizeof(__u8)  == 1 );


class CUFSD_Strings: public api::IBaseStringManager
{
public:
  /////////////////////////////////////////////////////////
  //            IBaseStringManager virtual
  /////////////////////////////////////////////////////////
  virtual size_t strlen(api::STRType type, const void * str);
  virtual unsigned toupper( unsigned c );
  virtual int stricmp(api::STRType type, const void* str1, const void* str2);

#ifndef UFSD_DRIVER_LINUX
  virtual const void *strchr(api::STRType type,  const void *string, unsigned c );
  virtual const void *strrchr(api::STRType type,  const void *string, unsigned c );
#endif
  virtual int Convert(api::STRType, const void*, size_t, api::STRType, void*, size_t, size_t*);

#if 0
  /////////////////////////////////////////////////////////
  //            IStringManager virtual
  /////////////////////////////////////////////////////////
  virtual int AddRef() { return ERR_NOTIMPLEMENTED; }
  virtual int Release() { return ERR_NOTIMPLEMENTED; }
  virtual void strcpy(api::STRType, void*, const void*) {}
  virtual void strncpy(api::STRType, void*, const void*, size_t) {}
  virtual void strcat(api::STRType, void*, const void*) {}
  virtual int strcmp(api::STRType, const void*, const void*) { return ERR_NOTIMPLEMENTED; }
  virtual int strncmp(api::STRType, const void*, const void*, size_t) { return ERR_NOTIMPLEMENTED; }
  virtual int strnicmp(api::STRType, const void*, const void*, size_t) { return ERR_NOTIMPLEMENTED; }
  virtual int MatchMask(api::STRType, const void*, const void*, bool) { return ERR_NOTIMPLEMENTED; }
  virtual int MatchMask(api::STRType, const void*, size_t, const void*, bool) { return ERR_NOTIMPLEMENTED; }
  virtual int CreateString(api::STRType, const void*, size_t, api::IString**) { return ERR_NOTIMPLEMENTED; }
#endif
  virtual void Destroy(){}
};


///////////////////////////////////////////////////////////
// CUFSD_Strings::toupper
//
// Converts a given lowercase letter to an uppercase letter if possible and appropriate.
///////////////////////////////////////////////////////////
unsigned
CUFSD_Strings::toupper(
    IN unsigned c
    )
{
#if defined UFSD_FAT || defined INCLUDE_UFSD_CHKFAT || defined INCLUDE_UFSD_MKFAT
  return ::toupper(c);
#else
  assert( 0 );
  return c;
#endif
}


///////////////////////////////////////////////////////////
// CUFSD_Strings::stricmp
//
// The strcmp function lexicographically compares lowercase versions of string1 and string2
// and returns a value indicating their relationship.
// Returns:
//   < 0  - string1 less than string2
//    0   - string1 is identical to string2
//   > 0  - string1 greater than string2
///////////////////////////////////////////////////////////
int
CUFSD_Strings::stricmp(
    IN api::STRType type,
    IN const void *string1,
    IN const void *string2
    )
{
  assert(string1);
  assert(string2);

  unsigned int cs = GetCharSize(type);

  if ( cs == sizeof(char) )
#ifdef _WIN32
    return _stricmp(reinterpret_cast<const char*>(string1), reinterpret_cast<const char*>(string2));
#else
    return ::strcasecmp(reinterpret_cast<const char*>(string1), reinterpret_cast<const char*>(string2));
#endif

  if ( cs == sizeof(unsigned short) )
  {
#ifdef _WIN32
    return _wcsicmp( (const wchar_t*)string1, (const wchar_t*)string2 );
#else
    int f,l;
    const __u16* dst = reinterpret_cast<const __u16*>(string1);
    const __u16* src = reinterpret_cast<const __u16*>(string2);

    do {
      f = ::toupper(*(dst++));
      l = ::toupper(*(src++));
    } while ( f && (f == l) );
    return f - l;
#endif
  }
  assert(0);
  return -1;
}

#ifndef UFSD_DRIVER_LINUX
///////////////////////////////////////////////////////////
// CUFSD_Strings::strchr
//
// The strchr function finds the first occurrence of c in string, or it returns NULL
// if c is not found.
// The null-terminating character is included in the search.
// type - string type ASCII or UNICODE
///////////////////////////////////////////////////////////
const void*
CUFSD_Strings::strchr(
    IN api::STRType type,
    IN const void *string,
    IN unsigned c
    )
{
  if ( GetCharSize(type) == sizeof(char) )
    return ::strchr(reinterpret_cast<const char *>(string), (int)(c));
  else if ( GetCharSize(type) == sizeof(__u16) )
  {
#ifdef _WIN32
    return ::wcschr( (const wchar_t*)string, (wchar_t)c );
#else
    for ( const __u16* str = reinterpret_cast<const __u16*>(string); 0 != *str; str++ )
    {
      if( *str == c )
        return (void*)str;
    }
    return NULL;
#endif
  }
  assert(0);
  return NULL;
}



///////////////////////////////////////////////////////////
// CUFSD_Strings::strrchr
//
// The strrchr function finds the last occurrence of c in string.
// The search includes the terminating null character.
// type - string type ASCII or UNICODE
// Returns a pointer to the last occurrence of c in string, or NULL if c is not found.
///////////////////////////////////////////////////////////
const void*
CUFSD_Strings::strrchr(
    IN api::STRType type,
    IN const void *string,
    IN unsigned c
    )
{
  if ( GetCharSize(type) == sizeof(char) )
    return ::strrchr(reinterpret_cast<const char *>(string), (int)(c));
  else if ( GetCharSize(type) == sizeof(__u16) )
  {
#ifdef _WIN32
    return ::wcsrchr( (const wchar_t*) string, (wchar_t)c );
#else
    for ( const __u16* str = reinterpret_cast<const __u16*>(string) + this->strlen( api::StrUTF16, string ) - 1;
          str >= string;
          str-- )
    {
      if( *str == c )
        return (void*)str;
    }
    return NULL;
#endif
  }
  assert(0);
  return NULL;
}
#endif


//
// See file /linux/fs/nls/nls_base.c
//

/*
 * Sample implementation from Unicode home page.
 * http://www.stonehand.com/unicode/standard/fss-utf.html
 */
struct utf8_table {
  int     cmask;
  int     cval;
  int     shift;
  long    lmask;
  long    lval;
};

static struct utf8_table utf8_table[] =
{
  {0x80,  0x00,   0*6,    0x7F,           0,         /* 1 byte sequence */},
  {0xE0,  0xC0,   1*6,    0x7FF,          0x80,      /* 2 byte sequence */},
  {0xF0,  0xE0,   2*6,    0xFFFF,         0x800,     /* 3 byte sequence */},
  {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000,   /* 4 byte sequence */},
  {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000,  /* 5 byte sequence */},
  {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000, /* 6 byte sequence */},
  {0,                   /* end of table    */}
};


///////////////////////////////////////////////////////////
// utf8_mbtowc
//
// Convert a multibyte character to a corresponding wide character
// n - Number of bytes to check (in 's')
// If the object that 's' points to forms a valid multibyte
// character, utf8_mbtowc returns the length in bytes of the multibyte character.
// If the object that 's' points to is a wide-character null character (L'\0'),
// the function returns 1.
// If the object that 's' points to does not form a valid multibyte character
// within the first 'n' characters, it returns –1.
///////////////////////////////////////////////////////////
static int
utf8_mbtowc(__u16 *p, const __u8 *s, int n)
{
  int nc = 0;
  int c0 = *s;
  long l = c0;
  for ( const struct utf8_table *t = utf8_table; t->cmask; t++) {
    nc += 1;
    if ((c0 & t->cmask) == t->cval) {
      l &= t->lmask;
      if (l < t->lval)
        return -1;
      *p = (__u16)l;
      return nc;
    }
    if (n <= nc)
      return -1;
    s += 1;
    int c = (*s ^ 0x80) & 0xFF;
    if (c & 0xC0)
      return -1;
    l = (l << 6) | c;
  }
  return -1;
}


///////////////////////////////////////////////////////////
// utf8_mbstowcs
//
// Convert a multibyte string to a corresponding wide string
// m - maximum characters in 'pwcs'
// n - maximum number of chars in 's'
///////////////////////////////////////////////////////////
static int
utf8_mbstowcs(__u16 *pwcs, int m, const __s8 *s, int n)
{
  __u16 *op;
  const __u8 *ip;
  int size;

  op = pwcs;
  ip = (const __u8*)s;
  while ( n > 0 && *ip && m > 0) {
    if (*ip & 0x80) {
      size = utf8_mbtowc(op, ip, n);
      if (-1 == size) {
        /* Ignore character and move on */
        ip += 1;
        n  -= 1;
      } else {
        op += 1;
        m  -= 1;
        ip += size;
        n  -= size;
      }
    } else {
      *op++ = *ip++;
      n -= 1;
      m -= 1;
    }
  }
  return (int)(op - pwcs);
}


///////////////////////////////////////////////////////////
// utf8_wctomb
//
// Converts a wide character to the corresponding multibyte character
// If utf8_wctomb converts the wide character to a multibyte character,
// it returns the number of bytes (which is never greater than MB_CUR_MAX)
// in the wide character.
// If 'wc' is the wide-character null character (L'\0'), utf8_wctomb returns 1.
// If the conversion is not possible in the utf8 locale, utf8_wctomb returns –1.
// n - maximum number of bytes in 's'
///////////////////////////////////////////////////////////
static int
utf8_wctomb(__u8 *s, int n, __u16 wc )
{
  long l = wc;
  for (const struct utf8_table *t = utf8_table; t->cmask; t++) {
    if (l <= t->lmask) {
      int c = t->shift;
      __u8* s0 = s;
      *s++ = (__u8)(t->cval | (l >> c));
      while( c > 0 && --n > 0 ) {
        c -= 6;
        *s++ = (__u8)(0x80 | ((l >> c) & 0x3F));
      }
      return (int)(s - s0);
    }
  }
  return -1;
}


///////////////////////////////////////////////////////////
// utf8_wcstombs
//
// Converts a wide string to the corresponding multibyte string
// n - maximum number of chars in 's'
// m - maximum characters in 'pwcs'
///////////////////////////////////////////////////////////
size_t
utf8_wcstombs(char *s, size_t n, const unsigned short *pwcs, size_t m)
{
  const __u16 *ip = pwcs;
  __u8* op = (__u8*)s;

  while (m-- > 0 && *ip && n > 0) {
    if (*ip > 0x7f) {
      int size = utf8_wctomb(op, (int)n, *ip);
      if (size == -1) {
        /* Ignore character and move on */
      } else {
//        __u16 tmp;
//        assert( size == utf8_mbtowc(&tmp, op, size) && tmp == *ip );
        op += size;
        n  -= size;
      }
    } else {
      *op++ = (__u8) *ip;
      n -= 1;
    }
    ip++;
  }
  return op - (__u8*)s;
}


//#define CHECK_CONVERT "Use this define to check string conversation UNICODE->MBCS->UNCIODE"

///////////////////////////////////////////////////////////
// CUFSD_Strings::Convert
//
//Convert string strSrc from UNICODE to ASCII and vice versa
//Result is in strDest.
//  srcLen - the length of source string in characters
//            if (long)SrcLen < 0 then zero terminated string is assumed
//  MaxOut - number of characters in output buffer
//  DstLen - the length of destination string in characters
// Parameter type defines type of result string:
// if type - StrASCII, then result is in UNICODE format.
// if type - StrUNICODE, then result is in ASCII format.
//Returns:
//  0 - if success
///////////////////////////////////////////////////////////
int
CUFSD_Strings::Convert(
    IN  api::STRType  type,
    IN  const void*   strSrc,
    IN  size_t        SrcLen,
    IN  api::STRType  dstype,
    OUT void*         strDest,
    IN  size_t        MaxOut,
    OUT size_t*       DstLen
    )
{
#if 0
  int Ret  = 0;
  size_t dLen   = 0;

  if ( (long)SrcLen < 0 )
    SrcLen = 0x7fffffff; //INT_MAX;

  if ( SrcLen > MaxOut )
    SrcLen = MaxOut;

  if ( GetCharSize(type) == sizeof(char) )
  {
    const char* aStr  = reinterpret_cast<const char*>(strSrc);
    __u16* wDst       = reinterpret_cast<__u16*>(strDest);
    while( dLen < SrcLen )
    {
      dLen += 1;
      if ( 0 == (*wDst++ = *aStr++) )
        break;
    }
  }
  else if ( GetCharSize(type) == sizeof(__u16) )
  {
    const __u16* wStr   = reinterpret_cast<const __u16*>(strSrc);
    char* aDst          = reinterpret_cast<char*>(strDest);
    while( dLen < SrcLen )
    {
      dLen += 1;
      if ( 0 == (*aDst++ = (char)(*wStr++)) )
        break;
    }
  }
  else
  {
    assert(0);
    Ret = ~0u;
  }

  if ( NULL != DstLen )
    *DstLen = dLen;
  return Ret;

#else

  size_t tmp;
  if ( (long)SrcLen < 0 )
    SrcLen = 0x7fffffff; //INT_MAX;
  if ( NULL == DstLen )
    DstLen = &tmp;
  if ( GetCharSize(type) == sizeof(char) )
  {
    if (GetCharSize(dstype) == sizeof(char))
    {
      memcpy(strDest, strSrc, SrcLen);
      *DstLen = SrcLen;

      if (dstype & api::StrToUpper)
      {
        char* s = reinterpret_cast<char*>(strDest);
        for (size_t i = 0; i < SrcLen; ++i)
          s[i] = (unsigned char)toupper(s[i]);
      }
    }
    else
    {
      // MBCS->UNICODE
      int WChars = utf8_mbstowcs( (__u16*)strDest, (int)MaxOut, (const char*)strSrc, (int)SrcLen );
      if ( WChars > 0 )
      {
        *DstLen = WChars;
        if ( *DstLen < MaxOut )
          reinterpret_cast<__u16*>(strDest)[*DstLen] = 0;
#if defined CHECK_CONVERT & !defined NDEBUG
        int Len    = (*DstLen*4 + 1);
        char* aStr = (char*)malloc( Len );
        if ( NULL != aStr )
        {
          // UNICODE->MBCS
          if ( 0x7fffffff == SrcLen )
            SrcLen = ::strlen( (const char*)strSrc );
          size_t SrcLen2 = utf8_wcstombs( aStr, Len, (const __u16*)strDest, *DstLen );
          assert( SrcLen == SrcLen2 && 0 == memcmp( aStr, strSrc, SrcLen2 ) );
          free( aStr );
        }
#endif
      }
    }
    return 0;
  }
  else if ( GetCharSize(type) == sizeof(__u16) )
  {
    // UNICODE->MBCS
    size_t Chars = utf8_wcstombs( (char*)strDest, (int)MaxOut, (const __u16*)strSrc, (int)SrcLen );
    if ( Chars > 0 )
    {
      *DstLen = Chars;
      if ( *DstLen < MaxOut )
        reinterpret_cast<char*>(strDest)[*DstLen] = 0;
#if defined CHECK_CONVERT & !defined NDEBUG
      int Len    = sizeof(__u16)*(1 + *DstLen);
      __u16* wStr = (__u16*)malloc( Len );
      if ( NULL != wStr )
      {
        // MBCS->UNICODE
        if ( 0x7fffffff == SrcLen )
          SrcLen = ::wcslen( (const wchar_t*)strSrc );
        int SrcLen2 = utf8_mbstowcs( wStr, Len, (const char*)strDest, (int)*DstLen );
        assert( SrcLen == SrcLen2 && 0 == memcmp( wStr, strSrc, SrcLen2*2 ) );
        free( wStr );
      }
#endif
      return 0;
    }
  }

  *DstLen = 0;
  assert(0);
  return ~0U;
#endif
}


///////////////////////////////////////////////////////////
// CUFSD_Strings::strlen
//
// Retrieve string length - the number of characters until zero
///////////////////////////////////////////////////////////
size_t
CUFSD_Strings::strlen(
    IN api::STRType type,
    IN const void * str
    )
{
  assert(NULL != str);

  if ( GetCharSize(type) == sizeof(char) )
    return ::strlen(reinterpret_cast<const char*>(str));
  else if ( GetCharSize(type) == sizeof(__u16) )
  {
    const __u16* wStr = reinterpret_cast<const __u16*>(str);
    size_t Len = 0;
    while( 0 != *wStr++ )
      Len++;
    return Len;
  }

  assert(0);
  return 0;
}



// The only string manager
static CUFSD_Strings s_StrMngr;


///////////////////////////////////////////////////////////
// UFSD_GetStringsService
//
// The API exported from this module.
// It returns the pointer to the static class.
///////////////////////////////////////////////////////////
api::IBaseStringManager*
UFSD_GetStringsService()
{
#ifdef WIN32
  //*(FARPROC*)&s_RtlUpcaseUnicodeChar = GetProcAddress( GetModuleHandleW(L"ntdll.dll"), "RtlUpcaseUnicodeChar" );
  setlocale( LC_ALL, "" );
#else
//  setlocale( LC_ALL, "UTF-8" );
#endif
  return &s_StrMngr;
}
