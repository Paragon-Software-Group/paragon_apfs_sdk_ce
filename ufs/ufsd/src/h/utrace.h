// <copyright file="utrace.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file macro for trace

#include <api/log.hpp>

#if !defined NO_LOGGING && defined UFSD_TRACE

#ifdef UFSD_USE_STATIC_TRACE

  EXTERN_C void UFSD_DumpMemory( const void*  Mem, size_t nBytes );
  EXTERN_C void ufsd_error( int err, const char *file, int line );
  EXTERN_C void __cdecl ufsd_vtrace( const char *fmt, va_list ap );

  #ifdef __GNUC__
    static __inline void ufsd_trace_log( void*, const char* fmt, ... ) __attribute__ ((format (printf, 2, 3)));
    static __inline void ufsd_error_log( void*, int, const char* fmt, ... ) __attribute__ ((format (printf, 3, 4)));
  #endif

  static __inline void ufsd_trace_log( void*, const char* fmt, ... )
  {
    va_list ap;
    va_start( ap, fmt );
#ifdef UFSD_LEVEL_UFSD
    if ( ufsd_trace_level & UFSD_LEVEL_UFSD )
      ufsd_vtrace( fmt, ap );
#else
    ufsd_vtrace( fmt, ap );
#endif
    va_end( ap );
  }

  static __inline void ufsd_error_log( void*, int, const char* fmt, ... )
  {
    va_list ap;
    va_start( ap, fmt );
#ifdef UFSD_LEVEL_UFSD
    if ( ufsd_trace_level & UFSD_LEVEL_UFSD )
      ufsd_vtrace( fmt, ap );
#else
    ufsd_vtrace( fmt, ap );
#endif
    va_end( ap );
  }

  #ifndef UFSDTrace
#ifdef __GNUC__
    static void __inline ufsd_trace2( const char* fmt, ... ) __attribute__ ((format (printf, 1, 2)));
#endif

    static void __inline ufsd_trace2( const char* fmt, ... )
    {
      va_list ap;
      va_start( ap, fmt );
      ufsd_vtrace( fmt, ap );
      va_end( ap );
    }

    #define UFSDTrace(a)        ufsd_trace2 a
  #endif

  static __inline void ufsd_error_log2( void*, int )
  {

  }

  static __inline void ufsd_trace_dump_log( void*, int, const void* mem, size_t bytes ) { UFSD_DumpMemory( mem, bytes ); }
  //  static __inline void ufsd_error_log( void*, int err, const char *file, int line ) { ufsd_error( err, file, line ); }

  #define ULOG_WARNING(X)     ufsd_trace_log X
  #define ULOG_MESSAGE(X)     ufsd_trace_log X
  #define ULOG_INFO(X)        ufsd_trace_log X
  #define ULOG_TRACE(X)       ufsd_trace_log X

  #define ULOG_DEBUG1(X)      ufsd_trace_log X
  #define ULOG_DEBUG2(X)      ufsd_trace_log X
  #define ULOG_DEBUG3(X)      ufsd_trace_log X

  #define ULOG_ERROR(X)       ufsd_error_log X

  #define ULOG_DUMP(X)        ufsd_trace_dump_log  X

  #undef SLOG_ERROR
  #define SLOG_ERROR(log, code) ufsd_error( code, s_pFileName, __LINE__ )
//  #define LOG_DUMP(log, level, buf, len, ...) UFSD_DumpMemory( buf, len )

#else

  #define ULOG_WARNING(X)                    LOG_WARNING X
  #define ULOG_MESSAGE(X)                    LOG_MESSAGE X
  #define ULOG_INFO(X)                       LOG_INFO    X
  #define ULOG_TRACE(X)                      LOG_TRACE   X

  #define ULOG_DEBUG1(X)                     LOG_DEBUG1  X
  #define ULOG_DEBUG2(X)                     LOG_DEBUG2  X
  #define ULOG_DEBUG3(X)                     LOG_DEBUG3  X

  #define ULOG_ERROR(X)                      SLOG_ERROR  X
  #define ULOG_DUMP(X)                       LOG_DUMP    X

#endif

#else    //NO_LOGGING defined

  #define ULOG_WARNING(X)                    do{}while((void)0,0)
  #define ULOG_MESSAGE(X)                    do{}while((void)0,0)
  #define ULOG_INFO(X)                       do{}while((void)0,0)
  #define ULOG_TRACE(X)                      do{}while((void)0,0)

  #define ULOG_DEBUG1(X)                     do{}while((void)0,0)
  #define ULOG_DEBUG2(X)                     do{}while((void)0,0)
  #define ULOG_DEBUG3(X)                     do{}while((void)0,0)

  #define ULOG_ERROR(X)                      do{}while((void)0,0)
  #define ULOG_DUMP(X)                       do{}while((void)0,0)

  #undef UFSDTrace

#endif    //NO_LOGGING

#ifndef UFSDTrace
  #define UFSDTrace(a)                       do{}while((void)0,0)
#endif
