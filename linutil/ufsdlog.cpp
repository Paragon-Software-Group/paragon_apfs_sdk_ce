// <copyright file="ufsdlog.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file implements api::ILog interface for UFSD library
//
////////////////////////////////////////////////////////////////

#include <ufsd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>


#define UFSD_TRACE_BUFSIZE 0x800


class UFSD_Log : public api::ILog
{
public:
  UFSD_Log()
    : m_Log( NULL )
    , m_Mask( 0 )
    , m_ReffCount( 1 )
  {
    *m_ModuleName = 0;
  }

  void SetMask( unsigned int Mask ) { m_Mask = Mask; }
  bool IsOpen() const { return m_Log != NULL; }

  //=======================================================
  //            api::IBaseLog virtual functions
  //=======================================================
public:

  virtual bool CanTrace( unsigned int LogLevel );
  virtual int Trace( unsigned int LogLevel, int PosIndent, const char* Message, ... );
  virtual int Dump( unsigned int LogLevel, int PosIndent, const void* Buffer, size_t Length );
  virtual int Dump( unsigned int LogLevel, int PosIndent, const void* Buffer, size_t Length, const char* Message, ... );
  virtual int Error( int Code, const char* FileName, size_t LineNumber );
  virtual int Error( int Code, const char* FileName, size_t LineNumber, const char* Message, ... );
  virtual int RootError( int /*Code*/, unsigned int /*Nature*/, const char* /*Description*/, const char* /*FileName*/, size_t /*LineNumber*/ ) { return ERR_NOTIMPLEMENTED; }
  virtual int RootError( int /*Code*/, unsigned int /*Nature*/, const char* /*Description*/, const char* /*FileName*/, size_t /*LineNumber*/, const char* /*Message*/, ... ) { return ERR_NOTIMPLEMENTED; }
  virtual int GetRootError( int* /*RootError*/, unsigned int* /*Nature*/ ) { return ERR_NOTIMPLEMENTED; }
  virtual int GetRootDescription( char* /*Buff*/, size_t /*BuffSize*/, size_t* /*DescriptionSize*/ ) { return ERR_NOTIMPLEMENTED; }
  virtual void ClearRootError() {}
  //attach params to root error, while processing occured error upward in callstack
  virtual int AddRootErrorParam( const char* /*moduleName*/, const char* /*param*/, const char* /*value*/ ) { return ERR_NOTIMPLEMENTED; }
  virtual int GetRootErrorParams( char* /*buf*/, size_t /*len*/, size_t* /*n*/ ) { return ERR_NOTIMPLEMENTED; }

  //=======================================================
  //            api::ILog virtual functions
  //=======================================================

  virtual unsigned int GetLogMask( bool /*fDefault*/ ) { return m_Mask; }
  virtual int GetIndent() { return 0; }
  virtual int CreateLog( const char* ModuleName, ILog** Log );
  virtual int CreateLog( const char* /*ModuleName*/, unsigned int /*LogMask*/, int /*PosIndent*/, ILog** /*Log*/ ) { return ERR_NOTIMPLEMENTED; }
  virtual int AddRef() { return ++m_ReffCount; }

  virtual int Release()
  {
    int Ref = m_ReffCount;

    if ( --Ref == 0 && m_Log )
      fflush( m_Log );
    else
      m_ReffCount = Ref;

    return Ref;
  }

private:
  FILE* m_Log;
  unsigned int  m_Mask;
  int           m_ReffCount;
  char          m_ModuleName[MAX_FILENAME];
};


///////////////////////////////////////////////////////////////////////////////
bool UFSD_Log::CanTrace( unsigned int LogLevel )
{
  if ( !LogLevel )
    LogLevel = LOG_LEVEL_INFO;

  return FlagOn( m_Mask, LogLevel );
}


///////////////////////////////////////////////////////////////////////////////
int UFSD_Log::CreateLog( const char* ModuleName, ILog** Log )
{
  if ( !Log )
    return ERR_BADPARAMS;

  UFSD_Log* l = NULL;

  if ( *Log != this )
  {
    l = new UFSD_Log();

    if ( !l )
      return ERR_NOMEMORY;

    l->m_Mask = m_Mask;
    l->m_Log = m_Log;

    *Log = l;

    ++m_ReffCount;
  }
  else
    l = this;

  if ( l->m_ModuleName[0] == '\0' )
    snprintf( l->m_ModuleName, MAX_FILENAME, "%s", ModuleName );

  if ( !IsOpen() && m_Mask )
  {
    char log[MAX_FILENAME];
    snprintf( log, MAX_FILENAME, "%s.log", ModuleName );
    m_Log = fopen( log, "wt" );

    if ( !m_Log )
      return ERR_OPENFILE;
  }

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////////////////////////
int UFSD_Log::Trace( unsigned int LogLevel, int PosIndent, const char* Message, ... )
{
  int Status = ERR_NOERROR;

  if ( m_Log && CanTrace( LogLevel ) )
  {
    va_list args;
    va_start( args, Message );

    const size_t size_msgbuf = UFSD_TRACE_BUFSIZE;//_vscprintf(Message, args) + 1;
    char msgbuf[size_msgbuf] = {};
    vsnprintf( msgbuf, size_msgbuf, Message, args );
    int err = fprintf( m_Log, "%s: %s\n", m_ModuleName, msgbuf );
    va_end( args );

    if ( err < 0 )
      Status = ERR_WRITEFILE;
  }

  return Status;
}


///////////////////////////////////////////////////////////////////////////////
int UFSD_Log::Error( int Code, const char* FileName, size_t LineNumber )
{
  return Trace( LOG_LEVEL_ERROR, 0, "%s: Error %#x, %s:%lu", m_ModuleName, Code, FileName, LineNumber );
}


///////////////////////////////////////////////////////////////////////////////
int UFSD_Log::Error( int Code, const char* FileName, size_t LineNumber, const char* Message, ... )
{
  int Status = ERR_NOERROR;

  if ( m_Log && CanTrace( LOG_LEVEL_ERROR ) )
  {
    va_list args;
    va_start( args, Message );

    const size_t size_msgbuf = UFSD_TRACE_BUFSIZE;//_vscprintf(Message, args) + 1;
    char msgbuf[size_msgbuf] = {};
    vsnprintf( msgbuf, size_msgbuf, Message, args );
    int err = fprintf( m_Log, "%s: Error %#x, %s:%zu, %s\n", m_ModuleName, Code, FileName, LineNumber, msgbuf );
    va_end( args );

    if ( err < 0 )
      Status = ERR_WRITEFILE;
  }

  return Status;
}


//---------------------------------------------------------
// GetDigit
//
// Helper for Dump function
//---------------------------------------------------------
char
GetDigit( size_t Digit )
{
  Digit &= 0xF;
  if ( Digit <= 9 )
    return (char)('0' + Digit);

  return (char)('A' + Digit - 10);
}


///////////////////////////////////////////////////////////
int
UFSD_Log::Dump( unsigned int LogLevel, int PosIndent, const void* Mem, size_t nBytes )
{
  if ( NULL == Mem || 0 == nBytes )
    return ERR_NOERROR;

  if ( !CanTrace( LogLevel ) )
    return ERR_NOERROR;

  const unsigned char* Arr = (const unsigned char*)Mem;
  const unsigned char* Last = Arr + nBytes;
  size_t  nRows = (nBytes + 0xF) >> 4;
  for ( size_t i = 0; i < nRows && Arr < Last; i++ )
  {
    char szLine[75];
    char* ptr = szLine;

    // Create address 0000, 0010, 0020, 0030, and so on
    if ( nBytes > 0x10000 )
      * ptr++ = GetDigit( i >> 16 );
    if ( nBytes > 0x8000 )
      * ptr++ = GetDigit( i >> 12 );
    *ptr++ = GetDigit( i >> 8 );
    *ptr++ = GetDigit( i >> 4 );
    *ptr++ = GetDigit( i >> 0 );
    *ptr++ = '0';
    *ptr++ = ':';
    *ptr++ = ' ';

    const unsigned char* Arr0 = Arr;
    int j;
    for ( j = 0; j < 0x10 && Arr < Last; j++, Arr++ )
    {
      *ptr++ = GetDigit( *Arr >> 4 );
      *ptr++ = GetDigit( *Arr >> 0 );
      *ptr++ = ' ';
    }
    while ( j++ < 0x10 )
    {
      *ptr++ = ' ';
      *ptr++ = ' ';
      *ptr++ = ' ';
    }

    *ptr++ = ' ';
    *ptr++ = ' ';
    for ( Arr = Arr0, j = 0; j < 0x10 && Arr < Last; j++ )
    {
      unsigned char ch = *Arr++;
      if ( ch >= 128 )
        ch -= 128;

      if ( ch < 32 || 0x7f == ch || '%' == ch )
        ch = '.';//^= 0x40;

      *ptr++ = ch;
    }
    // Set last zero
    *ptr++ = 0;

    Trace( LogLevel, PosIndent, "%s", szLine );
  }

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
int
UFSD_Log::Dump(
  unsigned int        LogLevel,
  int                 PosIndent,
  const void* Buffer,
  size_t              Length,
  const char* Message,
  ...
)
{
  va_list args;
  va_start( args, Message );

  const size_t msg_bufsize = UFSD_TRACE_BUFSIZE;//_vscprintf(Message, args) + 1;
  char buf[msg_bufsize] = {};
  vsnprintf( buf, msg_bufsize, Message, args );
  va_end( args );

  int Status = Trace( LogLevel, PosIndent, buf );
  if ( UFSD_SUCCESS( Status ) )
    Status = Dump( LogLevel, PosIndent, Buffer, Length );

  return Status;
}


static UFSD_Log s_Log;



///////////////////////////////////////////////////////////
// UFSD_GetLog
//
// The API exported from this module.
// It returns the pointer to the static class.
///////////////////////////////////////////////////////////
api::ILog*
UFSD_GetLog()
{
  return &s_Log;
}


///////////////////////////////////////////////////////////
// EnableTrace
//
// Enables trace from UFSD
///////////////////////////////////////////////////////////
void EnableLogTrace( const char* ModuleName )
{
  s_Log.SetMask( LOG_MASK_ALL );

  if ( !s_Log.IsOpen() )
  {
    api::ILog* pLog = &s_Log;
    s_Log.CreateLog( ModuleName, &pLog );
  }
}

/*void DisableTrace()
{
  s_Log.SetMask(0);
}*/
