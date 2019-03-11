// <copyright file="ucommon.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/*++

Module Name:

    common.h

Abstract:

    This module contains common macros for ufsd

Revision History:

    14-April-2016  - created.

--*/


#ifndef DEBUG_ONLY
  #ifdef NDEBUG
    #define DEBUG_ONLY(X)
  #else
    #define DEBUG_ONLY(X)      X
  #endif
#endif


#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define CEIL_DOWN(vol,alignment)            ((vol) / (alignment))
#define CEIL_DOWN64(vol,alignment)          (div_u64((vol), (alignment)))

#define CEIL_UP(vol,alignment)              (((vol) + (alignment) - 1)/(alignment))
#define CEIL_UP64(vol,alignment)            (div_u64(((vol) + (alignment) - 1), (alignment)))

#define ROUND_DOWN(vol,alignment)           (((vol) / (alignment)) * (alignment))

#define ROUND_UP(vol,alignment)             ((((vol) + (alignment) - 1)/(alignment)) * (alignment))
#define ROUND_UP64(vol,alignment)           (div_u64(((vol) + (alignment) - 1), (alignment)) * (alignment))

#define MAX_ROUND_DOWN(vol1,vol2,alignment) (ROUND_DOWN(MAX((vol1),(vol2)), (alignment)))

#define MAX_ROUND_UP(vol1,vol2,alignment)   (ROUND_UP(MAX((vol1),(vol2)), (alignment)))
