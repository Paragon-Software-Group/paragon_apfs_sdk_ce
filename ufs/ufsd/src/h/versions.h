// <copyright file="versions.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
#ifndef _VERSIONS_H_APFSUTIL_
#define _VERSIONS_H_APFSUTIL_

#define UFSD_APFS
#define UFSD_ZLIB
#define UFSD_LZFSE

#ifndef CPU2LE
#define CPU2LE(x)             (x)
#endif

#ifndef DRIVER_ONLY
#define DRIVER_ONLY(x)
#endif

#ifndef UFSDTracek
#define UFSDTracek(x)
#endif

#ifndef TRACE_ONLY
 #ifndef NDEBUG
  #define TRACE_ONLY(x) x
 #else
  #define TRACE_ONLY(x)
 #endif
#endif

#ifndef UFSD_VER_STRING
#define UFSD_VER_STRING
#endif

#define _T(x) x

#ifndef UFSD_TurnOnTraceLevel
  #define UFSD_TurnOnTraceLevel()
  #define UFSD_RevertTraceLevel()
#endif


#endif
