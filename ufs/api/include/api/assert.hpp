// <copyright file="assert.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __ASSERT_H__
#define __ASSERT_H__

#pragma once

#include <api/types.hpp>

////////////////////////////////////////////////////////////////
#ifndef assert

  #if defined __APPLE__ && !defined MACH_ASSERT && !defined NDEBUG
    #define NDEBUG
  #endif

  #ifdef NDEBUG
    #define assert(e) do {} while((void)0,0)

  #elif defined _MSC_VER
    #if _MSC_VER >= 1400
      #define assert(e) do {if(!(0,e)) {__debugbreak();}} while((void)0,0)
    #else
      #define assert(e) do {if(!(0,e)) {__asm int 3}} while((void)0,0)
    #endif

  #else
  // Use standard assert. Usually it calls exit()
    #ifdef __APPLE__
      #include <kern/assert.h>
    #else
      #include <assert.h>
    #endif
  #endif

#endif // assert


////////////////////////////////////////////////////////////////
#ifndef verify
  #ifdef NDEBUG
    #define verify(f)     ((void)(f))
  #else
    #define verify(f)     assert(f)
  #endif
#endif


////////////////////////////////////////////////////////////////
#ifndef __APPLE__
  #ifndef C_ASSERT
    #if defined(_MSC_VER) && (_MSC_VER < 1300)
      // __LINE__ macro broken when -ZI is used see Q199057
      // fortunately MSVC and GCC up to 4.x ignores duplicate typedef's.
      #define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
    #elif defined __GNUC__ && __cplusplus >= 201103L
      // static assert in gcc since 4.3, requires -std=c++0x
      #define C_ASSERT(e) static_assert( e, #e )
    #else
      #define __CA__JOIN(a)  __CA__JOIN1(a)
      #define __CA__JOIN1(a) abc##a
      #define C_ASSERT(e) typedef char __CA__JOIN(__LINE__)[(e)?1:-1]
    #endif
  #endif
#else
  #define C_ASSERT(e)
#endif


#ifndef UNREFERENCED_PARAMETER
# define UNREFERENCED_PARAMETER(P)   ((void)(P))
#endif

//#include <stddef.h>

#ifndef FIELD_OFFSET
# if defined(UFSD_OSX_SNAPSHOT_USER_LIB) || defined(UFSD_OSX_P2V)
#   define FIELD_OFFSET(type, field) __builtin_offsetof(type, field)
# elif ( defined __GNUC__ && !defined __APPLE__) || defined offsetof
#  define FIELD_OFFSET offsetof
# elif defined __WATCOMC__
#  define FIELD_OFFSET(type, field)    __offsetof(type,field)
# elif defined(_MSC_VER) && (_MSC_VER >= 1300)
   // MSC 13.0 and above has keywords used to allow 64-bit support detection.
#  define FIELD_OFFSET(type, field)    ((long)(size_t)(size_t*)&(((type *)0)->field))
# else
#  define FIELD_OFFSET(type, field)    ((long)(long*)&(((type *)0)->field))
# endif
#endif


#ifndef IS_FIELD_ALIGNED
  // true if field is aligned
# define IS_FIELD_ALIGNED(type, field) \
   (0 == (FIELD_OFFSET(type,field) % sizeof(((type *)0)->field)) )
#endif

#endif //__ASSERT_H__
