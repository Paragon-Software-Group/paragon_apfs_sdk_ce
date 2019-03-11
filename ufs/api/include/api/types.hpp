// <copyright file="types.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

/*
 * NOTE: According to "Preparing Code for the IA-64 Architecture":
 * http://www.intel.com/cd/ids/developer/emea/rus/dc/64bit/17969.htm
 *
 * The UNIX/64 and Windows 2000 (64-bit) data models have slightly diverged,
 * differing on some data types. They still share the same meaning for the types
 * char, short, int, float, and double. However, in Windows 2000 (64-bit), long
 * is still 32 bits, while in UNIX/64 long is 64 bits. In addition, there is no
 * ANSI C/C++ standardization for a 64-bit model, as of yet.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#pragma once

//
// We need to know what is 'size_t'
//
#ifdef BLUESCRN
  #include <ntrtl.h>
#else
  #ifndef KERNEL
    #include <stddef.h>
    #include <stdarg.h>
  #endif

  #ifdef __APPLE__
    #include <sys/types.h>
    #include <stdarg.h>
  #endif
#endif


#if defined _MSC_VER
  // These #pragma warning allows to compile at level 4 warning
  #pragma warning( disable : 4291 ) //'declaration' : no matching operator delete found;
                                    // memory will not be freed if initialization throws an exception
  #pragma warning( disable : 4514 ) // "..." : unreferenced inline function has been removed
  #pragma warning( disable : 4711 ) // function "..." selected for automatic inline expansion
  #pragma warning( disable : 4710 ) // function "..." not inlined
  #pragma warning( disable : 4511 ) // copy constructor could not be generated
  #pragma warning( disable : 4512 ) // assignment operator could not be generated
  #pragma warning( disable : 4201 ) // nonstandard extension used : nameless struct/union
#endif


#ifndef NULL
  #define NULL   0L
#endif


#ifndef IN
  #define IN
  #define OUT
#endif


// Some functions require explicit call declaration
// especially when library is used as external library.
// Gnu requires different form of stdcall, cdecl
// so do not use it now
#ifdef __GNUC__
  #ifndef __stdcall
    #define __stdcall
    #define _stdcall
  #endif
  #ifndef __cdecl
    #define __cdecl
    #define _cdecl
  #endif
#endif


// Calling convention BASECALL
#if defined _MSC_VER
  #define BASECALL __stdcall
#elif defined(__linux__)
  #if defined __x86_64__
    #define BASECALL
  #else
    #define BASECALL __attribute__((stdcall))
  #endif
#elif defined(__APPLE__)
  #if defined __x86_64__
    #define BASECALL
  #else
    #define BASECALL __attribute__((stdcall))
  #endif
#elif defined __DJGPP__
  #define BASECALL
#else
  #define BASECALL
#endif


// Define empty __attribute__ for compilers that does not support it
#if !defined(__attribute__)
  #if !defined(__GNUC__)
    #define __attribute__(a)
  #endif
#endif


// there is a lot of cases when size does matter, i.e.
// for kernel-mode drivers. __declspec(novtable) saves
// from vtables for pure virtual classes to be created.
#if !defined(__declspec) && (!defined(_MSC_VER) || (_MSC_VER < 1200))
  #define __declspec(a)
#endif


// Not all compilers understand #pragma pack( push, 1 )/#pragma pack( pop )
// To suppress warning use this define
#if defined __arm__ || defined __mips__
  #define NO_PRAGMA_PACK
#endif


////////////////////////////////////////////////////////////////
#if !defined(BASE_ABSTRACT_CLASS)
  #if defined(_MSC_VER) && (_MSC_VER >= 1200)
    #define BASE_ABSTRACT_CLASS __declspec(novtable)
  #else
    #define BASE_ABSTRACT_CLASS
  #endif
#endif


#ifndef ARRSIZE
  #define ARRSIZE(x) (sizeof((x))/sizeof((x)[0]))
#endif


#if !defined _SIZE_T_DEFINED && !defined _SIZE_T
  #ifdef _WIN64
    typedef unsigned __int64 size_t;
    typedef signed __int64 ptrdiff_t;
  #elif defined _WIN32
    typedef unsigned int size_t;
    typedef signed int ptrdiff_t;
  #elif defined __unix && !defined __SIZE_TYPE__
    #if defined (__LP64__) || ( __WORDSIZE == 64)
      typedef unsigned long long size_t;
      typedef signed long long ptrdiff_t;
    #elif (__GNUC__ <= 2 && __GNUC_MINOR__ <= 95) || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 0)
      typedef unsigned long size_t;
      typedef signed long ptrdiff_t;
    #else
      typedef unsigned int size_t;
      typedef signed int ptrdiff_t;
    #endif
  #endif
  #define _SIZE_T_DEFINED
  #define _SIZE_T
#endif


#ifndef __WORDSIZE
  #ifdef _WIN64
    #define __WORDSIZE  64
  #elif defined _WIN32
    #define __WORDSIZE  32
  #elif defined __GNUC__
    #if defined __LP64__
      #define __WORDSIZE  64
    #else
      #define __WORDSIZE  32
    #endif
  #else
    #error "__WORDSIZE is not defined"
  #endif
#endif


////////////////////////////////////////////////////////////////
//
// Helper macroses
//
#ifndef FlagOn
  #define SetFlag(flags, flag) (flags |= (flag))
  #define ClearFlag(flags, flag) (flags &= ~(flag))
  #define FlagOn(flags, flag) ( 0 != ((flags) & (flag)))
  #define FlagOnXact(flags, flag1, flag2) ( (flag2) == ((flags) & (flag1)) )
#endif


#ifndef QuadAlign
  #define QuadAlign( n )      (((n) + 7) & (~7) )
  #define IsQuadAligned( n )  (0 == ((size_t)(n) & 7))
  #define Quad2Align( n )     (((n) + 15) & (~15) )
  #define IsQuad2Aligned( n ) (0 == ((size_t)(n) & 15))
  #define Quad4Align( n )     (((n) + 31) & (~31) )
  #define IsSizeTAligned( n ) (0 == ((size_t)(n) & (sizeof(size_t)-1)))
  #define DwordAlign( n )     (((n) + 3) & (~3) )
  #define IsDwordAligned( n ) (0 == ((size_t)(n) & 3))
  #define WordAlign( n )      (((n) + 1) & (~1) )
  #define IsWordAligned( n )  (0 == ((size_t)(n) & 1))
#endif


#ifndef Add2Ptr
  #define Add2Ptr(P,I)   ((unsigned char*)(P) + (I))
  #define PtrOffset(B,O) ((size_t)((size_t)(O) - (size_t)(B)))
#endif


////////////////////////////////////////////////////////////////
//
// This macro defines a simple unsigned value 0xFFFFFFFF (~0U) or 0xFFFFFFFFFFFFFFFF(~0ULL)
//
#define MINUS_ONE_T    ((size_t)(-1))


////////////////////////////////////////////////////////////////
//
//   compiler generates default constructor, copy constructor and operator= .
//   Nobody should call these functions anytime.
//   To reduce code size we just declare but not implement these functions
//   Really this macros is used to reduce the size of DLL of exported classes
//   And to suppress the warnings of gcc (see -Weffc++)
//
class base_noncopyable
{
 protected:
    base_noncopyable() {}
    ~base_noncopyable() {}
 private:  // emphasize the following members are private
    base_noncopyable( const base_noncopyable& );
    const base_noncopyable& operator=( const base_noncopyable& );
};


////////////////////////////////////////////////////////////////
//
// GCC used in MAC does not allow to use multiple base classes
//
#ifndef __APPLE__
  #define NOTCOPYABLE ,public base_noncopyable
#else
  #define NOTCOPYABLE
#endif


////////////////////////////////////////////////////////////////
#ifndef __PARAGON_UINT64_DEFINED
#define __PARAGON_UINT64_DEFINED

#if defined _MSC_VER
  typedef unsigned __int64    UINT64;
  typedef signed   __int64    INT64;
#elif defined __GNUC__
  typedef unsigned long long  UINT64;
  typedef long long           INT64;
#else
  #error "Does your compiler support build-in 64bit values"
#endif

#ifndef PU64
  //
  // Used to declare 64bit constants
  // F.e. #define DOS_DATE_BASE PU64(0x01A8E79FE1D58000)
  // is   #define DOS_DATE_BASE 0x01A8E79FE1D58000ULL  in GCC
  // and  #define DOS_DATE_BASE 0x01A8E79FE1D58000UI64 in VC
  //
  #define __PMERGE(a,b) a ## b
  #if defined _MSC_VER && _MSC_VER <= 1200
    #define PU64(x) __PMERGE(x, UI64)
    #define PI64(x) __PMERGE(x, I64 )
  #else
    #define PU64(x) __PMERGE(x, ULL)
    #define PI64(x) __PMERGE(x, LL)
  #endif

#endif // #ifndef PU64

#ifndef PLL
  //
  // Used to printf 64bit values
  // F.e. UINT64 a;
  // printf( "%" PLL "x\n", a );
  // is   printf( "%llx\n", a ); in GCC
  // and  printf( "%I64x\n", a ); in VC
  //
  #if defined _MSC_VER
    #define PLL "I64"
  #else
    #define PLL "ll"
  #endif
#endif // #ifndef PLL

#ifndef MINUS_ONE_64
  #define MINUS_ONE_64  PU64(0xFFFFFFFFFFFFFFFF)
#endif

#ifndef MAX_INT64
  #define MAX_INT64 PU64( 0x7FFFFFFFFFFFFFFF )
#endif


///////////////////////////////////////////////////////////
// UINT64 MERGE_UINT64
//
// Useful function to merge two 32 bit values into 64 bit value
///////////////////////////////////////////////////////////
static inline
UINT64 MERGE_UINT64( IN unsigned int low, IN unsigned int high ){
  return (((UINT64)high) << 32) | low;
}

#undef LODWORD
#undef HIDWORD
///////////////////////////////////////////////////////////
// LODWORD
//
// Simple function to access to low 32 bit of 64 bit value
// NOTE: this function returns r-value
///////////////////////////////////////////////////////////
static inline
unsigned int LODWORD( IN const UINT64& x ){
  return (unsigned int)(x);
}


///////////////////////////////////////////////////////////
// HIDWORD
//
// Simple function to access to high 32 bit of 64 bit value
// NOTE: this function returns r-value
///////////////////////////////////////////////////////////
static inline
unsigned int HIDWORD( IN const UINT64& x ){
  return (unsigned int)(x>>32);
}


#endif // __PARAGON_UINT64_DEFINED


////////////////////////////////////////////////////////////////
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID {
    unsigned int   Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID; // sizeof() == 16
#endif


////////////////////////////////////////////////////////////////
#ifndef likely

  #if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
    // The __builtin_expect is a method that gcc (versions >= 2.96) offer
    // for programmers to indicate branch prediction information to the compiler.
    // The return value of __builtin_expect is the first argument
    // (which could only be an integer) passed to it.
    #define likely(x)       __builtin_expect((x),1)
    #define unlikely(x)     __builtin_expect((x),0)

  #else

    #define likely(x) x
    #define unlikely(x) x

  #endif // #if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)

#endif // #ifndef likely

namespace api {

  // Returns non zero if v is a power of 2 and not zero
  static inline bool IsPowerOf2( size_t v ) {
    return 0 != v && 0 == (v & (v-1) );
  }

#ifndef ARGUMENT_PRESENT
  #define ARGUMENT_PRESENT(p) (NULL != (p))
#endif

} // namespace api

#endif //__TYPES_H__
