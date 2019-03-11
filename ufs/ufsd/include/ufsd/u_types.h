// <copyright file="u_types.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File UFSD/include/types.h
 *
 *  Here must be placed types redefinitions for UFSD.
 ****************************************************/

//*******************************************************
//
// NOTE: According to "Preparing Code for the IA-64 Architecture":
//       http://www.intel.com/cd/ids/developer/emea/rus/dc/64bit/17969.htm
//
// The UNIX/64 and Windows 2000 (64-bit) data models have slightly diverged, differing on
// some data types. They still share the same meaning for the types char, short, int,
// float, and double. However, in Windows 2000 (64-bit), long is still 32 bits, while in
// UNIX/64 long is 64 bits. In addition, there is no ANSI C/C++ standardization for a 64-bit
// model, as of yet.
//
//*******************************************************

#include "ufsd_export.hpp"

#if defined _MSC_VER
  // These #pragma warning allows to compile UFSD at level 4 warning
  #ifdef ufs_ufsd_SHARED_DEFINE
    #pragma warning( disable : 4275 ) // non dll-interface class 'ufsd_noncopyable' used as base for dll-interface class
  #endif
#endif

//
// BASE_BIGENDIAN - external define
// Should the UFSD be compiled in bigendian memory model?
//

//#if (('1234' >> 24) != '1')  // == if little endian
//  #define BASE_BIGENDIAN
//#endif

#ifdef BASE_BIGENDIAN
  #define BE_ONLY(x)  x
  #define LE_ONLY(x)
#else
  #define BE_ONLY(x)
  #define LE_ONLY(x)  x
#endif


// Helper template from Loki library
template <int v>
struct Int2Type
{
  enum { value = v };
};


#ifndef PZZ
  //
  // Used to printf size_t values
  // F.e. size_t a;
  // printf( "%"PZZ"x\n", a );
  // is   printf( "%zx\n", a ); in GCC
  // and  printf( "%Ix\n", a ); in VC
  //
  #if defined _MSC_VER
    // MSDN: printf supports 'I', but really it does not
    #if _MSC_VER > 1200
      #define PZZ "I"
    #else
      #define PZZ ""
    #endif
  #elif defined __DJGPP__
    // DJGPP doesn't support 'Z'
    #define PZZ "l"
  #elif defined __QNX__
    // QNX doesn't support 'Z'
    #define PZZ ""
  #else
    // C99 standard
    #define PZZ "z"
  #endif
#endif


#ifndef PAGE_SIZE
  #define PAGE_SIZE   4096
  #define PAGE_SHIFT  12
#endif

#if defined UFSD_DRIVER_LINUX || defined UFSD_DRIVER_OSX
  #define UFSD_DRIVER_PURE_VIRTUAL = 0
#else
  #define UFSD_DRIVER_PURE_VIRTUAL
#endif

#define SetFlagBit( flags, bit) (flags |= (1<<bit))
#define ClearFlagBit(flags, bit) (flags &= ~(1<<bit))
#define FlagOnBit(flags, bit) ( 0 != ((flags) & (1<<bit)) )

#define U_QUOTE1(name) #name
#define U_QUOTE2(name) U_QUOTE1(name)


#define cpuid_1(have) \
  do { \
      unsigned eax, ecx; \
      eax = 1; \
      __asm__("cpuid" : "=c"(ecx) : "a"(eax) : "%ebx", "%edx"); \
      (have) = ecx ; \
  } while (0)

