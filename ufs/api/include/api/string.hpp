// <copyright file="string.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __STRING_H__
#define __STRING_H__

#pragma once

#include <api/types.hpp>

namespace api {

typedef unsigned char STRType;

//not all types of string can be supported in some implementation
const unsigned char StrUnknown = 0;
const unsigned char StrUTF8    = 1;
const unsigned char StrUTF16   = 2; //unicode
const unsigned char StrUCS16   = 4; //unicode
const unsigned char StrOEM     = 8;
const unsigned char StrANSI    = 0x10;
const unsigned char StrMask    = 0x1f;

//conversion flags
const unsigned char StrUnicodeToShort= 0x20;
const unsigned char StrToLower = 0x40;
const unsigned char StrToUpper = 0x80;


class IString;

class BASE_ABSTRACT_CLASS IBaseStringManager
{
public:

    static inline
    unsigned            GetCharSize(IN STRType Type)
    {
      switch(Type & StrMask)
      {
      case StrUTF16:
      case StrUCS16:
        return 2;
      case StrUTF8:
      case StrANSI:
      case StrOEM:
        return 1;
      default:
        return 0;
      }
    }

    static inline
    bool                IsEmpty(IN STRType Type, IN const void* Str)
    {
      switch(Type & StrMask)
      {
      case StrUTF16:
      case StrUCS16:
        return 0 == *(const short*)Str;
      case StrUTF8:
      case StrANSI:
      case StrOEM:
        return 0 == *(const char*)Str;
      default:
        return true;
      }
    }

    //Retrieve string length - the number of characters until zero
    virtual size_t      strlen(
        IN STRType          Type,
        IN const void*      Str
    ) = 0;

    // fat only
    virtual unsigned    toupper(
        IN unsigned         Char
    ) = 0;

    // fat only
    virtual int         stricmp(
      IN STRType            Type,
      IN const void *       String1,
      IN const void *       String2
      ) = 0;

#ifndef UFSD_DRIVER_LINUX
    //size of string buffer in bytes with end zero
    size_t              BuffSize(
        IN STRType          Type,
        IN const void*      Str
        )
    {
      return (strlen(Type, Str) + 1) * GetCharSize(Type);
    }

    virtual const void* strchr(
        IN STRType          Type,
        IN const void *     String,
        IN unsigned         Char
    ) = 0;

    virtual const void* strrchr(
        IN STRType          Type,
        IN const void *     String,
        IN unsigned         Char
    ) = 0;
#endif

    virtual int         Convert(
        IN  STRType         SrcType,
        IN  const void*     SrcStr,
        IN  size_t          SrcLen,
        IN  STRType         DstType,
        OUT void*           DstStr,
        IN  size_t          MaxOut,
        OUT size_t*         DstLen// OPTIONAL //strlen(DstStr) after conversion
    ) = 0;

    virtual void Destroy() = 0;
};


class BASE_ABSTRACT_CLASS IStringManager : public IBaseStringManager
{
public:
    virtual int         AddRef() = 0;
    virtual int         Release() = 0;

    virtual void        strcpy(
        IN STRType          Type,
        IN OUT void  *      StrDestination,
        IN const void*      StrSource
    ) = 0;

    virtual void        strncpy(
        IN STRType          Type,
        IN OUT void   *     StrDestination,
        IN const void *     StrSource,
        IN size_t           Count
    ) = 0;

    virtual void        strcat(
        IN STRType          Type,
        IN OUT void   *     StrDestination,
        IN const void *     StrSource
    ) = 0;

    virtual int         strcmp(
        IN STRType          Type,
        IN const void *     String1,
        IN const void *     String2
    ) = 0;

    virtual int         strncmp(
        IN STRType          Type,
        IN const void *     String1,
        IN const void *     String2,
        IN size_t           Count
    ) = 0;

    virtual int         strnicmp(
        IN STRType          Type,
        IN const void *     String1,
        IN const void *     String2,
        IN size_t           Count
    ) = 0;

    virtual int         MatchMask(
        IN STRType          Type,
        IN const void *     Str, //zero terminated
        IN const void *     Mask,//zero terminated
        IN bool             fCaseSensitive
    ) = 0;

    virtual int         MatchMask(
        IN STRType          Type,
        IN const void *     Str,
        IN size_t           Len,
        IN const void *     Mask,//zero terminated
        IN bool             fCaseSensitive
    ) = 0;

    virtual int         CreateString(
        IN STRType          Type,
        IN const void *     Str,
        IN size_t           Len,
        OUT IString**       PString
    ) = 0;
};


class BASE_ABSTRACT_CLASS IString
{
public:

    virtual void        Destroy()                           = 0;

    virtual IString&    operator= (const IString& Str)      = 0;
    virtual IString&    operator+=(const IString& Str)      = 0;

    virtual int         Append(
        IN STRType          Type,
        IN const void*      Str //zero terminated
    ) = 0;

    virtual int         Append(
        IN STRType          Type,
        IN const void*      Str,
        IN size_t           Len
    ) = 0;

    virtual int         Append(
        IN const IString&   Str
    ) = 0;

    virtual int         Replace(
        IN int              Pos,
        IN size_t           Len,
        IN STRType          ReplaceStrType,
        IN const void*      ReplaceWithStr //zero terminated
    ) = 0;

    virtual int         Replace(
        IN int              Pos,
        IN size_t           Len,
        IN STRType          ReplaceStrType,
        IN size_t           ReplaceFromStrOffset,
        IN size_t           ReplaceWithSize,
        IN const void*      ReplaceWithStr //zero terminated
    ) = 0;

    virtual int         Replace(
        IN int              Pos,
        IN size_t           Len,
        IN const IString&   ReplaceWithStr
    ) = 0;

    //constant operations
    virtual bool        operator==(const IString& Str) const = 0;
    virtual bool        operator!=(const IString& Str) const = 0;

    virtual unsigned int operator[](int Pos) const   = 0;

    virtual bool        IsEmpty() const              = 0;
    virtual STRType     StrType() const              = 0;
    virtual size_t      StrLen() const               = 0;
    virtual size_t      BuffSize() const             = 0;
    virtual size_t      BuffSize(STRType Type) const = 0;

    virtual const void* StrBuffer() const            = 0;

    virtual int         Compare(
        IN STRType          Type,
        IN const void*      Str, //zero terminated
        IN bool             fCaseSensitive
    ) const = 0;

    virtual int         Compare(
        IN STRType          Type,
        IN const void*      Str,
        IN size_t           Len,
        IN bool             fCaseSensitive
    ) const = 0;

    virtual int         Compare(
        IN const IString&   Str,
        IN bool             fCaseSensitive
    ) const = 0;

    virtual int         MatchMask(
        IN STRType          Type,
        IN const void*      Mask, //zero terminated
        IN bool             fCaseSensitive
    ) const = 0;

    virtual int         MatchMask(
        IN STRType          Type,
        IN const void*      Mask,
        IN size_t           Len,
        IN bool             fCaseSensitive
    ) const = 0;

    virtual int         MatchMask(
        IN const IString&   Mask,
        IN bool             fCaseSensitive
    ) const = 0;

    virtual int         FirstPosOf(
        IN  STRType         Type,
        IN  unsigned        Char,
        IN  int             FromPos,
        OUT int*            FoundPos
    ) const = 0;

    virtual int         LastPosOf(
        IN  STRType         Type,
        IN  unsigned        Char,
        IN  int             FromPos,
        OUT int*            FoundPos
    ) const = 0;

    virtual int         Search(
        IN  STRType         Type,
        IN  const void*     Mask,
        IN  bool            fCaseSensitive,
        IN  int             FromPos,
        OUT int*            FoundPos
    ) const = 0;

    virtual int         Search(
        IN  STRType         Type,
        IN  const void*     Mask,
        IN  size_t          Len,
        IN  bool            fCaseSensitive,
        IN  int             FromPos,
        OUT int*            FoundPos
    ) const = 0;

    virtual int         Search(
        IN  const IString&  Mask,
        IN  bool            fCaseSensitive,
        IN  int             FromPos,
        OUT int*            FoundPos
    ) const = 0;

    virtual int         CopyTo(
        IN  STRType         Type,
        OUT void*           pBuff,
        IN  size_t          BuffLen,
        OUT size_t*         ResultLen
    ) const = 0;

    virtual int         CopyTo(
        IN  STRType         Type,
        OUT void*           pBuff,
        IN  size_t          BuffLen,
        OUT size_t*         ResultLen,
        IN  int             FromPos,
        IN  size_t          CopyLen
    ) const = 0;
};


//#define DISABLE_TOUTF8
#ifndef DISABLE_TOUTF8

//special class for logging string as UTF8
//class implementation placed in core project
class ToUtf8 : public base_noncopyable
{
public:
    ToUtf8(api::STRType type, const void *pStr, size_t str_len, bool len_limit);
    ~ToUtf8();
    const char *Get() {return m_out_ptr;}

private:

    char *       m_utf8_buf;
    const char * m_out_ptr;
};

#define S2UTF8(type, str)              api::ToUtf8((type), (str),    0, false).Get()
#define S2UTF8LEN(type, str, len)      api::ToUtf8((type), (str), (len), true).Get()

#else
#define S2UTF8(type, str)              (str)
#define S2UTF8LEN(type, str, len)      (str)

#endif //DISABLE_TOUTF8

} // namespace api

#endif //__STRING_H__
