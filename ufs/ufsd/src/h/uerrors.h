// <copyright file="uerrors.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
//#include "utrace.h"

//===================================================================
//       Standard method of tracing errors
//===================================================================
#ifdef UFSD_TRACE_ERROR
  #ifdef UFSD_GET_LOG // == !defined UFSD_NO_PRINTK && !defined NO_LOGGING && !defined UFSD_DRIVER_LINUX && defined UFSD_TRACE && !defined UFSD_USE_STATIC_TRACE
    #define E( l, n )               GetLog() ? ( (GetLog())->Error((l), s_pFileName, __LINE__)  , (n) ) : (n)
    #define PRINT_LOG( code )       SLOG_ERROR( GetLog(), code )
    #define PRINT_LOG2( log, code ) SLOG_ERROR( log, code )
  #elif !defined NO_LOGGING && defined UFSD_TRACE_ERROR && defined UFSD_USE_STATIC_TRACE
    EXTERN_C void ufsd_error( int err, const char *file, int line );
    #define E( l, n )               ( ufsd_error( l, s_pFileName, __LINE__ ), (n) )
    #define PRINT_LOG( code )         ufsd_error( code, s_pFileName, __LINE__ )
    #define PRINT_LOG2( log, code )   ufsd_error( code, s_pFileName, __LINE__ )
  #endif
#endif

#ifndef PRINT_LOG
  #define E( l, n )  (n)
  #define PRINT_LOG( code ) {}
  #define PRINT_LOG2( log, code ) {}
#endif


#define CHECK_CALL( call )                 \
do{                                        \
  int StatuS = (call);                     \
  if ( unlikely( ERR_NOERROR != StatuS ) ){\
    PRINT_LOG( StatuS );                   \
    return StatuS;                         \
  }                                        \
}while((void)0,0)

#define CHECK_CALL_LOG( call, log )        \
do{                                        \
  int StatuS = (call);                     \
  if ( unlikely( ERR_NOERROR != StatuS ) ){\
    PRINT_LOG2( log, StatuS );             \
    return StatuS;                         \
  }                                        \
}while((void)0,0)

#define CHECK_CALL_SILENT( call )          \
do{                                        \
  int StatuS = (call);                     \
  if ( unlikely( ERR_NOERROR != StatuS ) ) \
    return StatuS;                         \
}while((void)0,0)


#define CHECK_CALL_EXIT_SILENT( call )     \
do{                                        \
  Status = (call);                         \
  if ( unlikely( ERR_NOERROR != Status ) ){\
    goto Exit;                             \
  }                                        \
}while((void)0,0)


#define CHECK_CALL_EXIT( call )            \
do{                                        \
  Status = (call);                         \
  if ( unlikely( ERR_NOERROR != Status ) ){\
    PRINT_LOG( Status );                   \
    goto Exit;                             \
  }                                        \
}while((void)0,0)

#define CHECK_CALL_EXIT_LOG( call, log )   \
do{                                        \
  Status = (call);                         \
  if ( unlikely( ERR_NOERROR != Status ) ){\
    PRINT_LOG2( log, Status );             \
    goto Exit;                             \
  }                                        \
}while((void)0,0)

//
// Trace error if error but cancel
//
#define CHECK_CALL_CANCEL( call )          \
do{                                        \
  int StatuS = (call);                     \
  if ( unlikely( ERR_NOERROR != StatuS ) ){\
    if ( ERR_CANCELLED != StatuS ){        \
      PRINT_LOG( StatuS );                 \
    }                                      \
    return StatuS;                         \
  }                                        \
}while((void)0,0)

#define CHECK_STATUS( s )                   \
do{                                         \
  if ( unlikely( ERR_NOERROR != (s)) )     {\
      PRINT_LOG( s );                       \
      return (s);                           \
  }                                         \
}while((void)0,0)


#define CHECK_STATUS_EXIT( s )              \
do{                                         \
  if ( unlikely( ERR_NOERROR != (s) ) )    {\
      PRINT_LOG( s );                       \
      goto Exit;                            \
  }                                         \
}while((void)0,0)


#define CHECK_STATUS_EXIT_LOG( s, log )     \
do{                                         \
  if ( unlikely( ERR_NOERROR != (s) ) )    {\
      PRINT_LOG2( log, s );                 \
      goto Exit;                            \
  }                                         \
}while((void)0,0)


#ifdef UFSD_MALLOC_CANT_FAIL

  #define CHECK_PTR( p )        (void)(p)
  #define CHECK_PTR_EXIT( p )   (void)(p)
  #define CHECK_PTR_BOOL( p )   (void)(p)
  #define CHECK_BOOL( p )       (void)(p)
  #define CHECK_BOOL_EXIT( p )  (void)(p)

#else

  #define CHECK_PTR( p )                      \
  do{                                         \
    if ( unlikely( NULL == (p) ) ){           \
      return ERR_NOMEMORY;                    \
    }                                         \
  }while((void)0,0)

  #define CHECK_PTR_EXIT( p )                 \
  do{                                         \
    if ( unlikely( NULL == (p) ) ){           \
      Status = ERR_NOMEMORY;                  \
      goto Exit;                              \
    }                                         \
  }while((void)0,0)

  #define CHECK_PTR_BOOL( p )                 \
  do{                                         \
    if ( unlikely( NULL == (p) ) ){           \
      return false;                           \
    }                                         \
  }while((void)0,0)

  #define CHECK_BOOL( p )                     \
  do{                                         \
    if ( unlikely(!(p)) ){                    \
      return ERR_NOMEMORY;                    \
    }                                         \
  }while((void)0,0)

  #define CHECK_BOOL_EXIT( p )                \
  do{                                         \
    if ( unlikely(!(p)) ){                    \
      Status = ERR_NOMEMORY;                  \
      goto Exit;                              \
    }                                         \
  }while((void)0,0)

#endif // #ifdef UFSD_MALLOC_CANT_FAIL
