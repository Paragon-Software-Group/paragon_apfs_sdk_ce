// <copyright file="misc.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file implements usefull utilities
//
////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
  #include <direct.h> // getcwd
#else
  #ifdef __linux__
    #include <mntent.h>
  #endif
  #include <sys/types.h>
  #include <sys/stat.h>

  #include <unistd.h> // getcwd
  #include <limits.h>
  #include <dirent.h>
#endif


#ifndef IN
# define IN
# define OUT
#endif

#ifndef ARRSIZE
  #define ARRSIZE(x) (sizeof((x))/sizeof((x)[0]))
#endif

#if !defined MOUNTED && defined _PATH_MOUNTED
  #define MOUNTED _PATH_MOUNTED
#endif


#ifdef _WIN32
#elif defined __linux__ || defined __APPLE__
#if defined __linux__
///////////////////////////////////////////////////////////
// GetAnotherName
//
// This function looks for entry in "/dev" folder that points to
// given name.
//
// Returns static buffer like "/dev/hda1"
// which points to given name
///////////////////////////////////////////////////////////
static const char*
GetAnotherName(
    IN const char* szName,
    IN bool        bVerbose
    )
{
  char szLink[PATH_MAX];
  static char szPath[PATH_MAX];
  struct dirent *dp;
  struct stat StatBuf;

//  if ( bVerbose )
//    fprintf( stdout, "GetAnotherName: Input name \"%s\"\n", szName );

  if ( strlen( szName ) <= 5 || 0 != memcmp( szName, "/dev/", 5 ) )
  {
//    if ( bVerbose )
//      fprintf( stdout, "GetAnotherName: Input name \"%s\" is not a \"dev\" name\n", szName );
    return NULL;
  }

  if( 0 != stat( szName, &StatBuf ) )
    return NULL;

//  if ( bVerbose )
//    fprintf( stdout, "GetAnotherName: st_mode 0x%lx\n", StatBuf.st_mode );

  if( S_ISBLK(StatBuf.st_mode)
   && ( NULL == strchr( szName + 5, '/' ) || ( !strncmp( "/dev/block/vold/", szName, 16 ) ) ) )
  {
    // The input name has a form "/dev/hda1"
    if( readlink( szName, szLink, ARRSIZE(szLink) ) < 0 )
      return NULL;

    int err = '/' != szLink[0]
      ? snprintf( szPath, ARRSIZE(szPath), "/dev/%s", szLink )
      : snprintf( szPath, ARRSIZE(szPath), "%s", szLink );
    if ( err < 0 )
      abort();

//    if ( bVerbose )
//      fprintf( stdout, "GetAnotherName: Name \"%s\" is the link to \"%s\"\n", szName, szPath );
    return szPath;
  }

  DIR* dirp = opendir( "/dev" );
  if ( NULL == dirp )
    return NULL;

  while ( NULL != (dp = readdir (dirp)) )
  {
    struct stat StatBuf;
    char const *entry = dp->d_name;
    if ( '\0' == entry['.' != entry[0]? 0 : '.' != entry[1]? 1 : 2] )
      continue;

    if ( 0 == strcmp( "root", dp->d_name ) )
      continue;

    snprintf( szPath, ARRSIZE(szPath), "/dev/%s", dp->d_name );

    if( 0 != stat( szPath, &StatBuf ) )
      continue;

    if( !S_ISBLK(StatBuf.st_mode) )
      continue;

    if( readlink( szPath, szLink, ARRSIZE(szLink) ) < 0 )
      continue;

//    if ( bVerbose )
//      fprintf( stdout, "GetAnotherName: \"%s\" => \"%s\"\n", szPath, szLink );

    if( 0 == strcmp( szName + 5, szLink ) )
    {
      if ( bVerbose )
        fprintf( stdout, "GetAnotherName: Link \"%s\" links to \"%s\"\n", szPath, szName );
      closedir( dirp );
      return szPath;
    }
  }

//  if ( bVerbose )
//    fprintf( stdout, "GetAnotherName: not found\n" );

  closedir(dirp);
  return NULL;
}


///////////////////////////////////////////////////////////
// GetMountHlp
//
// Returns corresponded part of string from file "/proc/mounts"
// Or NULL if device is not mounted
///////////////////////////////////////////////////////////
static const char*
GetMountHlp(
    IN const char* szMntFile,
    IN const char* szDevice,
    IN bool        bVerbose
    )
{
  static char szLine[512];

  const char* szLink = GetAnotherName( szDevice, bVerbose );
  FILE *f = fopen( szMntFile, "r" );

  if ( NULL != f )
  {
    szLine[0] = 0;
    for ( ;; )
    {
      struct mntent *mnt = getmntent(f);
      if ( NULL == mnt )
        break;

      if ( 0 == strcmp( "none", mnt->mnt_fsname ) )
        continue;

      if ( 0 == strcmp( szDevice, mnt->mnt_fsname )
       || (NULL != szLink && 0 == strcmp( szLink, mnt->mnt_fsname ) ) )
      {
        snprintf( szLine, ARRSIZE(szLine), "%s %s %s %d %d",
                  mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts,
                  mnt->mnt_freq, mnt->mnt_passno );

        if ( bVerbose )
          fprintf( stdout, "GetMount(%s): \"%s\" is mounted at \"%s\"\n", szMntFile, szDevice, szLine );

        break;
      }
    }
    fclose (f);

    if ( 0 != szLine[0] )
      return szLine;
  }

  if ( bVerbose )
    fprintf( stdout, "GetMount(%s): \"%s\" is not mounted\n", szMntFile, szDevice );

  return NULL;
}


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
    )
{
  const char* Ret = GetMountHlp( "/proc/mounts", szDevice, bVerbose );
#ifdef MOUNTED
  if ( NULL == Ret )
    Ret = GetMountHlp( MOUNTED, szDevice, bVerbose );
#endif
  return Ret;
}
#endif // #if defined __linux__


///////////////////////////////////////////////////////////
// ParsePathHlp
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
// If the function successes it returns the pointer into szPath
// which points to relative path and begins with '/'
//
// Returns NULL if not possible to parse
///////////////////////////////////////////////////////////
static const char*
ParsePathHlp(
    IN const char*  szMntFile,
    IN const char*  szPath,
    IN  bool        bVerbose,
    OUT char**      szDevice,
    OUT char**      szMntDir,
    OUT char**      szMntOpt,
    OUT char**      szMntType,
    OUT char**      szRelative,
    OUT bool*       bMounted
    )
{
  const char* szRet = NULL;

  // Reset return values
  if ( NULL != szDevice )
    *szDevice = NULL;
  if ( NULL != szMntDir )
    *szMntDir = NULL;
  if ( NULL != szMntOpt )
    *szMntOpt = NULL;
  if ( NULL != szMntType )
    *szMntType = NULL;
  if ( NULL != szRelative )
    *szRelative = NULL;
  if ( NULL != bMounted )
    *bMounted = false;

  if ( bVerbose )
    fprintf( stdout, "ParsePath: \"%s\"\n", szPath );

  bool bDeviceOnly = false;
  size_t path_len = strlen( szPath );

#if defined __linux__
    //
    // Parse given name for <mount_name>/<relative_path>
    //
    FILE* f = fopen( szMntFile, "r" );
    if ( NULL == f )
  {
    if ( bVerbose )
    {
      fprintf( stdout, "failed to open \"%s\"\n", szMntFile );
    }
    return NULL;
  }
  for ( ;; )
  {
    struct mntent *mnt = getmntent( f );
    if ( NULL == mnt )
      break;

    if ( 0 == strcmp( "none", mnt->mnt_fsname )
      || 0 == strcmp( "/dev", mnt->mnt_fsname )
      || 0 == strcmp( "devfs", mnt->mnt_fsname )
      || 0 == strcmp( "udev", mnt->mnt_fsname )
      || 0 == strcmp( "/dev", mnt->mnt_dir )        // devtmpfs on /dev type devtmpfs (rw,mode=0755)
      )
    {
      continue;
    }

    size_t dir_len    = strlen(mnt->mnt_dir);
    size_t fsname_len = strlen(mnt->mnt_fsname);
    const char* szNewRet = NULL;
    const char* szHint   = NULL;

//      fprintf( stdout, "MOUNT: %s %s %s %s\n", mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts );
    if ( dir_len > 1 && path_len >= dir_len
      && 0 == memcmp( szPath, mnt->mnt_dir, dir_len )
      && ( 0 == szPath[dir_len] || '/' == szPath[dir_len] ) )
    {
      //
      // path has a form "/mnt/ntfs/...."
      //
      szNewRet  = szPath + dir_len;
      szHint    = "directory";

Found:
      if ( bVerbose )
        fprintf( stdout, "ParsePath: Found mounted %s: %s on %s %s (%s)\n", szHint, mnt->mnt_fsname, mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts );

      if ( NULL == szRet || (szNewRet - szPath) > (szRet - szPath) )
      {
        szRet = szNewRet;
        // Free previous match and store new one
        if ( NULL != szDevice )
        {
          free( *szDevice );
          *szDevice = strdup( mnt->mnt_fsname );
        }
        if ( NULL != szMntDir )
        {
          free( *szMntDir );
          *szMntDir = strdup( mnt->mnt_dir );
        }
        if ( NULL != szMntType )
        {
          free( *szMntType );
          *szMntType = strdup( mnt->mnt_type );
        }
        if ( NULL != szMntOpt )
        {
          free( *szMntOpt );
          *szMntOpt = strdup( mnt->mnt_opts );
        }
        if ( NULL != szRelative )
        {
          free( *szRelative );
          *szRelative = strdup( 0 == szRet[0]? "/." : szRet );
        }
      }

      //
      // Don't break here. Just continue enumeration. May be we will find the most better match
      //
      continue;
    }

    if ( path_len >= fsname_len
      && 0 == memcmp(szPath, mnt->mnt_fsname,fsname_len)
      && (0 == szPath[fsname_len] || '/' == szPath[fsname_len]) )
    {
      //
      // path has a form "/dev/hda1/...."
      //
      szNewRet  = szPath + fsname_len;
      szHint    = "device";
      goto Found;
    }

    const char* szLink = GetAnotherName( mnt->mnt_fsname, bVerbose );
    if ( NULL != szLink )
    {
      size_t link_len = strlen(szLink);
      if ( path_len >= link_len
        && 0 == memcmp(szPath, szLink, link_len )
        && (0 == szPath[link_len] || '/' == szPath[link_len]) )
      {
        //
        // path has a form "/dev/hda1/...."
        //
        szNewRet  = szPath + link_len;
        szHint    = "device by link";
        goto Found;
      }
    }
  }

  fclose (f);

  if ( NULL != szRet )
  {
    if ( NULL != bMounted )
      *bMounted = true;
    if ( bVerbose )
      fprintf( stdout, "ParsePath: Path is mounted\n" );
  }
  else if ( path_len > 5
     && 0 == memcmp( "/dev/", szPath, 5 ) )
#else
  if ( path_len > 5
     && 0 == memcmp( "/dev/", szPath, 5 ) )
#endif
  {
    if ( bVerbose )
      fprintf( stdout, "ParsePath: Try to parse /dev/XXX for \"%s\"\n", szPath );

    //
    // path has a form "/dev/hda1/...."
    // and not mounted
    //

    szRet = strchr( szPath + 5, '/' );
    if ( NULL == szRet )
    {
      // path has a form "/dev/hda1"
      if ( NULL != szRelative )
        *szRelative = strdup( "/." );

      if ( NULL != szDevice )
        *szDevice = strdup( szPath );

      bDeviceOnly = true;
    }
    else for(;;)
    {
      struct stat st;
      char szTmp[FILENAME_MAX];

      size_t dev_len = NULL == szRet? path_len : (szRet - szPath);
      if ( dev_len >= ARRSIZE(szTmp) )
        return NULL;

      memcpy( szTmp, szPath, dev_len );
      szTmp[dev_len] = 0;

      int err = stat( szTmp, &st );
      if ( bVerbose )
        fprintf( stdout, "ParsePath: stat(%s) -> %d, mode=%o\n", szTmp, err, st.st_mode );

      if ( 0 == err &&  !S_ISDIR(st.st_mode) )
      {
        if ( NULL != szRelative )
          *szRelative = strdup( NULL == szRet || 0 == szRet[0]? "/." : szRet );

        if ( NULL == szRet )
          bDeviceOnly = true;

        if ( NULL != szDevice )
        {
          *szDevice = (char*)malloc( dev_len + 1 );
          if ( NULL != *szDevice )
          {
            memcpy( *szDevice, szPath, dev_len );
            (*szDevice)[dev_len] = 0;
          }
        }

        if ( bVerbose )
          fprintf( stdout, "ParsePath: Found device: %.*s  %s\n", (int)dev_len, szPath, szRet );
        break; // OK
      }
      if ( NULL == szRet )
        break;
      szRet = strchr( szRet + 1, '/' );
    }

    if ( bVerbose )
      fprintf( stdout, "ParsePath: Path is not mounted\n" );
  }

  if ( NULL != szRet )
  {
    if ( bVerbose )
      fprintf( stdout, "ParsePath: Relative path \"%s\"\n", szRet );
    return szRet;
  }

  if ( '/' == szPath[0] && !bDeviceOnly )
  {
    if ( NULL != szRelative )
      *szRelative = strdup( szPath );
    if ( NULL != szMntDir )
      *szMntDir = strdup( "/" );
    if ( NULL != bMounted )
      *bMounted = true;
    return szPath;
  }

  if ( bVerbose )
    fprintf( stdout, "ParsePath: No relative path\n" );

  return NULL;
}


///////////////////////////////////////////////////////////
// ParsePath
//
//
///////////////////////////////////////////////////////////
const char*
ParsePath(
    IN const char*  szPath,
    IN  bool        bVerbose,
    OUT char**      szDevice,
    OUT char**      szMntDir,
    OUT char**      szMntOpt,
    OUT char**      szMntType,
    OUT char**      szRelative,
    OUT bool*       bMounted
    )
{
  const char* Ret = ParsePathHlp( "/proc/mounts", szPath, bVerbose, szDevice, szMntDir, szMntOpt, szMntType, szRelative, bMounted );
#ifdef MOUNTED
  if ( NULL == Ret )
    Ret = ParsePathHlp( MOUNTED, szPath, bVerbose, szDevice, szMntDir, szMntOpt, szMntType, szRelative, bMounted );
#endif
  return Ret;
}

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
// If the function successes it returns the pointer into szPath
// which points to relative path and begins with '/'
//
// Returns NULL if not possible to parse
///////////////////////////////////////////////////////////
// const char*
// SplitPath(
//     IN const char*  szPath,
//     IN bool         bVerbose,
//     OUT bool*       bMounted       // Can be NULL
//     )
// {
//   return ParsePath( szPath, bVerbose, NULL, NULL, NULL, NULL, NULL, bMounted );
// }

#else  /* ifdef __linux__ */
#warning "No GetMount / ParsePath support"
#endif /* ifdef __linux__ */


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
    )
{
  size_t len;
  char szCurDir[FILENAME_MAX];

  if ( bVerbose )
    fprintf( stdout, "GetAbsPath input: \"%s\"\n", szRelative );

  if ( 0 == nMaxCharsInAbsPath )
  {
    fprintf( stderr, "Invalid arguments\n" );
    return false;
  }

  if ( NULL == getcwd( szCurDir, ARRSIZE(szCurDir) ) )
  {
    perror( "getcwd failed" );
    return false;
  }

  const char* last_slash = strrchr( szRelative, '/' );
  if ( NULL == last_slash )
  {
    fprintf( stderr, "Invalid relative path \"%s\"\n", szRelative );
    return false;
  }


  len = nMaxCharsInAbsPath - 1;
  if ( len > (size_t)(last_slash - szRelative) )
    len = last_slash - szRelative;

  strncpy( szAbsPath, szRelative, len );
  szAbsPath[len] = 0;
  if ( 0 != chdir( szAbsPath ) )
  {
    perror( "chdir failed" );
    fprintf( stderr, "Can't change directory to \"%s\"\n", szAbsPath );
    return false;
  }

  // Get current path
  if ( NULL == getcwd( szAbsPath, (int)nMaxCharsInAbsPath ) )
  {
    perror( "getcwd failed" );
    return false;
  }

#ifdef WIN32
  // Converts windows slashes to linux slashes
  for ( len = 0; ; len++ )
  {
    char c = szAbsPath[len];
    if ( 0 == c )
      break;
    if ( '\\' == c )
      szAbsPath[len] = '/';
  }
#else
  len = strlen( szAbsPath );
#endif

  if ( '/' != szAbsPath[len-1] )
  {
    szAbsPath[len++] = '/';
    szAbsPath[len]   = 0;
  }

  if ( len + strlen( last_slash + 1 ) >= nMaxCharsInAbsPath )
  {
    fprintf( stderr, "Input buffer %lu chars is too small\n", (unsigned long)nMaxCharsInAbsPath );
    return false;
  }

  strcpy( szAbsPath + len, last_slash + 1 );

  // Restore current directory
  if ( 0 != chdir( szCurDir ) )
  {
    perror( "chdir failed" );
    fprintf( stderr, "Can't restore original directory \"%s\"\n", szCurDir );
    return false;
  }

  if ( bVerbose )
    fprintf( stdout, "GetAbsPath output: \"%s\"\n", szAbsPath );

  return true;
}


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
    )
{
  unsigned int Bytes = 0;
  for ( ;; )
  {
    char c = *s++;
    if ( 0 == c )
      break;
    if ( '0' <= c && c <= '9' )
    {
      Bytes *= 10;
      Bytes += c - '0';
    }
    else if ( 'K' == c || 'k' == c )
      Bytes *= 1024;
    else if ( 'M' == c || 'm' == c )
      Bytes *= 1024*1024;
  }

  return Bytes;
}


#ifdef _MODULE_TEST
int main( int argc, char* argv[] )
{
  //
  // Test function GetMount and SplitPath
  //
  // /mnt/ntfs is mounted to /dev/hdb1 which is link to /dev/ide/host0/bus0/target1/lun0/part1
  //

//  GetMount( "/dev/hdb1", true );
//  GetMount( "/mnt/ntfs", true );

  SplitPath( "/mnt/ntfs/.", true );
  SplitPath( "/mnt/ntfs", true );
  SplitPath( "/dev/hdb1/.", true );
  SplitPath( "/dev/hdb1", true );

  return 0;
}
#endif
