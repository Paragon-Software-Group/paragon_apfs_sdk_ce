// <copyright file="time.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __TIME_H__
#define __TIME_H__

#pragma once

#include <api/types.hpp>

namespace api {

// UTC means Universal Coordinated Time
// UTC = local time + bias
class BASE_ABSTRACT_CLASS ITime
{
public:
    // This function returns UTC time:
    // 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601
    virtual UINT64 Time() = 0;

    // The bias is the difference, in minutes, between UTC time and local time.
    // UTC = Local + Bias
    //
    // F.e.
    //  Eastern time zone: +300
    //  Paris,Berlin:      -60
    //  Moscow:            -180
    //
    // The return value should be in range (-1440 : +1440), where 1440 == 24 * 60
    // If bias is out of this range then "UTC is not known" assumed
    // The implementation should return UFSD_BIAS_UTC_UNKNOWN in this case to simplify check
    //
    virtual int GetBias() = 0;

    // Number of 100ns in one second
    enum { TicksPerSecond = 10*1000*1000 };

    virtual void Destroy() = 0;

    // hfs/exfat driver uses this struct to represent time
    // tv_sec in most significant dword allows to compare times as 64 bit values
    union utime{
      UINT64 time64;
      struct {
        unsigned int tv_nsec;   // nanoseconds
        unsigned int tv_sec;    // seconds since 1970
      };
    };
};


// Seconds since 1601
#define SecondsToStartOf1904 PU64(0x0000000239EAE080)
#define SecondsToStartOf1970 PU64(0x00000002B6109100)
#define SecondsToStartOf1980 PU64(0x00000002C8DF3700)

// "GetBias" return value if UTC is unknown
#define UFSD_BIAS_UTC_UNKNOWN   0xf00000
} // namespace api

#endif //__TIME_H__
