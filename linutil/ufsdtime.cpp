// <copyright file="ufsdtime.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file implements CTime for UFSD library
//
////////////////////////////////////////////////////////////////

#include <time.h>
#ifdef _WIN32
  #include <windows.h>
  #define timezone _timezone
  #define tzset _tzset
#else
  #include <sys/time.h>
#endif

#ifdef __ANDROID__
#include <linux/time.h>
#endif

// Include UFSD specific files
#include <ufsd.h>

#define _100ns2seconds  (10*1000*1000)
#define SecondsToStartOf1970 PU64(0x00000002B6109100)


class UFSD_Time : public api::ITime
{
public:
  // This function returns UTC
  virtual UINT64 Time()
  {
#ifdef WIN32
    UINT64 ft;
    C_ASSERT( sizeof(FILETIME) == sizeof(UINT64) );
    GetSystemTimeAsFileTime( (FILETIME*)&ft );
    return ft;
#else
    timeval tv;
    if ( 0 != gettimeofday( &tv, NULL ) )
    {
      // error
      tv.tv_sec   = time(0);
      tv.tv_usec  = 0;
    }
    return (SecondsToStartOf1970 + tv.tv_sec) * _100ns2seconds + tv.tv_usec * 10;
#endif
  }

  // The bias is the difference, in minutes, between UTC time and local time.
  // F.e.
  //  Eastern time zone: +300
  //  Paris,Berlin:      -60
  //  Moscow:            -180
  virtual int GetBias()
  {
#if defined __linux__ || defined _WIN32
    return timezone/60; // timezone stores it in seconds.
#else
    struct timezone tz = {0};
    if ( 0 == gettimeofday(NULL, &tz) )
      return tz.tz_minuteswest;
    return 0;
#endif
  }

  virtual void Destroy() {}
};

// The only Time Manager
static UFSD_Time s_TimeMngr;


///////////////////////////////////////////////////////////
// UFSD_GetTimeService
//
// The API exported from this module.
// It returns the pointer to the static class.
///////////////////////////////////////////////////////////
api::ITime*
UFSD_GetTimeService(void)
{
  tzset();
  return &s_TimeMngr;
}
