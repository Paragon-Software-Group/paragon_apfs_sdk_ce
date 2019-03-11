// <copyright file="funcs.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains declarations of many external functions
// used in this project
//
// NOTE: File <ufsd.h> should be included before this file
////////////////////////////////////////////////////////////////

#ifndef _UFSD_FUNCS_H
#define _UFSD_FUNCS_H

#ifndef IN
# define IN
# define OUT
#endif

#ifndef PACKAGE_TAG
  #define PACKAGE_TAG "(" __DATE__ " " __TIME__ ")"
#endif

#define VER_STRING _(NLS_CAT_NAME " " PACKAGE_TAG "\nUFSD\nParagon Software Group\n\n")


#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

///////////////////////////////////////////////////////////
// UFSD_GetMessageService
//
// Returns time manager to be used in UFSD. See ufsdtime.cpp
///////////////////////////////////////////////////////////
api::ITime*
UFSD_GetTimeService();

///////////////////////////////////////////////////////////
// UFSD_GetStringService
//
// Returns string manager to be used in UFSD. See ufsdstr.cpp
///////////////////////////////////////////////////////////
api::IBaseStringManager*
UFSD_GetStringsService();

///////////////////////////////////////////////////////////
// UFSD_GetMemoryManager
//
// Returns memory manager to be used in UFSD. See ufsdmmgr.cpp
///////////////////////////////////////////////////////////
api::IBaseMemoryManager*
UFSD_GetMemoryManager();

///////////////////////////////////////////////////////////
// UFSD_IOHandlerCreate
//
// Returns read/write manager to be used in UFSD. See ufsdio.cpp
///////////////////////////////////////////////////////////
int
UFSD_IOHandlerCreate(
    IN const char*           szDevice,
    IN bool                  bReadOnly,
    IN bool                  bNoDiscard, // If not zero then skip discard volume
    OUT api::IDeviceRWBlock**  Rw,
    OUT int*                 fd,
    IN bool                  bForceDismount,
    IN bool                  bVerbose,
    IN unsigned int          BytesPerSector
    );

///////////////////////////////////////////////////////////
// UFSD_FSDumpIOCreate
//
// Returns read/write manager to be used in UFSD. See fsdumpio.cpp
///////////////////////////////////////////////////////////
int
UFSD_FSDumpIOCreate(
    IN const char*           szDevice,
    IN bool                  bReadOnly,
    OUT api::IDeviceRWBlock**  RwBlock,
    IN bool                  bVerbose,
    IN unsigned int          BytesPerSector
    );

///////////////////////////////////////////////////////////
// UFSD_GetMessageService
//
// Returns mesasge manager to be used in UFSD. See ufsdmsg.cpp
///////////////////////////////////////////////////////////
api::IMessage*
UFSD_GetMessageService(IN FILE* fd = stdout,
                  IN size_t (__stdcall *ExtSyncRequest)(
                  IN api::IMessage*  This,
                  IN size_t     code,
                  IN OUT void*  arg) = NULL,
                  IN bool bSkipPercents = false);

///////////////////////////////////////////////////////////
// utf8_wcstombs
//
// Converts a wide string to the corresponding multibyte string
///////////////////////////////////////////////////////////
size_t
utf8_wcstombs(char *s, size_t n, const unsigned short *pwcs, size_t m);

///////////////////////////////////////////////////////////
// EnableBreak
//
// Enable/Disable break
///////////////////////////////////////////////////////////
void
EnableBreak(
    IN bool bEnable
    );

///////////////////////////////////////////////////////////
// GetUfsdErrorStr
//
// Translates UFSD error code to string. See debug.c
///////////////////////////////////////////////////////////
const char*
GetUfsdErrorStr(
    IN int nError
    );

///////////////////////////////////////////////////////////
// PrintUfsdStatusIfError
//
// Prints on stderr error string
///////////////////////////////////////////////////////////
void
PrintUfsdStatusIfError(
    IN int nError,
    IN const char* Hint = NULL
    );


///////////////////////////////////////////////////////////
// EnableTrace
//
// Enables trace from UFSD
///////////////////////////////////////////////////////////
void
EnableTrace(
    IN const char* ModuleName
    );

///////////////////////////////////////////////////////////
// CloseTrace
//
// This function closes trace file
///////////////////////////////////////////////////////////
EXTERN_C
void
CloseTrace();


///////////////////////////////////////////////////////////
// ParseSize
//
// Parses string for size like
// 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K, 32M
// Returns 0 if error
///////////////////////////////////////////////////////////
unsigned int
ParseSize(
    IN const char* s
    );


///////////////////////////////////////////////////////////
// SetPartitionType
//
// Sets partition type
///////////////////////////////////////////////////////////
bool
SetPartitionType(
    IN int            Device,
    IN unsigned char  Type = 7
    );


#ifdef __linux__
///////////////////////////////////////////////////////////
// GetMount
//
// Returns corresponded part of string from file "/proc/mounts"
// Or NULL if device is not mounted
///////////////////////////////////////////////////////////
const char*
GetMount(
    IN const char* szDevice,
    IN bool        bVerbose
    );
#else
  #define GetMount( szDevice, bVerbose ) NULL
#endif

///////////////////////////////////////////////////////////
// SplitPath
//
// szPath - absolute path.
//   (1)       Can be represented as <device_name>/<relative_path>
//   (2)       Or                    <mount_name>/<relative_path>
//
// bMounted is set to:
//    0 if name has a form (1)
// or 1 if name has a form (2)
//
//
// If the function successed it returns the pointer into szPath
// which points to relative path and begins with '/'
//
// Returns NULL if not possible to parse
///////////////////////////////////////////////////////////
// const char*
// SplitPath(
//     IN const char*  szPath,
//     IN bool         bVerbose,
//     OUT bool*       bMounted       // Can be NULL
//     );

///////////////////////////////////////////////////////////
// ParsePath
//
//
// szPath - absolute path.
//   (1)       Can be represented as <device_name>/<relative_path>
//   (2)       Or                    <mount_name>/<relative_path>
//
// bMounted is set to:
//    0 if name has a form (1)
// or 1 if name has a form (2)
//
//
// If the function successed it returns the pointer into szPath
// which points to relative path and begins with '/'
//
// Returns NULL if not possible to parse
///////////////////////////////////////////////////////////
const char*
ParsePath(
    IN  const char* szPath,
    IN  bool        bVerbose   = false,
    OUT char**      szDevice   = NULL,
    OUT char**      szMntDir   = NULL,
    OUT char**      szMntOpt   = NULL,
    OUT char**      szMntType  = NULL,
    OUT char**      szRelative = NULL,
    OUT bool*       bMounted   = NULL
    );


///////////////////////////////////////////////////////////
// GetAbsPath
//
// Translates relative path to absolute one
// Returns true if OK
// A-la realpath
///////////////////////////////////////////////////////////
bool
GetAbsPath(
    IN  const char* szRelative,
    IN  bool        bVerbose,
    OUT char*       szAbsPath,
    IN  size_t      nMaxCharsInAbsPath
    );


#endif // #ifndef _UFSD_FUNCS_H
