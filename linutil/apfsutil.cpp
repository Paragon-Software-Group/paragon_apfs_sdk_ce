// <copyright file="apfsutil.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains the code that:
// - parses command line
// - calls UFSD library
// - runs desired tests
//
////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // memset
#include <assert.h>
#include <sys/stat.h>
#ifndef __STDC_LIMIT_MACROS
# define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#if UINTPTR_MAX == 0xffFFffFF
# define UFSD_ON_32BIT
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
# define UFSD_ON_64BIT
#else
# error "Unknown platform - does not look either like 32-bit or 64-bit"
#endif


#define MAX_APFS_VOLUMES 100
#define MAX_APFS_PASSPHRASE_LENGTH 128


//
// Include main ufsd header
//
#include <ufsd.h>
#ifdef UFSD_WITH_OPENSSL
# include <cipherfactory.hpp>
#endif

//
// All UFSD classes and functions are placed in UFSD namespace
//
using namespace UFSD;

#ifndef NLS_CAT_NAME
# define NLS_CAT_NAME "apfsutil"
#endif

#define PRINT_ERROR( err )  PrintUfsdStatusIfError( err )

#define CHECK_CALL( call )                  \
do{                                         \
  int Status = (call);                      \
  if ( ERR_NOERROR != Status ){             \
    PRINT_ERROR( Status );                  \
    return Status;                          \
  }                                         \
}while(0)

//
// This file contains declarations of useful UFSD-client functions
//
#include "funcs.h"
#include "gettext.h"

#ifdef _WIN32
//---------------------------------------------------------
// To compile windows utility
//---------------------------------------------------------
# define strcasecmp _stricmp

#else
//---------------------------------------------------------
// To compile linux utility
//---------------------------------------------------------

# include <unistd.h>
# include <fcntl.h>
# include <locale.h>

//---------------------------------------------------------
// End of Linux specific
//---------------------------------------------------------
#endif // _WIN32

api::ILog* UFSD_GetLog();
void EnableLogTrace( const char* ModuleName );

namespace UFSD {

CFileSystem*
CreateFsApfs(
  IN api::IBaseMemoryManager* mm,
  IN api::IBaseStringManager* ss,
  IN api::ITime* tt,
  IN api::IBaseLog* log
);

}

static bool s_bVerbose;
const char* s_szDev;
const char* s_szFile;

struct apfsutil_options
{
  const char* pass[MAX_APFS_VOLUMES];
  // TODO: options
  bool subvolumes;
};

#ifdef _WIN32

///////////////////////////////////////////////////////////
// SplitPath
//
// If the function successes it returns the pointer into szPath
// which points to relative path and begins with '/'
///////////////////////////////////////////////////////////
const char*
SplitPath(
    IN const char* szPath
    )
{
  const char* relative = strchr( szPath, ':' );
  s_szDev  = szPath;
  if ( NULL != relative )
  {
    while ( '\\' == relative[1] || '/' == relative[1] )
      ++relative;
  }
  s_szFile = NULL != relative && 0 != relative[1]? relative+1: NULL;
  return s_szFile;
}

#else

char  s_szPath[FILENAME_MAX];
char* s_szRelative;
char* s_szDevice;
char* s_mnt_dir, *s_mnt_type, *s_mnt_opts;
bool  s_bIsMounted;


///////////////////////////////////////////////////////////
// SplitPath
//
//
///////////////////////////////////////////////////////////
const char*
SplitPath(
    IN const char*  szPath
    )
{
  s_szDev  = szPath;
  if ( '/' == szPath[0] )
    strncpy( s_szPath, szPath, sizeof(s_szPath) - 1 );
  else
#ifndef __APPLE__
  if ( !GetAbsPath( szPath, s_bVerbose, s_szPath, sizeof(s_szPath) ) )
#endif
  {
    fprintf( stderr, _("Can't get absolute path for \"%s\"\n"), szPath );
    return (const char*)0x1;
  }

  // Remove last slash if any
  size_t path_len = strlen( s_szPath );
  if ( path_len > 0 && '/' == s_szPath[path_len-1] )
    s_szPath[--path_len] = 0;

  //
  // Check if it is a file
  //
  struct stat st = {0};
  int err = stat( s_szPath, &st );
  if ( s_bVerbose )
    fprintf( stdout, "SplitPath: stat(\"%s\") -> %d, mode=%o\n", s_szPath, err, st.st_mode );

  if ( 0 == err && S_ISREG( st.st_mode ) )
  {
    s_szDevice    = s_szPath;
    s_mnt_dir     = NULL;
    s_mnt_opts    = NULL;
    s_mnt_type    = NULL;
    s_szRelative  = NULL;
    s_bIsMounted  = true;
    s_szFile      = NULL;
  }
  else
  {
#if defined __linux__ || defined __APPLE__
    s_szFile = ParsePath( s_szPath, s_bVerbose,
                          &s_szDevice, &s_mnt_dir, &s_mnt_opts,
                          &s_mnt_type, &s_szRelative, &s_bIsMounted );
#else
    s_szFile = (const char*)0x1;
    fprintf( stdout, "SplitPath: Not implemented\n" );
#endif
  }
  return s_szFile;
}
#endif // #ifdef _WIN32

///////////////////////////////////////////////////////////
// DumpMemory (API function)
//
// This function Dumps memory into trace
///////////////////////////////////////////////////////////
void
DumpMemory(
    IN FILE*          fid,
    IN unsigned int   address,
    IN const void*    Mem,
    IN size_t         nBytes
    )
{
  if ( NULL == Mem || 0 == nBytes )
    return;
  const unsigned char*  Arr  = (const unsigned char*)Mem;
  const unsigned char*  Last = Arr + nBytes;
  unsigned int nRows         = (unsigned int)((nBytes + 0xF)/0x10);
  for ( unsigned int i = 0; i < nRows && Arr < Last; i++ )
  {
    char szLine[81];
    char* ptr = szLine;

    // Create address 0000, 0010, 0020, 0030, and so on
    ptr += sprintf( ptr, "%04x: ", address + i*16 );

    const unsigned char* Arr0 = Arr;
    int j;
    for ( j = 0; j < 0x10 && Arr < Last; j++, Arr++ )
      ptr += sprintf( ptr, "%02x ", (unsigned int )*Arr );

    while( j++ < 0x10 )
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
      if ( ch > 128 )
        ch -= 128;

      if ( ch < 32 || ch == 0x7f )
        ch ='.';//^= 0x40;

      *ptr++ = ch;
    }
    // Set last zero
    *ptr++ = 0;
    assert( (size_t)(ptr - szLine) < ARRSIZE(szLine) );

    fprintf( fid, "%s\n", szLine );
  }
  fprintf( fid, "\n" );
}


#ifdef UFSD_APFS_RO
#define RW_CASES "\n   The following cases are available in Paragon APFS SDK Pro:\n"
#define NLS_VERSION "Community Edition"
#else
#define RW_CASES ""
#define NLS_VERSION "Pro"
#endif

///////////////////////////////////////////////////////////
// OnUsage
//
// print a list of the parameters to the program
///////////////////////////////////////////////////////////
static void
OnUsage()
{
  fprintf( stdout, _("Demo utility on using Paragon APFS SDK\n") );
  fprintf( stdout, _(
"Usage: " NLS_CAT_NAME " <test_name> [options] /path/to/file/or/folder\n"
"\ntest_name:\n"
"   enumroot        root folder enumeration\n"
"   enumfolder      readdir example\n"
"   readfile        read selected file\n"
"   listea          list and show all file extended attributes\n"
"   listsubvolumes  sub-volumes enumeration\n"
RW_CASES
"   createfile      create file\n"
"   createfolder    create folder\n"
"   queryalloc      list file extents (allocations)\n"
"   fsinfo          file system information\n"
"\noptions:\n"
"   --passN=password  use password for the volume N (for encrypted volumes)\n"
"                     APFS volumes are listed from N=1 to 100\n"
"   --trace         turn on UFSD trace\n"
"   --subvolumes    mount all APFS subvolumes\n"
"   --version       show version and exit\n"
) );
#ifdef _WIN32
  fprintf( stdout,
_("E.g. " NLS_CAT_NAME " enumfolder f:\\testfolder\n"
  "E.g. " NLS_CAT_NAME " enumroot \\\\?\\Volume{d7c5ce70-3220-11db-b0fb-005056c00008}\n") );
#else
  fprintf( stdout, _("E.g. " NLS_CAT_NAME " readfile /dev/sdb1/test_folder/newfile.txt\n" ));
  fprintf( stdout, _("E.g. " NLS_CAT_NAME " enumfolder --subvolumes --pass1=qwerty /dev/sdb1/Ufsd_Volumes/Untitled/test_folder\n") );
#endif
}


///////////////////////////////////////////////////////////
// EnumerateFolder
//
// helper function for OnEnum...
///////////////////////////////////////////////////////////
static int
EnumerateFolder(
    IN CDir* pDir,
    IN bool  bBrief
    )
{
  if ( NULL == pDir )
    return ERR_BADPARAMS;

  // start enumeration
  CEntryNumerator* Enum = NULL;
  CHECK_CALL(pDir->StartFind( Enum, 0 ));

  FileInfo Info;

  int Status;
  // continue enumeration while success
  while ( UFSD_SUCCESS( Status = pDir->FindNext( Enum, Info ) ) )
  {
    if (bBrief )
    {
      fprintf( stdout, "%s ", (char*)Info.Name );
      continue;
    }

    // set attributes
    char Attr[] = "-rwxrw-rw-";
    Attr[0] = U_ISDIR(Info.Mode) ? 'd' :
              U_ISLNK(Info.Mode) ? 'l' :
              U_ISSOCK(Info.Mode)? 's' :
              U_ISBLK(Info.Mode) ? 'b' :
              U_ISCHR(Info.Mode) ? 'c' : '-';

    Attr[1] = FlagOn(Info.Mode, U_IRUSR) ? 'r' : '-';
    Attr[2] = FlagOn(Info.Mode, U_IWUSR) ? 'w' : '-';
    Attr[3] = FlagOn(Info.Mode, U_IXUSR) ? 'x' : '-';

    Attr[4] = FlagOn(Info.Mode, U_IRGRP) ? 'r' : '-';
    Attr[5] = FlagOn(Info.Mode, U_IWGRP) ? 'w' : '-';
    Attr[6] = FlagOn(Info.Mode, U_IXGRP) ? 'x' : '-';

    Attr[7] = FlagOn(Info.Mode, U_IROTH) ? 'r' : '-';
    Attr[8] = FlagOn(Info.Mode, U_IWOTH) ? 'w' : '-';
    Attr[9] = FlagOn(Info.Mode, U_IXOTH) ? 'x' : '-';

    // print file info
    assert( Info.NameType == api::StrUTF8 );
    fprintf( stdout, "%s %o %5u %4u %4u %8" PLL "u %s\n",
      Attr, Info.Mode & 0x1FF, Info.HardLinks, Info.Uid, Info.Gid, Info.FileSize, (char*)Info.Name);
  }

  if ( bBrief )
    fprintf( stdout, "\n" );

  // free resources
  Enum->Destroy();

  return Status == ERR_NOFILEEXISTS ? ERR_NOERROR : Status;
}


///////////////////////////////////////////////////////////
// OnEnumRoot
//
// readdir example: shows all files in the root folder
///////////////////////////////////////////////////////////
static int
OnEnumRoot(
    IN CFileSystem* fs,
    IN const char*
    )
{
  fprintf( stdout, "Dir content:\n" );
  return EnumerateFolder(fs->m_RootDir, false);
}


///////////////////////////////////////////////////////////
// GetParent
//
// helper function
///////////////////////////////////////////////////////////
static CDir* GetParent(
    IN CDir*        Root,
    IN const char*& Path
    )
{
  if ( NULL == Path )
    return Root;

  if ( '/' == *Path )
    ++Path;

  const char* slash = strchr( Path, '/' );

  if ( slash == NULL )
    return Root;

  CDir* Parent;
  int Status = Root->OpenDir( api::StrUTF8, Path, slash - Path, Parent );

  if ( !UFSD_SUCCESS( Status ) )
    return NULL;

  Path = slash + 1;

  return GetParent( Parent, Path );
}


///////////////////////////////////////////////////////////
// OpenFile
//
// helper function
///////////////////////////////////////////////////////////
static int
OpenFile(
    IN  CFileSystem*  fs,
    IN  const char*   Path,
    OUT CFile*&       File,
    OUT CDir*&        Parent
    )
{
  if ( NULL == Path )
  {
    fprintf( stderr, "openfile: No file specified\n" );
    return ERR_BADPARAMS;
  }

  Parent = GetParent( fs->m_RootDir, Path );

  if ( NULL == Parent || NULL == Path )
    return ERR_BADPARAMS;

  int Status = Parent->OpenFile( api::StrUTF8, Path, fs->m_Strings->strlen( api::StrUTF8, Path ), File );

  if ( !UFSD_SUCCESS( Status ) && NULL != Parent->m_Parent )
    Parent->Destroy();

  return Status;
}


///////////////////////////////////////////////////////////
// CloseFile
//
// helper function
///////////////////////////////////////////////////////////
static void
CloseFile(
    IN CFile* File,
    IN CDir*  Parent
    )
{
  if ( NULL != File )
    File->Destroy();

  if ( NULL != Parent && NULL != Parent->m_Parent )
    Parent->Destroy();
}


///////////////////////////////////////////////////////////
// OnEnumFolder
//
// readdir example: open and enumerate selected folder
///////////////////////////////////////////////////////////
static int
OnEnumFolder(
    IN CFileSystem* fs,
    IN const char*  Path
    )
{
  CDir* pWorkDir = NULL;
  int Status = ERR_NOERROR;

  if ( NULL == Path )
    pWorkDir = fs->m_RootDir;
  else
  {
    CDir* Parent = GetParent( fs->m_RootDir, Path );

    if ( NULL == Parent || NULL == Path )
      return ERR_BADPARAMS;

    Status = Parent->OpenDir( api::StrUTF8, Path, fs->m_Strings->strlen( api::StrUTF8, Path ), pWorkDir );
  }

  if ( UFSD_SUCCESS( Status ) )
  {
    fprintf( stdout, "Dir content:\n" );
    Status = EnumerateFolder(pWorkDir, false);

    if ( NULL != pWorkDir->m_Parent )
      pWorkDir->Destroy();
  }

  return Status;
}


///////////////////////////////////////////////////////////
// OnReadFile
//
// file reading example: open and read selected file
///////////////////////////////////////////////////////////
static int
OnReadFile(
    IN CFileSystem* fs,
    IN const char*  Path
    )
{
  CDir* Parent;
  CFile* File;

  CHECK_CALL( OpenFile( fs, Path, File, Parent ) );

  const size_t BufSize = 0x1000;
  void* pReadBuf = Zalloc2( BufSize );

  if ( NULL == pReadBuf )
    return ERR_NOMEMORY;

  UINT64 Offset = 0;
  size_t Bytes;
  int Status;

  while ( UFSD_SUCCESS( Status = File->Read( Offset, Bytes, pReadBuf, BufSize )) && Bytes != 0 )
  {
    fprintf( stdout, "read: %" PLL "x + %" PZZ "x -> %" PZZ "x (%x)\n", Offset, BufSize, Bytes, Status );
    DumpMemory( stdout, (unsigned)Offset, pReadBuf, Bytes );

    Offset += Bytes;
  }

  Free2( pReadBuf );

  CloseFile( File, Parent );
  return Status;
}


///////////////////////////////////////////////////////////
// OnListEa
//
// extended attributes listing
///////////////////////////////////////////////////////////
static int
OnListEa(
    IN CFileSystem* fs,
    IN const char*  Path
    )
{
  CDir* Parent;
  CFile* File;

  CHECK_CALL( OpenFile( fs, Path, File, Parent ) );

  size_t Bytes;
  size_t BufSize = 0;
  void* pBuffer = NULL;
  int Status;

  while ( UFSD_SUCCESS( Status = File->ListEa( pBuffer, BufSize, &Bytes ) ) && 0 != Bytes )
  {
    if ( NULL != pBuffer )
      break;

    BufSize = Bytes;
    pBuffer = Zalloc2( BufSize + 1);  // HACK: +1 zero byte to mark the last attribute

    if ( NULL == pBuffer )
    {
      Status = ERR_NOMEMORY;
      break;
    }
  }

  if ( UFSD_SUCCESS( Status ) )
  {
    const char* xAttr = static_cast<const char*>(pBuffer);

    if ( NULL != xAttr )
    {
      while ( *xAttr != '\0' )
      {
        void* Data = NULL;
        BufSize = 0;
        size_t AttrNameLen = strlen( xAttr );

        while ( UFSD_SUCCESS( Status = File->GetEa( xAttr, AttrNameLen, Data, BufSize, &Bytes ) ) && 0 != Bytes )
        {
          if ( NULL != Data )
            break;

          BufSize = Bytes;
          Data = Zalloc2( BufSize );

          if ( NULL == Data )
          {
            Status = ERR_NOMEMORY;
            break;
          }
        }

        fprintf( stdout, "%s\n", xAttr );
        DumpMemory( stdout, 0, Data, BufSize );
        xAttr += AttrNameLen + 1;

        Free2( Data );
      }
    }
  }
  Free2( pBuffer );

  CloseFile( File, Parent );
  return Status;
}


///////////////////////////////////////////////////////////
// OnEnumSubvolumes
//
//
///////////////////////////////////////////////////////////
static int
OnEnumSubvolumes(
  IN CFileSystem* fs,
  IN const char*
  )
{
  fprintf( stdout, "Volumes:\n" );

  char MainVolName[MAX_FILENAME];
  CHECK_CALL(fs->GetVolumeInfo( NULL, NULL, NULL, NULL, 0, NULL, api::StrUTF8, MainVolName, MAX_FILENAME ));

  fprintf( stdout, "%s ", MainVolName );

  const char* DirName = "Ufsd_Volumes";
  CDir* VolumeRoot;

  CHECK_CALL( fs->m_RootDir->OpenDir( api::StrUTF8, DirName, fs->m_Strings->strlen( api::StrUTF8, DirName ), VolumeRoot ) );

  int Status = EnumerateFolder( VolumeRoot, true );
  VolumeRoot->Destroy();

  return Status;
}

///////////////////////////////////////////////////////////
// OnCreateFile
//
//
///////////////////////////////////////////////////////////
static int
OnCreateFile(
  IN CFileSystem* fs,
  IN const char* Path
  )
{
  CDir* Parent = GetParent( fs->m_RootDir, Path );

  if ( NULL == Parent || NULL == Path )
  {
    fprintf( stderr, "wrong path: %s\n", Path );
    return ERR_NOFILEEXISTS;
  }

  CFile* File = NULL;
  int Status = Parent->CreateFile( api::StrUTF8, Path, fs->m_Strings->strlen(api::StrUTF8, Path), &File );

  CloseFile( File, Parent );

#ifdef UFSD_APFS_RO
  assert( ERR_WPROTECT == Status );
  Status = ERR_NOTIMPLEMENTED;
#endif

  return Status;
}


///////////////////////////////////////////////////////////
// OnCreateFolder
//
//
///////////////////////////////////////////////////////////
static int
OnCreateFolder(
  IN CFileSystem* fs,
  IN const char* Path
  )
{
  CDir* Parent = GetParent( fs->m_RootDir, Path );

  if ( NULL == Parent || NULL == Path )
  {
    fprintf( stderr, "wrong path: %s\n", Path );
    return ERR_NOFILEEXISTS;
  }

  int Status = Parent->CreateDir( api::StrUTF8, Path, fs->m_Strings->strlen( api::StrUTF8, Path ) );

  if ( NULL != Parent->m_Parent )
    Parent->Destroy();

#ifdef UFSD_APFS_RO
  assert( ERR_WPROTECT == Status );
  Status = ERR_NOTIMPLEMENTED;
#endif

  return Status;
}


///////////////////////////////////////////////////////////
// OnQueryAlloc
//
// file query allocation ranges
///////////////////////////////////////////////////////////
static int
OnQueryAlloc(
  IN CFileSystem* fs,
  IN const char* Path
  )
{
  CDir* Parent;
  CFile* File;

  CHECK_CALL( OpenFile( fs, Path, File, Parent ) );

  UFSD::UFSD_GET_RETRIEVAL_POINTERS Arg;
  size_t BufSize = 0x10000;

  Memzero2( &Arg, sizeof( Arg ) );

  Arg.Handle.FsObject = File;
  Arg.Flags = UFSD_GET_ALLOCATED;
#ifdef UFSD_APFS_RO
  int Status = ERR_NOTIMPLEMENTED;
#else
  int Status = ERR_MORE_DATA;
#endif
  UFSD::RETRIEVAL_POINTERS_BUFFER* pExtents = NULL;

  pExtents = (UFSD::RETRIEVAL_POINTERS_BUFFER*)Zalloc2( BufSize );
  if ( pExtents == NULL )
    return ERR_NOMEMORY;

  bool extent_exist = false;
  UINT64 Vcn = 0;
  unsigned int n = 0;

  while ( Status == ERR_MORE_DATA )
  {
    size_t Bytes;
    Status = fs->IoControl( UFSD::IOCTL_GET_RETRIEVAL_POINTERS2, &Arg, sizeof( Arg ), pExtents, BufSize, &Bytes );

    for ( size_t i = 0; i < pExtents->ExtentCount; ++i )
    {
      if ( !extent_exist )
      {
        extent_exist = true;
        fprintf( stdout, "   #            LCN          COUNT\n" );
      }
      fprintf( stdout, "%4u %14" PLL "x %14" PLL "x\n", n++, pExtents->Extents[i].Lcn, pExtents->Extents[i].NextVcn - Vcn);
      Vcn = pExtents->Extents[i].NextVcn;
    }

    if ( Status == ERR_MORE_DATA )
      Arg.StartingVcn = pExtents->Extents[pExtents->ExtentCount - 1].NextVcn;
  }

  Free2( pExtents );

  CloseFile( File, Parent );

  if ( Status != ERR_NOTIMPLEMENTED && !extent_exist )
    fprintf( stdout, "File does not have extents on disk\n" );

  return Status;
}


///////////////////////////////////////////////////////////
// OnFsInfo
//
// get information about the file system
///////////////////////////////////////////////////////////
static int
OnFsInfo(
  IN CFileSystem* fs,
  IN const char*
  )
{
  const unsigned short MaxApfsVolumes = 100;
  const size_t BufSize = sizeof( UFSD_VOLUME_APFS_INFO ) * MaxApfsVolumes;
  UFSD_VOLUME_APFS_INFO* pBuffer = (UFSD_VOLUME_APFS_INFO*)malloc( BufSize );

  if ( NULL == pBuffer )
  {
    fprintf( stderr, _( "No memory\n" ) );
    return -1;
  }

  size_t Bytes;
  int Status = fs->IoControl( IOCTL_GET_APFS_INFO, NULL, 0, pBuffer, BufSize, &Bytes );

  if ( UFSD_SUCCESS( Status ) )
  {
    UFSD_VOLUME_APFS_INFO* Info = (UFSD_VOLUME_APFS_INFO*)pBuffer;
    unsigned short VolCnt = 0;

    for ( unsigned short i = 0; i < MaxApfsVolumes && sizeof( UFSD_VOLUME_APFS_INFO ) * VolCnt < Bytes; ++i )
    {
      unsigned char* s = Info->SerialNumber;
      fprintf( stdout,
               _( "APFS Volume [%u] %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n"
                  "  Name               : %s (Case-%ssensitive)\n"
                  "  Creator            : %s\n"
                  "  Used Clusters      : %" PLL "u\n"
                  "  Reserved Clusters  : %" PLL "u\n"
                  "  Files              : %" PLL "u\n"
                  "  Directories        : %" PLL "u\n"
                  "  Symlinks           : %" PLL "u\n"
                  "  Special files      : %" PLL "u\n"
                  "  Snapshots          : %" PLL "u\n"
                  "  Encrypted          : %s\n\n" ),
               Info->Index,
               s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15],
               Info->VolumeName, Info->CaseSensitive ? "" : "in",
               Info->Creator,
               Info->BlocksUsed, Info->BlocksReserved,
               Info->FilesCount, Info->DirsCount, Info->SymlinksCount, Info->SpecFilesCount, Info->SnapshotsCount,
               Info->Encrypted ? "Yes" : "No"
      );

      ++VolCnt;
      ++Info;
    }
  }

  free( pBuffer );

#ifdef UFSD_APFS_RO
  Status = ERR_NOTIMPLEMENTED;
#endif

  return Status;
}



typedef int (*HandlerFunc)(CFileSystem*, const char*);

struct t_CmdHandler{
  const char*   cmd_text;
  HandlerFunc   handler;
};


static const t_CmdHandler s_Cmd[] = {
  { "enumroot"        , OnEnumRoot         },   // readdir example
  { "enumfolder"      , OnEnumFolder       },   // enumerate folder
  { "readfile"        , OnReadFile         },   // file reading
  { "listea"          , OnListEa           },   // list all extended attributes
  { "listsubvolumes"  , OnEnumSubvolumes   },   // sub-volumes enumeration
  // handlers for RW version
  { "createfile"      , OnCreateFile       },   // create file
  { "createfolder"    , OnCreateFolder     },   // create folder
  { "queryalloc"      , OnQueryAlloc       },   // list file extents (allocations)
  { "fsinfo"          , OnFsInfo           },   // get file system information
  //
  // TODO: add your handlers here
  // ...
  { NULL      , NULL },
};


///////////////////////////////////////////////////////////
// parse_options
//
// Full trusted function // used to be
// Returns 0 if error
///////////////////////////////////////////////////////////
static bool
parse_options(
  IN int argc,
  IN const char* argv[],
  OUT struct apfsutil_options* opts
  )
{
  int i;

  if ( argc <= 1 )
    return false;

  memset( opts, 0, sizeof( apfsutil_options ) );

  for ( i = 1; i < argc; i++ )
  {
    const char* a = argv[i];
    if ( 0 == strcmp( "--version", a ) )
    {
      fprintf( stdout, _(NLS_CAT_NAME ": " NLS_VERSION " version " PACKAGE_TAG "\nParagon Software Group\n\n") );
      exit( 1 );
    }

    if ( 0 == strcmp( "--trace", a ) )
      EnableLogTrace( NLS_CAT_NAME );
    else if ( 0 == strcmp( "--subvolumes", a ) )
      opts->subvolumes = true;
    else if ( 0 == strncmp( "--pass", a, 6 ) )
    {
#ifndef UFSD_WITH_OPENSSL
      fprintf( stderr, "--pass option doesn't work without OpenSSL\n" );
      exit( -1 );
#endif
      char* end = NULL;
      unsigned int v = strtoul( a + 6, &end, 0 );

      if ( v > MAX_APFS_VOLUMES || v == 0 || *end != '=' )
      {
        fprintf( stderr, "Wrong volume Id in the option %s\n", a );
        exit( -5 );
      }

      size_t l = strlen( end + 1 );

      if ( l > MAX_APFS_PASSPHRASE_LENGTH )
      {
        fprintf( stderr, "Wrong password length (%" PZZ "u) in the option %s\n", l, a );
        exit( -5 );
      }

      opts->pass[v-1] = end + 1;
    }
    else if ( 0 == strcmp( "--help", a ) || 0 == strcmp( "-h", a ) )
      return false; // Force to call OnUsage
  }

  return true;
}


///////////////////////////////////////////////////////////
// main
//
//
///////////////////////////////////////////////////////////
int __cdecl
main(
    IN int          argc,
    IN const char*  argv[]
    )
{
  api::IDeviceRWBlock* Rw     = NULL;
  int                  Ret    = 0;
  apfsutil_options     opts;

#ifndef _WIN32
  setlocale(LC_MESSAGES, "");
  setlocale(LC_CTYPE, "");
  bindtextdomain(NLS_CAT_NAME, LOCALEDIR);
  textdomain(NLS_CAT_NAME);
#endif

  const char* szCmdName = argv[1];
  char szDevice[FILENAME_MAX] = { 0 };

  // Parse command line
  // And exit if there are errors
  if ( !parse_options( argc, argv, &opts ) || szCmdName[0] == '-' )
  {
    OnUsage();
    return -1;
  }

  if ( SplitPath( argv[argc-1] ) == (const char*)0x1 )
  {
    OnUsage();
    return -1;
  }

#ifdef _WIN32
  if (':' != s_szDev[1])
  {
    // Copy as is
    strncpy(szDevice, s_szDev, ARRSIZE(szDevice));
  }
  else
  {
    // include ":"
    snprintf(szDevice, ARRSIZE(szDevice), "%C:", s_szDev[0]);
    szDevice[ARRSIZE(szDevice) - 1] = 0;
  }

  if ( NULL != s_szFile )
  {
    char* c = const_cast<char*>(s_szFile);
    while ( *c )
    {
      if ( '\\' == *c )
        * c = '/';
      ++c;
    }

    size_t path_len = strlen( s_szFile );
    c = const_cast<char*>(s_szFile);
    while( path_len > 0 )
    {
      if ( '/' == c[path_len - 1] )
      {
        c[path_len - 1] = 0;
        --path_len;
      }
      else
        break;
    }
  }
#else
  if ( NULL != s_szDevice )
    strncpy(szDevice, s_szDevice, ARRSIZE(szDevice));
#endif

  if ( 0 == *szDevice )
  {
    fprintf( stderr, "wrong path: %s\n", szDevice );
    return -2;
  }

#if !defined _WIN32 && !defined __QNX__
  //
  // Check for mounted devices
  //
  const char* szMount = GetMount( s_szDevice, false );
  if ( NULL != szMount )
  {
    fprintf( stderr, _("\nThe device \"%s\" is mounted as \"%s\"\nImpossible to run test\n\n"), s_szDevice, szMount );
    return -3;
  }
#endif

#ifdef UFSD_APFS_RO
  bool bReadOnly = true;
#else
  bool bReadOnly = false;
#endif

  //
  // Seek command to run
  //
  HandlerFunc h = NULL;
  const t_CmdHandler* t = s_Cmd;

  while ( NULL != t->cmd_text && NULL == h )
  {
    if ( 0 == strcasecmp( t->cmd_text, szCmdName ) )
      h = t->handler;
    else
      ++t;
  }

  if ( NULL == h )
  {
    fprintf( stderr, "No handler specified for command '%s'\n", szCmdName );
    return -4;
  }

  //
  // Try device
  //
  int Status = UFSD_IOHandlerCreate( szDevice, bReadOnly, false, &Rw, NULL, false, false, 0 );

  if ( !UFSD_SUCCESS( Status ) )
  {
    // Here Status is an 'errno'
    // Return it in negative form
    Ret = -(int)Status;
  }
  else
  {
    assert( NULL != Rw );

    //
    // Call UFSD code
    //
    api::IBaseMemoryManager* Mm = UFSD_GetMemoryManager();
    api::IBaseStringManager* Ss = UFSD_GetStringsService();
    api::ITime* Tt = UFSD_GetTimeService();
    api::IBaseLog* Log = UFSD_GetLog();
    CFileSystem* fs = CreateFsApfs( Mm, Ss, Tt, Log );

    if ( NULL == fs )
    {
      fprintf( stderr, "Failed to init APFS" );
      Status = ERR_NOMEMORY;
    }
    else
    {
      //
      // Call PreInit to set additional parameters like password list and cipher factory
      // for encrypted volumes
      //
      PreInitParams params;
      Memzero2( &params, sizeof( params ) );

      params.FsType = FS_APFS;
      params.PwdList = const_cast<char**>(opts.pass);
      params.PwdSize = MAX_APFS_VOLUMES;
#ifdef UFSD_WITH_OPENSSL
      cipher::CCipherFactory factory(NULL);
      params.Cf = &factory;
#endif

      //
      // Ready to initialize the file system
      //
      size_t Options = 0;
#ifdef UFSD_ON_32BIT
      if ( opts.subvolumes || h == OnEnumSubvolumes )
      {
        fprintf( stderr, "All subvolumes can only be accessed with 64-bit builds\n" );
        Status = ERR_NOTIMPLEMENTED;
      }
      else
      {
#endif
      if ( opts.subvolumes || h == OnEnumSubvolumes || h == OnFsInfo )
        SetFlag( Options, UFSD_OPTIONS_MOUNT_ALL_VOLUMES );

      size_t Flags;
      if ( !UFSD_SUCCESS( Status = fs->Init( &Rw, 1, Options, &Flags, &params ) ) || ( FlagOn( Flags, UFSD_FLAGS_BAD_PASSWORD ) ) )
      {
        if ( FlagOn( Flags, UFSD_FLAGS_BAD_PASSWORD ) )
        {
          fprintf( stderr, "Wrong password was specified for encrypted volume\n" );
          Status = ERR_BADPARAMS;
        }
        else if ( Status == ERR_FSUNKNOWN && FlagOn( Flags, UFSD_FLAGS_ENCRYPTED_VOLUMES ) )
        {
          fprintf( stderr, "No password was specified for encrypted volume\n" );
          Status = ERR_BADPARAMS;
        }
        else
          // file system was not recognized
          fprintf( stderr, "UFSD::Init returns %x\n", Status );
      }
      else
      {
        assert( NULL != fs && NULL != fs->m_RootDir );

        UINT64 T0 = Tt->Time();

        Status = (*h)(fs, s_szFile);
        fprintf( stdout, "APFS: %s returns %x. finished in %u ms\n",
                 szCmdName, Status, static_cast<unsigned int>((Tt->Time() - T0) * 1000U / api::ITime::TicksPerSecond) );
      }

      //
      // Destroy file system object
      //
      fs->Destroy();
#ifdef UFSD_ON_32BIT
      }
#endif

      if ( !UFSD_SUCCESS( Status ) )
        Ret = -(int)Status;
    }
  }

  //
  // Free all resources
  //
  if ( NULL != Rw )
    Rw->Destroy();

  //
  // Translate error code to string
  //
  PrintUfsdStatusIfError( Status, NLS_CAT_NAME );

  // Exit to host OS
  return Ret;
}
