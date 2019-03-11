// <copyright file="errors.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __ERRORS_H__
#define __ERRORS_H__

#pragma once

#ifndef BASE_ERROR_BASE
  // Severity bit, Customer bit
  //#define BASE_ERROR_BASE  0xA0001000   //0xA0000100 - ufsd    IMAGEMGM_ERROR_BASE  0xA0000E00    CACHEMGM_ERROR_BASE  0xA0002000
  const int BASE_ERROR_BASE = 0xA0001000;
#endif

namespace api {

#define MAKE_BASE_ERROR(x)  ( BASE_ERROR_BASE | (x) )
#define IS_BASE_ERROR(x)    ( BASE_ERROR_BASE == (BASE_ERROR_BASE & (x) ) )



#define ERR_NOERROR         0x0000


#define ERR_BADPARAMS       MAKE_BASE_ERROR(0x0001)
#define ERR_NOMEMMNGR       MAKE_BASE_ERROR(0x0002)
#define ERR_BADRWBLOCK      MAKE_BASE_ERROR(0x0003)
#define ERR_FSUNKNOWN       MAKE_BASE_ERROR(0x0006)  //unresolved file system
#define ERR_NOMEMORY        MAKE_BASE_ERROR(0x0007)
#define ERR_OPENROOT        MAKE_BASE_ERROR(0x0009)  //root subdirectory not opened on the volume
#define ERR_OPENFILE        MAKE_BASE_ERROR(0x000A)  //some error in OpenFile call
#define ERR_CREATEFILE      MAKE_BASE_ERROR(0x000B)
#define ERR_READFILE        MAKE_BASE_ERROR(0x000C)
#define ERR_WRITEFILE       MAKE_BASE_ERROR(0x000D)
#define ERR_NOFILEEXISTS    MAKE_BASE_ERROR(0x000E)
#define ERR_FILEEXISTS      MAKE_BASE_ERROR(0x000F)
#define ERR_NOROOTDIR       MAKE_BASE_ERROR(0x0010)
#define ERR_NOCURRENTDIR    MAKE_BASE_ERROR(0x0012)
#define ERR_BADNAME         MAKE_BASE_ERROR(0x0013)
#define ERR_BADSTRTYPE      MAKE_BASE_ERROR(0x0014)
#define ERR_BADNAME_LEN     MAKE_BASE_ERROR(0x0015)
#define ERR_CLOSEUNABLE     MAKE_BASE_ERROR(0x0016)
#define ERR_NODIRENTRY      MAKE_BASE_ERROR(0x0017)
#define ERR_BADOBJECTTYPE   MAKE_BASE_ERROR(0x0019)
#define ERR_CREATEDIR       MAKE_BASE_ERROR(0x001B)
#define ERR_NOFSINTEGRITY   MAKE_BASE_ERROR(0x001C)  //file system has some internal structure error
#define ERR_BADATTRIBUTES   MAKE_BASE_ERROR(0x001F)
#define ERR_WPROTECT        MAKE_BASE_ERROR(0x0020)
#define ERR_DIRNOTEMPTY     MAKE_BASE_ERROR(0x0021)
#define ERR_FILENODATA      MAKE_BASE_ERROR(0x0022)
// Not enough free space
#define ERR_NOFREESPACE     MAKE_BASE_ERROR(0x0023)

// Errors used in IoControl:
// This function is not implemented
#define ERR_NOTIMPLEMENTED      MAKE_BASE_ERROR(0x0024)
// If the output buffer is too small to return any data
#define ERR_INSUFFICIENT_BUFFER MAKE_BASE_ERROR(0x0025)
// If the output buffer is too small to hold all of the data but can hold some entries
#define ERR_MORE_DATA           MAKE_BASE_ERROR(0x0026)

// Read/write bad block
#define ERR_BADBLOCK            MAKE_BASE_ERROR( 0x0027)
// User cancels operation
#define ERR_CANCELLED           MAKE_BASE_ERROR( 0x0028)
// Wrong or not supported object format
#define ERR_WRONGFORMAT         MAKE_BASE_ERROR(0x0029)
// thread already running
#define ERR_ALREADY_RUNNING     MAKE_BASE_ERROR(0x002D)
// error while create thread
#define ERR_CREATE_THREAD       MAKE_BASE_ERROR(0x002E)

// IRestartSaver errors
// error while start saving
#define ERR_START_SAVER         MAKE_BASE_ERROR(0x002F)
// saver not started
#define ERR_SAVER_NOTSTARTED    MAKE_BASE_ERROR(0x0030)

// Volumes are different and cannot be compared
#define ERR_VOLUMES_INCOMPATIBLE MAKE_BASE_ERROR(0x0031)

// Media is ejected while operation
#define ERR_NOMEDIA             MAKE_BASE_ERROR( 0x0033 )

// Generic not success
#define ERR_NOT_SUCCESS         MAKE_BASE_ERROR( 0x0034 )

// Generic error due to failed encryption\decryption operation
#define ERR_ENCRYPTION          MAKE_BASE_ERROR( 0x0035 )

// Lock required to perform operation
#define ERR_LOCK_REQ            MAKE_BASE_ERROR( 0x0036 )

// Need to restart log
#define ERR_LOG_RESTART         MAKE_BASE_ERROR( 0x0037 )

#define ERR_ACCESSDENIED        MAKE_BASE_ERROR( 0x0039 )
#define ERR_NOTFOUND            MAKE_BASE_ERROR( 0x003a )
#define ERR_NOTSUPPORTED        MAKE_BASE_ERROR( 0x003b )
#define ERR_LOCKED              MAKE_BASE_ERROR( 0x003c )
#define ERR_NETCONNECTION       MAKE_BASE_ERROR( 0x003d )
#define ERR_UNAUTORIZED     MAKE_BASE_ERROR( 0x003e )

#define ERR_UNKNOWN             0xFFFFFFFFu

//
// Use this define to simplify error checking
//
#define BASE_SUCCESS(x) ( ERR_NOERROR == (x) )

} // namespace api

#endif //__ERRORS_H__
