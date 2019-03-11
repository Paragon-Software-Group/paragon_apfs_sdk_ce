// <copyright file="debug.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains helper functions that is used to debug UFSD library
////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <locale.h>
#include <stdlib.h> // exit()

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h> // fsync
#endif

#include "gettext.h"
#include <api/types.hpp>
#include <api/errors.hpp>
#include <ufsd/u_errors.h>

#define IN
#define OUT

static int s_bEnableTrace;
static FILE* s_Trace = NULL;
//#define szLogName "f:\\ufsd\\head\\tst.log"

///////////////////////////////////////////////////////////
// EnableTrace
//
// This function turns on traces from UFSD
///////////////////////////////////////////////////////////
void
EnableTrace(
    IN const char* szModuleName
    )
{
  if ( NULL == s_Trace )
  {
    char szTmp[256];
    const char* szFileName;
#if !defined szLogName && defined _WIN32
    if ( 0 == GetModuleFileNameA( NULL, szTmp, sizeof(szTmp) ) )
    {
      _snprintf( szTmp, sizeof(szTmp), "./%s.log", szModuleName );
      szFileName = szTmp;
    }
    else
    {
      char* ptr = strrchr( szTmp, '.' );
      if ( NULL == ptr )
        ptr = strrchr( szTmp, '\\' );

      if ( NULL != ptr )
      {
        if ( ptr + sizeof("log") < szTmp + sizeof(szTmp) )
        {
          ptr[1] = 'l';
          ptr[2] = 'o';
          ptr[3] = 'g';
          ptr[4] = 0;
        }
        else
        {
          ptr[0] = 0;
        }
        szFileName = szTmp;
        szTmp[ARRSIZE(szTmp)-1] = 0;
      }
      else
        szFileName = "ufsd.log";
    }
#else
    snprintf( szTmp, sizeof(szTmp), "./%s.log", szModuleName );
    szFileName = szTmp;
#endif
    s_Trace = fopen( szFileName, "wt" );
  }
  s_bEnableTrace = 1;
}


///////////////////////////////////////////////////////////
// CloseTrace
//
// This function closes trace file
///////////////////////////////////////////////////////////
void
CloseTrace()
{
  if ( NULL != s_Trace )
  {
    fclose( s_Trace );
    s_Trace = NULL;
  }
  s_bEnableTrace = 0;
}


///////////////////////////////////////////////////////////
// ufsd_vtrace
//
//
///////////////////////////////////////////////////////////
extern "C" void __cdecl
ufsd_vtrace( const char *fmt, va_list arglist )
{
  char Buffer[1024];

  if ( !s_bEnableTrace )
    return;

#ifdef _WIN32
  int len = _vsnprintf( Buffer, sizeof(Buffer), fmt, arglist );
#else
  int len = vsnprintf( Buffer, sizeof(Buffer), fmt, arglist );
#endif
  bool bNewLine = len > 0 && '\n' == Buffer[len-1];
  const char* frm = bNewLine? "%s" : "%s\n";

#ifdef _WIN32
  if ( NULL == s_Trace )
    fprintf( stderr, frm, Buffer );
  else
  {
    fprintf( s_Trace, frm, Buffer );
    fflush( s_Trace );
  }

  OutputDebugStringA( Buffer );
  if ( !bNewLine )
    OutputDebugStringA( "\n" );

#else

  if ( NULL == s_Trace )
  {
    fprintf( stderr, frm, Buffer );
  }
  else
  {
    fprintf( s_Trace, frm, Buffer );
    fflush( s_Trace );
    fsync( fileno( s_Trace ) );
  }
#endif
}


///////////////////////////////////////////////////////////
// ufsd_trace
//
//
///////////////////////////////////////////////////////////
extern "C" void __cdecl
ufsd_trace( const char* fmt, ... )
{
  va_list ap;
  va_start( ap, fmt );
  ufsd_vtrace( fmt, ap );
  va_end( ap );
}


///////////////////////////////////////////////////////////
// ufsd_error
//
//
///////////////////////////////////////////////////////////
extern "C" void
ufsd_error(
    IN int          Err,
    IN const char*  FileName,
    IN int          Line
    )
{
  const char *Name = strrchr( FileName, '/' );
  if ( NULL == Name )
    Name = FileName - 1;
  ufsd_trace( "UFSD error 0x%x, %d, %s\n", Err, Line, Name + 1 );

#if defined _MSC_VER & defined _DEBUG
#ifdef _WIN64
  __debugbreak();
#else
  _asm int 3
#endif
#endif
}

#ifdef UFSD_LIB_STATIC

///////////////////////////////////////////////////////////
// __cxa_pure_virtual
//
//
///////////////////////////////////////////////////////////
extern "C" void
__cxa_pure_virtual(void)
{
//    CYG_FAIL("attempt to use a virtual function before object has been constructed");
    for ( ; ; )
    {
      ;
    }
}

#endif


///////////////////////////////////////////////////////////
// GetUfsdErrorStr
//
// Translates UFSD error code to string.
///////////////////////////////////////////////////////////
const char*
GetUfsdErrorStr(
    IN int nError
    )
{
  switch( nError )
  {
  case ERR_BADPARAMS:       return _("Bad parameters");
  case ERR_NOMEMMNGR:       return _("Memory manager is not valid");
  case ERR_BADRWBLOCK:      return _("Read/Write manager is not valid");
  case ERR_FSUNKNOWN:       return _("Unknown FileSystem");
  case ERR_NOMEMORY:        return _("Not enough memory");
  case ERR_BADUFSDFILE:     return _("Invalid file descriptor");
  case ERR_OPENROOT:        return _("Can't open root");
  case ERR_OPENFILE:        return _("Can't open file");
  case ERR_CREATEFILE:      return _("Can't create file");
  case ERR_READFILE:        return _("Can't read file");
  case ERR_WRITEFILE:       return _("Can't write file");
  case ERR_NOFILEEXISTS:    return _("File does not exist");
  case ERR_FILEEXISTS:      return _("Such file already exists");
  case ERR_NOROOTDIR:       return _("Invalid root directory");
  case ERR_NOUFSDINIT:      return _("UFSD is not initialized");
  case ERR_NOCURRENTDIR:    return _("Current directory is not set");
  case ERR_BADNAME:         return _("Ivalid file name");
  case ERR_BADNAME_LEN:     return _("File name length is invalid");
  case ERR_BADSTRTYPE:      return _("Invalid string type");
  case ERR_CLOSEUNABLE:     return _("Unable to close searches");
  case ERR_NODIRENTRY:      return _("The specified directory entry does not exist");
  case ERR_BADOBJECTTYPE:   return _("Invalid type of object");
  case ERR_CREATEDIR:       return _("Can't create directory");
  case ERR_NOFSINTEGRITY:   return _("File system is not consistent");
  case ERR_BADATTRIBUTES:   return _("Invalid file attributes");
  case ERR_WPROTECT:        return _("Volume is write protected");
  case ERR_DIRNOTEMPTY:     return _("Can't delete directory 'cause it is not empty");
  case ERR_FILENODATA:      return _("No such file");
  case ERR_NOSPC:           return _("No space left on device");
  case ERR_NOTIMPLEMENTED:  return _("This function is not implemented");
  case ERR_INSUFFICIENT_BUFFER: return _("The output buffer is too small to return any data");
  case ERR_MORE_DATA:       return _("the output buffer is too small to hold all of the data but can hold some entries");
  case ERR_BADBLOCK:        return _("Attempt to read/write bad block");
  case ERR_CANCELLED:       return _("Operation has been canceled");
  case ERR_NOMEDIA:         return _("Media is ejected");
  }

  return NULL;
}


///////////////////////////////////////////////////////////
// PrintUfsdStatusIfError
//
//
///////////////////////////////////////////////////////////
void
PrintUfsdStatusIfError(
    IN int Status,
    IN const char* Hint
    )
{
  if ( ERR_NOERROR != Status )
  {
    const char* szErr = GetUfsdErrorStr( Status );
    if ( NULL == Hint )
    {
      if ( NULL == szErr )
        fprintf( stderr, _("Error 0x%x\n"), Status );
      else
        fprintf( stderr, "%s\n", szErr );
    }
    else
    {
      if ( NULL == szErr )
        fprintf( stderr, _("\"%s\": Error 0x%x\n"), Hint, Status );
      else
        fprintf( stderr, "\"%s\": %s\n", Hint, szErr );
    }
  }
}
