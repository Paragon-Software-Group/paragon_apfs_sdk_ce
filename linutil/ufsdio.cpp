// <copyright file="ufsdio.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file implements CRWBlock for UFSD library
//
////////////////////////////////////////////////////////////////
#define _FILE_OFFSET_BITS 64
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#if defined __QNX__ && defined __QNXNTO__
  #include <sys/cam_device.h>
  #include <sys/dcmd_cam.h>
  #include <sys/dcmd_blk.h>
#endif

#include <stdio.h>
#if defined __APPLE__ || defined __QNX__
  #include <stdlib.h>
  #include <sys/disk.h>
#elif defined __FreeBSD__
  #include <stdlib.h>
#else
  #include <malloc.h>
#endif
#include <errno.h>
//#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
  //
  // Windows specific headers and defines are declared here
  //
  #include <io.h>

  #define open64  _open
  #define close   _close
  #define lseek64 _lseeki64
  #define __off64_t_defined
  static inline int pread64( int h, void *b, size_t c, __int64 off ) {
    if ( _lseeki64( h, off, 0 ) < 0 )
      return -1;
    return _read( h, b, (unsigned)c );
  }
  static inline int pwrite64( int h, const void *b, size_t c, __int64 off ) {
    if ( _lseeki64( h, off, 0 ) < 0 )
      return -1;
    return _write( h, b, (unsigned)c );
  }
  #define R_OK  4

  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #pragma warning( disable: 4201 ) // nonstandard extension used : nameless struct/union
  #include <winioctl.h>

  #ifndef IOCTL_DISK_GET_PARTITION_INFO_EX

    #define IOCTL_DISK_GET_PARTITION_INFO_EX    CTL_CODE(IOCTL_DISK_BASE, 0x0012, METHOD_BUFFERED, FILE_ANY_ACCESS)
//    #define IOCTL_DISK_SET_PARTITION_INFO       CTL_CODE(IOCTL_DISK_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
    #define IOCTL_DISK_SET_PARTITION_INFO_EX    CTL_CODE(IOCTL_DISK_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
    typedef enum _PARTITION_STYLE {
        PARTITION_STYLE_MBR,
        PARTITION_STYLE_GPT,
        PARTITION_STYLE_RAW
    } PARTITION_STYLE;

    typedef struct _PARTITION_INFORMATION_MBR {
        BYTE  PartitionType;
        BOOLEAN BootIndicator;
        BOOLEAN RecognizedPartition;
        DWORD HiddenSectors;
    } PARTITION_INFORMATION_MBR, *PPARTITION_INFORMATION_MBR;


    //
    // The following structure defines information in a GPT partition that is
    // not common to both GPT and MBR partitions.
    //

    typedef struct _PARTITION_INFORMATION_GPT {
        GUID PartitionType;                 // Partition type. See table 16-3.
        GUID PartitionId;                   // Unique GUID for this partition.
        DWORD64 Attributes;                 // See table 16-4.
        WCHAR Name [36];                    // Partition Name in Unicode.
    } PARTITION_INFORMATION_GPT, *PPARTITION_INFORMATION_GPT;


    typedef struct _PARTITION_INFORMATION_EX {
        PARTITION_STYLE PartitionStyle;
        LARGE_INTEGER StartingOffset;
        LARGE_INTEGER PartitionLength;
        DWORD PartitionNumber;
        BOOLEAN RewritePartition;
        union {
            PARTITION_INFORMATION_MBR Mbr;
            PARTITION_INFORMATION_GPT Gpt;
        };
    } PARTITION_INFORMATION_EX, *PPARTITION_INFORMATION_EX;

    typedef SET_PARTITION_INFORMATION SET_PARTITION_INFORMATION_MBR;

    typedef struct _SET_PARTITION_INFORMATION_EX {
      PARTITION_STYLE PartitionStyle;
      union {
        SET_PARTITION_INFORMATION_MBR Mbr;
//        SET_PARTITION_INFORMATION_GPT Gpt;
      };
    } SET_PARTITION_INFORMATION_EX, *PSET_PARTITION_INFORMATION_EX;

  #endif

  #if defined(__MINGW32__) && !defined SET_PARTITION_INFORMATION_EX && !defined UFSD_WITH_SET_PARTITION
    typedef struct _SET_PARTITION_INFORMATION_EX {
        PARTITION_STYLE PartitionStyle;
        union {
          SET_PARTITION_INFORMATION Mbr;
  //        SET_PARTITION_INFORMATION_GPT Gpt;
        };
      } SET_PARTITION_INFORMATION_EX, *PSET_PARTITION_INFORMATION_EX;
  #endif

  #ifdef _CONSOLE
    #define _Trace(a) fprintf a
  #endif

  #ifndef S_IRUSR
    #define S_IRUSR 0
    #define S_IWUSR 0
  #endif
  #ifndef S_IRGRP
    #define S_IRGRP 0
    #define S_IWGRP 0
  #endif

#else

  //
  // Linux specific headers and defines are declared here
  //
  #include <string.h> // strerror
  #include <unistd.h>
  #ifdef __APPLE__
    #undef BLKSSZGET
    #define BLKSSZGET DKIOCGETBLOCKSIZE
  #elif defined __FreeBSD__
    #include <sys/disk.h>
  #else
    #include <malloc.h>
    #ifndef __QNX__
      #include <linux/hdreg.h>
      #include <linux/fs.h>
    #endif
  #endif
  #include <sys/ioctl.h>

#ifndef __FreeBSD__ // Set "#if 0" to turn off default ioctl values
  #ifndef BLKPBSZGET
    #pragma message "BLKPBSZGET is not defined! include necessary files"
    #define BLKPBSZGET _IO(0x12,123)
  #endif

  #ifndef BLKSSZGET
    #pragma message "BLKSSZGET is not defined! include necessary files"
    #define BLKSSZGET  _IO(0x12,104)    // get block device sector size
  #endif

  #ifndef BLKDISCARDZEROES
    #pragma message "BLKDISCARDZEROES is not defined! include necessary files"
    #define BLKDISCARDZEROES _IO(0x12,124)
  #endif

  #ifndef BLKDISCARD
    #pragma message "BLKDISCARD is not defined! include necessary files"
    #define BLKDISCARD    _IO(0x12,119)
  #endif

  #ifndef BLKZEROOUT
    #pragma message "BLKZEROOUT is not defined! include necessary files"
    #define BLKZEROOUT _IO(0x12,127)
  #endif
#endif

  // Add missing defines.
  #ifndef O_BINARY
    #define O_BINARY 0
  #endif

  #define _Trace(a) fprintf a

#endif

#include <api/assert.hpp>
#include <ufsd.h>

#ifndef _Trace
  #define _Trace(a) do{}while((void)0,0)
#endif

#ifndef IN
# define IN
# define OUT
#endif

#ifdef __ANDROID__
  static inline int pread64(int h, void *b, unsigned int c, long long off) {
    if ( lseek64(h, off, SEEK_SET) < 0 )
      return -1;
    return read(h, b, c);
  }
  static inline int pwrite64(int h, const void *b, unsigned int c, long long off) {
    if ( lseek64(h, off, SEEK_SET) < 0 )
      return -1;
    return write(h, b, c);
  }
  #define open64 open
#else
  #ifndef __off64_t_defined
    #define open64 open
    #define lseek64 lseek
    #define pread64 pread
    #define pwrite64 pwrite
  #endif
#endif


//=============================================================================
//                        CUFSD_RWBlock
//=============================================================================
struct CUFSD_RWBlock : public api::IDeviceRWBlock, public base_noncopyable
{
  int               m_hFile;
  bool              m_bReadOnly;
  bool              m_bNoDiscard;  // If not zero then skip discard volume
#ifdef _WIN32
  bool              m_bLocked;
#else
  bool              m_bZeroes;      // true if volume supports discard+zero request
  bool              m_bWarnPrinted; // true if warning is printed about zeroing
  bool              m_bBlockDevice; // true for block devices
#endif
  bool              m_bVerbose;     // verbose actions
  bool              m_bNoMedia;     // No media occurs

  unsigned int      m_BytesPerSector;
  UINT64            m_Size;         // In bytes
  char*             m_szDevice;     // copy of device name

  CUFSD_RWBlock( IN bool bReadOnly, IN bool bNoDiscard )
    : m_hFile(-1)
    , m_bReadOnly(bReadOnly)
    , m_bNoDiscard(bNoDiscard)
#ifdef _WIN32
    , m_bLocked(false)
#else
    , m_bZeroes(false)
    , m_bWarnPrinted(false)
    , m_bBlockDevice(true)
    , m_bVerbose(false)
#endif
    , m_bNoMedia( false )
    , m_BytesPerSector(512)
    , m_Size(0)
    , m_szDevice(NULL)
  {
  }

  ~CUFSD_RWBlock()
  {
    if ( -1 != m_hFile )
    {
#ifdef _WIN32
      if ( m_bLocked )
      {
        DWORD Tmp;
        HANDLE hFile = (HANDLE)::_get_osfhandle(m_hFile);
        // dismount the volume
        ::DeviceIoControl( hFile, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &Tmp, NULL );
        // unlock the volume
        // Really this operation is not required 'cause close( m_hFile )
        ::DeviceIoControl( hFile, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &Tmp, NULL );
        ::FlushFileBuffers( hFile );
      }
#else
//      fdatasync( m_hFile );
      fsync( m_hFile );
#endif
      close( m_hFile );
    }
    free( m_szDevice );
  }

  int Init(
      IN const char*  szDevice,
      IN bool         bForceDismount,
      IN bool         bVerbose,
      OUT int*        fd,
      IN unsigned int NewBytesPerSector
      );

  //=============================================
  //    api::IDeviceRWBlock virtual functions
  //=============================================

  // returns non zero for read-only mode
  virtual int IsReadOnly() const
  {
    return m_bReadOnly;
  }

  virtual unsigned int GetSectorSize() const
  {
    return m_BytesPerSector;
  }

  virtual UINT64 GetNumberOfBytes() const
  {
    return m_Size;
  }

  // Discard range of bytes (known as trim for SSD)
  virtual int DiscardRange(
      IN  const UINT64& Offset,
      IN  const UINT64& Bytes
      );

  virtual int ReadBytes(
      IN const UINT64&  Offset,
      IN void*          Buffer,
      IN size_t         Bytes,
      IN unsigned int   Flags
      );

  virtual int WriteBytes(
      IN const UINT64&  Offset,
      IN const void*    Buffer,
      IN size_t         Bytes,
      IN unsigned int   Flags
      );

  virtual int Flush( unsigned int )
  {
    if ( -1 != m_hFile )
    {
#ifdef _WIN32
      ::FlushFileBuffers( (HANDLE)::_get_osfhandle( m_hFile ) );
#else
//      fdatasync( m_hFile );
      fsync( m_hFile );
#endif
    }
    return 0;
  }

  // Function like DeviceIoControl
  virtual int IoControl(
      IN  size_t          ,//IoControlCode,
      IN  const void*     ,//InBuffer        = NULL, // OPTIONAL
      IN  size_t          ,//InBuffSize      = 0,    // OPTIONAL
      OUT void*           ,//OutBuffer       = NULL, // OPTIONAL
      IN  size_t          ,//OutBuffSize     = 0,    // OPTIONAL
      OUT size_t*         //BytesReturned   = NULL  // OPTIONAL
      )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Close()
  {
    return -1;
  }

  void Destroy()
  {
    delete this;
  }

  //=============================================
  //   End of api::IDeviceRWBlock
  //=============================================
};


///////////////////////////////////////////////////////////
// CUFSD_RWBlock::Init
//
//
///////////////////////////////////////////////////////////
int
CUFSD_RWBlock::Init(
    IN const char*  szDevice,
    IN bool         bForceDismount,
    IN bool         bVerbose,
    OUT int*        fd,
    IN unsigned int NewBytesPerSector
    )
{
#ifdef _WIN32
  char szName[260];
  if ( ':' == szDevice[1] )
    _snprintf( szName, ARRSIZE(szName), "\\\\.\\%s", szDevice );
  else
    _snprintf( szName, ARRSIZE(szName), "%s", szDevice );
  szName[ARRSIZE(szName)-1] = 0;
#else
  const char* szName = szDevice;
#endif

  m_bVerbose = bVerbose;
#if defined _WIN32 & defined _CONSOLE
  int Attempts = 0;
Again:
#endif
  m_hFile = open64( szName,
                    O_BINARY | (m_bReadOnly? O_RDONLY : O_RDWR),
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );
  if ( -1 == m_hFile )
  {
    int err = errno;
    _Trace(( stderr, "Can't open \"%s\" : %s\n", szDevice, strerror(err) ));
    return err;
  }

  // Get the size of file
#ifdef __APPLE__
  ioctl( m_hFile, DKIOCGETBLOCKCOUNT, &m_Size );
#else
  lseek64( m_hFile, 0, SEEK_SET );
  m_Size = lseek64( m_hFile, 0, SEEK_END );
#endif

#ifdef _WIN32
  //
  // Windows specific part of code
  //
  DWORD Tmp;
  HANDLE hDevice = (HANDLE)::_get_osfhandle(m_hFile);
  int Status = ERR_BADPARAMS;

  DISK_GEOMETRY Geom;
  bool bDisk  = !!::DeviceIoControl( hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &Geom, sizeof(Geom), &Tmp, NULL );
  if ( !bDisk )
  {
    _Trace(( stderr, "Can't get sector size of \"%s\". Use default 512 bytes\n", szDevice ));
    assert( 512 == m_BytesPerSector );
  }
  else
  {
    if ( bVerbose )
      _Trace(( stdout, "DeviceIoControl( \"%s\", IOCTL_DISK_GET_DRIVE_GEOMETRY ) => BytesPerSector = %d\n", szDevice, Geom.BytesPerSector ));

    m_BytesPerSector  = Geom.BytesPerSector;
  }

  //
  // Standard sequence of operation on Raw disks:
  // LOCK + operation + DISMOUNT + UNLOCK
  //
  ::FlushFileBuffers( hDevice );

  if ( !bDisk )
  {
  }
  else if ( ::DeviceIoControl( hDevice, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &Tmp, NULL ) )
  {
    m_bLocked = true;
    if ( bVerbose )
      _Trace(( stdout, "\"%s\" is locked\n", szDevice ));
  }
  else if ( !m_bReadOnly )
  {
    _Trace(( stderr, "Can't lock volume \"%s\"\n", szDevice ));
#ifdef _CONSOLE
    if ( !bForceDismount )
    {
      fprintf( stderr, "Press 'y' to force dismount \"%s\"",szDevice );
      // Get the first char
      int r = fgetc( stdin );
      // Skip the next until the end
      while( !feof( stdin ) && '\n' != fgetc( stdin ) )
        ;

      if ( 'y' != r )
      {
        Status = ERR_CANCELLED;
        goto Error;
      }
    }

    if ( !::DeviceIoControl( hDevice, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &Tmp, NULL ) )
      _Trace(( stderr, "Can't dismount volume \"%s\"\n", szDevice ));
    else if ( ++Attempts < 3 )
    {
      _Trace(( stdout, "Volume \"%s\" dismounted.  All opened handles to this volume are now invalid.\n", szDevice ));
      // Reopen volume cause 'hDevice' becomes invalid
      close( m_hFile );
      Sleep( 2000 );
      goto Again;
    }
#else
    UNREFERENCED_PARAMETER( bForceDismount );
#endif

Error:
    close( m_hFile );
    m_hFile = -1;
    return Status;
  }

  if ( (UINT64)-1 == m_Size )
  {
    union {
      PARTITION_INFORMATION_EX ex;
      PARTITION_INFORMATION mbr;
    } Info;
    const char *hint = NULL;
    // Get the size of volume
    if ( ::DeviceIoControl( hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX,
                            NULL, 0,
                            &Info.ex, sizeof(Info.ex),
                            &Tmp, NULL ) )
    {
      m_Size = Info.ex.PartitionLength.QuadPart;
      hint = "IOCTL_DISK_GET_PARTITION_INFO_EX";
    }
    else if ( ::DeviceIoControl( hDevice, IOCTL_DISK_GET_PARTITION_INFO,
                                 NULL, 0,
                                &Info.mbr, sizeof(Info.mbr),
                                &Tmp, NULL ) )
    {
      m_Size = Info.mbr.PartitionLength.QuadPart;
      hint = "IOCTL_DISK_GET_PARTITION_INFO";
    }
    else
    {
      _Trace(( stderr, "Can't get the size of volume \"%s\"\n", szDevice ));
      goto Error;
    }

    if ( bVerbose )
      _Trace(( stdout, "DeviceIoControl( \"%s\", %s ) => 0x%" PLL "x\n", szDevice, hint, m_Size ));
  }

  // Allow to write to the end of volume
#ifndef FSCTL_ALLOW_EXTENDED_DASD_IO
#define FSCTL_ALLOW_EXTENDED_DASD_IO  0x00090083
#endif
  if ( !bDisk )
  {
  }
  else if ( !::DeviceIoControl( hDevice, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &Tmp, NULL )
          && ERROR_INVALID_PARAMETER != GetLastError() )
  {
    _Trace(( stderr, "FSCTL_ALLOW_EXTENDED_DASD_IO failed \"%s\"\n", szDevice ));
  }
  else
  {
    if ( bVerbose )
      _Trace(( stdout, "\"%s\": allowed EXTENDED_DASD_IO\n", szDevice ));
  }
#else
  //
  // Non-windows specific part of code
  //

#if !defined __QNX__ && !defined __FreeBSD__
  struct stat64 st;
  if ( 0 == fstat64( m_hFile, &st ) )
#else
  struct stat st = {0};
  if ( 0 == fstat( m_hFile, &st ) )
#endif
  {
    if ( bVerbose )
      _Trace(( stdout, "fstat(\"%s\") returns mode %o and size %llx\n", szDevice, (unsigned)st.st_mode, (UINT64)st.st_size ));

    if ( !S_ISBLK( st.st_mode ) )
    {
      m_bBlockDevice  = false;
    }
  }
  else
  {
    _Trace(( stderr, "fstat(\"%s\") failed \"%s\"\n", szDevice, strerror(errno) ));
  }

  if ( m_bBlockDevice )
  {
    // Get the size of sector in bytes
    int BytesPerSector = 0;

#ifdef __QNX__
  // Qnx specific
  #ifdef __QNXNTO__
    cam_devinfo_t dbuf = { 0 };
    int r = devctl( m_hFile, DCMD_CAM_DEVINFO, &dbuf, sizeof( dbuf ), NULL );
    BytesPerSector = dbuf.sctr_size;
    if ( r != EOK )
    {
      _Trace(( stderr, "devctl(DCMD_CAM_DEVINFO) failed (%s). Use default sector size(512)\n", strerror(errno) ));
      BytesPerSector = 0;
    }
  #else
    BytesPerSector = QNX4FS_BLOCK_SIZE;
    if ( 0 )
    {
        ;
    }
  #endif
#elif defined __FreeBSD__
    if ( 0 == ioctl( m_hFile, DIOCGSECTORSIZE, &BytesPerSector ) )
    {
      if ( bVerbose )
        _Trace(( stdout, "ioctl(\"%s\",DIOCGSECTORSIZE) returns %d\n", szDevice, BytesPerSector ));
    }
#else
    // BLKSSZGET: get block device logical block size
    // or
    // BLKPBSZGET: get block device hardware sector size
    if ( 0 == ioctl( m_hFile, BLKSSZGET, &BytesPerSector ) )
    {
      if ( bVerbose )
        _Trace(( stdout, "ioctl(\"%s\",BLKSSZGET) returns %d\n", szDevice, BytesPerSector ));
    }
    else
    {
      int err = errno;
      if ( 0 == ioctl( m_hFile, BLKPBSZGET, &BytesPerSector ) )
      {
        _Trace(( stderr, "ioctl(BLKSSZGET) failed (%s). ioctl(BLKPBSZGET) returns sector size %d\n", strerror(err), BytesPerSector ));
      }
      else
      {
        _Trace(( stderr, "ioctl(BLKPBSZGET/BLKSSZGET) failed (%s). Use default sector size(512)\n", strerror(errno) ));
        BytesPerSector = 512;
      }
    }

    //
    // Check for discard + zeroes (Really we need this code only if Verbose is true.
    //
    int zeroes = 0;
    if ( 0 != ioctl( m_hFile, BLKDISCARDZEROES, &zeroes ) )
    {
      m_bZeroes = false;
      if ( bVerbose )
        _Trace(( stdout, "ioctl(\"%s\", BLKDISCARDZEROES) failed (%s)\n", szDevice, strerror(errno) ));
    }
    else if ( 0 == zeroes )
    {
      m_bZeroes = false;
      if ( bVerbose )
        _Trace(( stdout, "\"%s\": discardzeroes is not supported by this device\n", szDevice ));
    }
    else
    {
      if ( bVerbose )
        _Trace(( stdout, "\"%s\": discardzeroes is supported\n", szDevice ));
      m_bZeroes = true;
    }
#endif
    m_BytesPerSector = BytesPerSector;
  }

#ifdef __APPLE__
  m_Size *= m_BytesPerSector;
#endif
#endif // #ifdef _WIN32

  if ( NULL != fd )
    *fd = m_hFile;

  switch( m_BytesPerSector )
  {
  case 512:   break;
  case 4096:  break;
  default:
    _Trace(( stderr, "%s: non standard sector size 0x%x bytes\n", szDevice,  m_BytesPerSector ));
  }

  if ( bVerbose )
    _Trace(( stdout, "\"%s\": disk size 0x%" PLL "x bytes. ~%" PLL "uGB, sector size 0x%x\n", szDevice, m_Size, (m_Size + 0x20000000) >> 30, m_BytesPerSector ));

  // Make a copy of device name to check for ejected media
  m_szDevice = strdup( szName );

  if ( 0 != NewBytesPerSector && NewBytesPerSector != m_BytesPerSector )
  {
    m_BytesPerSector  = NewBytesPerSector;
    _Trace(( stdout, "%s: force to use sector size 0x%x\n", szDevice, NewBytesPerSector ));
  }

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CUFSD_RWBlock::DiscardRange
//
// Discard range of bytes (known as trim for SSD)
///////////////////////////////////////////////////////////
int
CUFSD_RWBlock::DiscardRange(
    IN  const UINT64& Offset,
    IN  const UINT64& Bytes
    )
{
//    _Trace(( stderr, "DiscardBlocks 0x%" PLL "x : 0x%x\n", BlckNum, BlckCount ));
  if ( m_bReadOnly )
    return ERR_WPROTECT;

  if ( m_bNoDiscard )
    return ERR_NOTIMPLEMENTED;

  if ( 0 == Bytes )
    return ERR_NOERROR;

#ifdef _WIN32

#if 0
  // To debug fstrim
  size_t bytes = Bytes > 0x100000? 0x100000 : (size_t)Bytes;
  void* mem = calloc( 1, bytes );
  if ( NULL == mem )
    return ERR_NOMEMORY;
  int err = 0;
  for ( UINT64 Done = 0, ToWrite = Bytes; 0 != ToWrite; Done += bytes, ToWrite -= bytes )
  {
    if ( bytes > ToWrite )
      bytes = (size_t)ToWrite;
    err = WriteBytes( Offset + Done, mem, bytes, 0 );
    if ( 0 != err )
      break;
  }
  free( mem );
  return err;
#elif defined(__MINGW32__)
  DBG_UNREFERENCED_PARAMETER( Offset );
  DBG_UNREFERENCED_PARAMETER( Bytes );
  return ERR_NOTIMPLEMENTED;
#else
  UNREFERENCED_PARAMETER( Offset );
  UNREFERENCED_PARAMETER( Bytes );
  return ERR_NOTIMPLEMENTED;
#endif

#elif defined __FreeBSD__
  return ERR_NOTIMPLEMENTED;

#else

  UINT64 range[2];
  range[0] = Offset;
  range[1] = Bytes;
  if ( 0 == ioctl( m_hFile, BLKDISCARD, &range ) )
  {
    if ( !m_bZeroes && m_bVerbose && !m_bWarnPrinted )
    {
      _Trace(( stderr, "info: device \"%s\" does not zeroes while discarding\n", m_szDevice ));
      m_bWarnPrinted = true;
    }
    return ERR_NOERROR;
  }

  int ret = errno;
  if ( EOPNOTSUPP != ret && ENOTTY != ret ) {
    _Trace(( stderr, "ioctl( \"%s\", BLKDISCARD ) failed %d: \"%s\"\n", m_szDevice, ret, strerror( ret ) ));
  }
  return ret;
#endif
}


///////////////////////////////////////////////////////////
// CUFSD_RWBlock::ReadBytes
//
//
///////////////////////////////////////////////////////////
int
CUFSD_RWBlock::ReadBytes(
    IN const UINT64&  Offset,
    IN void*          Buffer,
    IN size_t         Bytes,
    IN unsigned int   Flags
    )
{
//    _Trace(( stderr, "ReadBytes 0x%" PLL "x : 0x%" PZZ "x\n", Offset, Bytes ));
  if ( m_bNoMedia )
    return ERR_NOMEDIA;

  if ( 0 == Bytes )
    return ERR_NOERROR;

  // Check boundary
  if ( Offset + Bytes > m_Size )
    return ERR_BADPARAMS;

  if ( FlagOn( Flags, RWB_FLAGS_PREFETCH ) )
  {
    assert( NULL == Buffer );
    // Not implemented yet
    return ERR_NOERROR;
  }

  if ( FlagOn( Flags, RWB_FLAGS_VERIFY ) )
  {
    assert( NULL == Buffer );
    // Verify volume
#ifdef _WIN32
    DWORD Tmp;
    VERIFY_INFORMATION Info;
    Info.StartingOffset.QuadPart = Offset;
    Info.Length         = (DWORD)Bytes;

    if ( !::DeviceIoControl( (HANDLE)::_get_osfhandle(m_hFile), IOCTL_DISK_VERIFY,
                               &Info, sizeof(Info), NULL, 0, &Tmp, NULL ) )
    {
      return ERR_BADPARAMS;
    }
#else
    // I don't know how to verify disks in Linux
    // Just try to read
    // Use 128Kb blocks
    size_t ToProcess = Bytes;
    size_t BlockSize = ToProcess < 128*1024? ToProcess : 128*1024;
    void* Tmp = malloc( BlockSize );
    if ( NULL != Tmp )
    {
      size_t Done = 0;
      while( 0 != ToProcess )
      {
        size_t part = ToProcess < BlockSize? ToProcess : BlockSize;
        int r = pread64( m_hFile, Tmp, part, Offset + Done );
        if ( -1 == r || (size_t)r != part )
        {
          int err = errno;
          free( Tmp );
          return -1 == r? err: ERR_BADPARAMS;
        }

        ToProcess -= part;
        Done      += part;
      }
      free( Tmp );
    }
#endif
    return ERR_NOERROR;
  }

  //
  // Try to read in one request
  //
  int r = pread64( m_hFile, Buffer, Bytes, Offset );
  if ( (size_t)r == Bytes )
    return ERR_NOERROR;

  int err  = errno; // save errno

#if defined _WIN32 || defined __FreeBSD__
  //
  // Problems with alignment?
  // This is not driver so use stack buffer
  //
  char      AlignBuffer[4096];
  unsigned  Head = (unsigned)Offset & (sizeof(AlignBuffer) - 1);
  UINT64    Off  = Offset;

  for( ;; )
  {
    unsigned Tail   = sizeof(AlignBuffer)  - Head;
    size_t  ToRead  = Tail < Bytes? Tail : Bytes;
    r = pread64( m_hFile, AlignBuffer, sizeof(AlignBuffer), Off - Head );
    if ( r < 0 || (size_t)r < (ToRead + Head) )
      goto Error;

    //
    // Copy data (skip 'Head' bytes)
    //
    memcpy( Buffer, Add2Ptr( AlignBuffer, Head ), ToRead );

    Bytes -= ToRead;
    if ( 0 == Bytes )
      return ERR_NOERROR;

    //
    // Move pointer and offset
    //
    Buffer = Add2Ptr( Buffer, ToRead );
    Off   += ToRead;
    Head   = 0;
  }

Error:
#endif

  assert( 0 );

  _Trace(( stderr, "\"%s\": error reading 0x%" PZZ "x bytes at offset 0x%" PLL "x, in buffer %p, pread64=%d, error=%d,%s\n",
           m_szDevice, Bytes, Offset, Buffer, r, err, strerror(err) ));
  if ( NULL != m_szDevice && access( m_szDevice, R_OK ) )
  {
    m_bNoMedia = true;
    return ERR_NOMEDIA;
  }

  return -1 == r? err : ERR_READFILE;
}


///////////////////////////////////////////////////////////
// CUFSD_RWBlock::WriteBytes
//
//
///////////////////////////////////////////////////////////
int
CUFSD_RWBlock::WriteBytes(
    IN const UINT64&  Offset,
    IN const void*    Buffer,
    IN size_t         Bytes,
    IN unsigned int   Flags
    )
{
  size_t  ToWrite;
  int err, r;

  //_Trace(( stderr, "WriteBytes 0x%" PLL "x : 0x%" PZZ "x\n", Offset, Bytes ));
  if ( m_bNoMedia )
    return ERR_NOMEDIA;

  if ( m_bReadOnly )
    return ERR_WPROTECT;

  if ( 0 == Bytes )
    return ERR_NOERROR;

  // Check boundary
  if ( Offset + Bytes > m_Size )
    return ERR_BADPARAMS;

  if ( FlagOn( Flags, RWB_FLAGS_ZERO ) )
  {
    assert( NULL == Buffer );
#ifdef __linux__
    UINT64 range[2];
    range[0] = Offset;
    range[1] = Bytes;
    if ( 0 == ioctl( m_hFile, BLKZEROOUT, &range ) )
      return ERR_NOERROR;
#endif
  }
  else
  {
    assert( NULL != Buffer );
    // Try as is
    r = pwrite64( m_hFile, Buffer, Bytes, Offset );
    if ( (size_t)r == Bytes )
      return ERR_NOERROR;
  }

  //
  // Problems with alignment?
  // This is not driver so use stack buffer
  //
  char    AlignBuffer[4096];
  unsigned  Head  = (unsigned)Offset & (sizeof(AlignBuffer) - 1);
  UINT64 Off      = Offset;

  for ( ;; )
  {
    if ( 0 == Head && Bytes >= sizeof(AlignBuffer) )
    {
      ToWrite = sizeof(AlignBuffer);
      r       = sizeof(AlignBuffer);
    }
    else
    {
      unsigned Tail   = sizeof(AlignBuffer)  - Head;
      ToWrite = Bytes < Tail? Bytes : Tail;
      r = pread64( m_hFile, AlignBuffer, sizeof(AlignBuffer), Off - Head );
      if ( -1 == r )
      {
        err = errno;
        break;
      }

      if ( (size_t)r < Head + ToWrite )
      {
        err   = ERR_READFILE;
        errno = 0;
        break;
      }
    }

    if ( FlagOn( Flags, RWB_FLAGS_ZERO ) )
      memset( AlignBuffer + Head, 0, ToWrite );
    else
    {
      memcpy( AlignBuffer + Head, Buffer, ToWrite );
      Buffer = Add2Ptr( Buffer, ToWrite );
    }

    r = pwrite64( m_hFile, AlignBuffer, r, Off - Head );
    if ( -1 == r )
    {
      err = errno;
      break;
    }

    Bytes   -= ToWrite;
    if ( 0 == Bytes )
      return ERR_NOERROR;

    Off += ToWrite;
    Head = 0;
  }

  assert( 0 );

  _Trace(( stderr, "\"%s\": error writing 0x%" PZZ "x bytes at offset 0x%" PLL "x, in buffer %p, pwrite64=%d, error=%d,%s\n",
          m_szDevice, Bytes, Off, Buffer, err, errno, strerror(err) ));
  if ( NULL != m_szDevice && access( m_szDevice, R_OK ) )
  {
    m_bNoMedia = true;
    return ERR_NOMEDIA;
  }
  return err;
}


#ifndef UFSD_DRIVER_LINUX
///////////////////////////////////////////////////////////
// UFSD_IOHandlerCreate
//
// The API exported from this module.
// It returns the pointer to new CUFSD_RWBlock
// NOTE: the caller should call UFSD_IOHandlerFree
// to free resources
///////////////////////////////////////////////////////////
int
UFSD_IOHandlerCreate(
    IN const char*        szDevice,
    IN bool               bReadOnly,
    IN bool               bNoDiscard,
    OUT api::IDeviceRWBlock**  RwBlock,
    OUT int*              fd,
    IN bool               bForceDismount,
    IN bool               bVerbose,
    IN unsigned int       BytesPerSector
   )
{
  // Set the default return value
  *RwBlock = NULL;

  // Try to allocate
  CUFSD_RWBlock* rw = new CUFSD_RWBlock( bReadOnly, bNoDiscard );
  if ( NULL == rw )
    return ERR_NOMEMORY;

  // Try to init
  int err = rw->Init( szDevice, bForceDismount, bVerbose, fd, BytesPerSector );

  // Check error
  if ( !UFSD_SUCCESS( err ) )
  {
    delete rw;
    return err;
  }

  *RwBlock = rw;
  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// SetPartitionType
//
// Sets partition type
///////////////////////////////////////////////////////////
bool
SetPartitionType(
    IN int            Device,
    IN unsigned char  Type
    )
{
#ifdef _WIN32
  DWORD Tmp;
  HANDLE hDevice = (HANDLE)_get_osfhandle(Device);

  union{
    PARTITION_INFORMATION_EX  InfoEx;
    PARTITION_INFORMATION     Info;
  };

  //
  // Get the partition information
  //
  if ( ::DeviceIoControl( hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &InfoEx, sizeof(InfoEx), &Tmp, NULL ) )
  {
    SET_PARTITION_INFORMATION_EX  SetInfoEx;

    if ( PARTITION_STYLE_GPT == InfoEx.PartitionStyle )
    {
      // Gpt, TODO
//      if ( PARTITION_IFS == Type )
    }
    else if ( PARTITION_STYLE_MBR == InfoEx.PartitionStyle )
    {
      // Mbr
      if ( Type == InfoEx.Mbr.PartitionType )
        return true;
      SetInfoEx.PartitionStyle    = PARTITION_STYLE_MBR;
      SetInfoEx.Mbr.PartitionType = Type;
      if ( ::DeviceIoControl( hDevice, IOCTL_DISK_SET_PARTITION_INFO_EX, &SetInfoEx, sizeof(SetInfoEx), NULL, 0, &Tmp, NULL ) )
        return true;
    }
  }

  if ( ::DeviceIoControl( hDevice, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &Info, sizeof(Info), &Tmp, NULL ) )
  {
    if ( Type == Info.PartitionType )
      return true;

    SET_PARTITION_INFORMATION SetInfo;
    SetInfo.PartitionType = Type;
    if ( ::DeviceIoControl( hDevice, IOCTL_DISK_SET_PARTITION_INFO, &SetInfo, sizeof(SetInfo), NULL, 0, &Tmp, NULL ) )
      return true;
  }
  // TODO: Currently only mkXXX call this function
  fprintf( stdout, "Partition is formatted but its fdisk type is not changed\n" );
#else
  //
  // Unfortunately I don't know how to change partition type in Linux
  //
  UNREFERENCED_PARAMETER( Device );
  UNREFERENCED_PARAMETER( Type );
#endif
  return false;
}
#endif
