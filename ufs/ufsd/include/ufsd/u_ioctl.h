// <copyright file="u_ioctl.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File UFSD/include/u_ioctl.h
 *
 *  The I/O codes for UFSD

 NOTE: According to "Preparing Code for the IA-64 Architecture":
       http://www.intel.com/cd/ids/developer/emea/rus/dc/64bit/17969.htm
 we can be sure that sizeof(unsigned int) == 4 on 32 and 64 bit platforms
 in gcc and in MS/Intel compilers

 ****************************************************/

//
// This file can be included from '.c' files
//
#ifdef __cplusplus
// Forward declaration
//struct api::IMessage;
//struct api::IRestartSaver;

namespace UFSD {
#endif

#ifndef IN
  #define IN
  #define OUT
#endif

#ifndef __PARAGON_UINT64_DEFINED
# define __PARAGON_UINT64_DEFINED
# if defined _MSC_VER
   typedef unsigned __int64    UINT64;
   typedef signed   __int64    INT64;
# elif defined __GNUC__
   typedef unsigned long long  UINT64;
   typedef long long           INT64;
# else
  #error "Does your compiler support build-in 64bit values"
# endif
#endif //__PARAGON_UINT64_DEFINED

#ifndef C_ASSERT2
# if defined(__cplusplus)
#  if defined(_MSC_VER) && (_MSC_VER < 1300)
// __LINE__ macro broken when -ZI is used see Q199057
// fortunately MSVC and GCC up to 4.x ignores duplicate typedef's.
#   define C_ASSERT2(e) typedef char __C_ASSERT__[(e)?1:-1]
#  else
#   define __CA__JOIN(a)  __CA__JOIN1(a)
#   define __CA__JOIN1(a) abc##a
#   define C_ASSERT2(e) typedef char __CA__JOIN(__LINE__)[(e)?1:-1]
#  endif
# else
#  define C_ASSERT2(e)
# endif
#endif

//#include <stddef.h> // offsetof is defined here (very often)
#ifndef FIELD_OFFSET2
# ifdef offsetof
#  define FIELD_OFFSET2 offsetof
# elif defined __WATCOMC__
#  define FIELD_OFFSET2(type, field)    __offsetof(type,field)
# elif defined(_MSC_VER) && (_MSC_VER >= 1300)
   // MSC 13.0 and above has keywords used to allow 64-bit support detection.
#  define FIELD_OFFSET2(type, field)    ((long)(__w64 long)(__w64 long*)&(((type *)0)->field))
# else // MSC 13.0 and above
#  define FIELD_OFFSET2(type, field)    ((long)(long*)&(((type *)0)->field))
# endif // MSC 13.0 and above
#endif


#if !defined(UFSD_CALLBACK)
#if defined (__GNUC__)
#if defined(i386)
#define UFSD_CALLBACK __attribute__((stdcall)) __attribute__((regparm(0)))
#else
#define UFSD_CALLBACK
#endif
#else
#define UFSD_CALLBACK __stdcall
#endif
#endif

#ifndef NO_PRAGMA_PACK
#pragma pack( push, 1 )
#endif

//
// Try to keep the existing I/O code values!
//

enum UFSD_IOCONTROL{
  IOCTL_GET_FSTYPE                = 1,

  //
  // Volume operations
  //
  IOCTL_GET_VOLUME_BITMAP         = 10,
  IOCTL_GET_FSVERSION             = 11,
  IOCTL_GET_VOLUME_BYTES          = 12,
  IOCTL_GET_VOLUME_LCN2LBO        = 13,
  IOCTL_GET_VOLUME_LBO2LCN        = 14,

  IOCTL_GET_DIRTY                 = 16,
  IOCTL_SET_DIRTY                 = 17,
  IOCTL_CLEAR_DIRTY               = 18,

  IOCTL_GET_STATISTICS            = 19,
  IOCTL_GET_MEMORY_USAGE          = 20,
  IOCTL_GET_VOLUME_INFO           = 21,
  IOCTL_GET_8_DOT_3               = 22,
  IOCTL_SET_8_DOT_3               = 23,

  IOCTL_FS_QUERY_EXTEND           = 29,
  IOCTL_FS_EXTEND                 = 30,
  IOCTL_FS_RESET_JOURNAL          = 31,
  IOCTL_FS_WIPE                   = 32,
  IOCTL_FS_QUERY_SHRINK           = 33,
  IOCTL_FS_SHRINK                 = 34,

  IOCTL_CLUSTERS_MARK_USE         = 35,
  IOCTL_CLUSTERS_MARK_FREE        = 36,

  IOCTL_FS_MERGE                  = 37,
  IOCTL_FS_SIEVE                  = 38,

  IOCTL_FS_MOVE_LEFT              = 39,

  IOCTL_FS_GET_BADBLOCKS          = 40,
  IOCTL_FS_SET_BADBLOCKS          = 41,

  IOCTL_FS_TRIM_RANGE             = 44,

  //
  // File operations
  //
  IOCTL_GET_RETRIEVAL_POINTERS2   = 51,
  IOCTL_SET_RETRIEVAL_POINTERS2   = 53,

  IOCTL_GET_COMPRESSION2          = 55,
  IOCTL_SET_COMPRESSION2          = 57,

  IOCTL_GET_SPARSE2               = 59,
  IOCTL_SET_SPARSE2               = 61,

  IOCTL_GET_SECURITY_DESC2        = 63,
  IOCTL_SET_SECURITY_DESC2        = 65,

  IOCTL_MOVE_FILE2                = 67,

  IOCTL_GET_TIMES2                = 73,
  IOCTL_SET_TIMES2                = 75,

  IOCTL_IS_META_FILE2             = 77,

  IOCTL_SET_SHORT_NAME2           = 79,

  IOCTL_GET_OBJECT_ID2            = 83,
  IOCTL_SET_OBJECT_ID2            = 85,
  IOCTL_DELETE_OBJECT_ID2         = 87,

  IOCTL_GET_SIZES2                = 89,
  IOCTL_SET_VALID_DATA2           = 91,
  IOCTL_SET_ZERO_DATA2            = 93,

  IOCTL_GET_ATTRIBUTES2           = 95,
  IOCTL_GET_REPARSE_POINT2        = 97,
  IOCTL_SET_ATTRIBUTES2           = 99,

  //
  // Refs
  //
  IOCTL_DROP_INTERGITY2             = 100,


  //
  // NTFS streams operations
  //
  IOCTL_GET_STREAMS2              = 121,
  IOCTL_SET_STREAMS2              = 123,
  IOCTL_READ_STREAM2              = 125,
  IOCTL_WRITE_STREAM2             = 127,
  IOCTL_CREATE_STREAM2            = 129,
  IOCTL_DELETE_STREAM2            = 131,
  IOCTL_SET_STREAM_SIZE2          = 133,

  //
  // HFS forks
  //
  IOCTL_OPEN_FORK2                = 140,


  //
  // Misc NTFS operations
  //
  IOCTL_GET_RAW_RECORD2           = 151,
  IOCTL_GET_RAW_RECORD_NUM        = 152,
  IOCTL_GET_MFT_ZONE              = 153,
  IOCTL_GET_NTFS_INFO             = 154,
  IOCTL_COMPACT_MFT               = 155,
  IOCTL_COMPACT_DIR               = 156,
  IOCTL_GET_NTFS_ENTRIES_COUNT    = 157,
  IOCTL_DEFRAG_MFT                = 158,
  IOCTL_GET_NTFS_NAMES            = 159,
  IOCTL_CREATE_USN_JOURNAL        = 160,
  IOCTL_DELETE_USN_JOURNAL        = 161,

  // Misc Registry operations
  IOCTL_GET_REGTYPE2              = 251,
  IOCTL_SET_REGTYPE2              = 253,

  //
  // exFAT operations
  //
  IOCTL_ENCODE_FH                 = 255,
  IOCTL_DECODE_FH                 = 256,

  // Misc Zip file system operations
  IOCTL_SET_PASSWORD              = 280,

  //
  // Misc APFS operations
  //
  IOCTL_GET_INODE_NAMES           = 512,
  IOCTL_GET_INODES_COUNT          = 513,
  IOCTL_GET_APFS_INFO             = 514,

  // Some compilers can use BYTE or WORD for enumerators
  // depending on enumerator values
  IOCTL_PLACEHOLDER           = 0xFFFFFFFFu

};


///////////////////////////////////////////////////////////
// Common structure for some iocodes
///////////////////////////////////////////////////////////
typedef struct{

  void*   FsObject;             // Handle of file/directory (CFSObject*)
  void*   UnUsed;               // To be compatible with previous version of UFSD_HANDLE

} UFSD_HANDLE;


//===================================================================
//
// IOCTL_GET_FSTYPE
//
// input  - none
//
// output - size_t a-la FS_XXX, see u_fsbase.h for details


//===================================================================
// IOCTL_GET_VOLUME_BITMAP
//
// input  - STARTING_LCN_INPUT_BUFFER
//
// output - VOLUME_BITMAP_BUFFER
//
// If the output buffer is too small to return any data, then the call fails,
// IoControl returns the error code ERR_INSUFFICIENT_BUFFER,
// and the returned byte count is zero.
// If the output buffer is too small to hold all of the data but
// can hold some entries, then UFSD returns as much as fits.
// IoControl returns the error code ERR_MORE_DATA, and BytesReturned indicates
// the amount of data returned. Your application should call IoControl again
// with the same operation, specifying a new starting point.

struct STARTING_LCN_INPUT_BUFFER{
  unsigned int  StartingLcn;
  // The LCN from which the IOCTL_GET_VOLUME_BITMAP operation should start
  // when describing a bitmap. The StartingLcn member will be rounded down
  // to a file-system-dependent rounding boundary, and that value will be
  // returned. This value should be an integral multiple of eight

};

C_ASSERT2( sizeof(struct STARTING_LCN_INPUT_BUFFER) == 4 );

struct VOLUME_BITMAP_BUFFER{
  UINT64 StartingLcn;   // Starting LCN requested as an input
  UINT64 BitmapSize;    // Number of clusters on the volume, starting from the starting LCN returned in the StartingLcn member of this structure.

  // The BitmapSize member is the number of clusters on the volume starting
  // from the starting LCN returned in the StartingLcn member of this structure.
  // For example, suppose there are 0xD3F7 clusters on the volume.
  // If you start the bitmap query at LCN 0xA007, then both the FAT and NTFS file systems
  // will round down the returned starting LCN to LCN 0xA000.
  // The value returned in the BitmapSize member will be (0xD3F7 - 0xA000), or 0x33F7.

  // Here bitmap is placed
//  unsigned char Bitmap[1];

};

C_ASSERT2( sizeof(struct VOLUME_BITMAP_BUFFER) == 16 );


//===================================================================
//
// IOCTL_GET_FSVERSION
//
// input  - none
//
// output - UFSD_FS_VERSION structure

struct UFSD_FS_VERSION{
  unsigned char   MajorVersion;
  unsigned char   MinorVersion;
  unsigned char   LogMajorVersion;  // 0 if unused
  unsigned char   LogMinorVersion;  // 0 if unused
};

C_ASSERT2( sizeof(struct UFSD_FS_VERSION) == 4 );


//===================================================================
//
// IOCTL_GET_VOLUME_BYTES
//
// input  - none
//
// output - pointer to UINT64 variable that accepts volume size in bytes


//===================================================================
//
// IOCTL_GET_VOLUME_LCN2LBO
//
// Translates volume logical cluster to volume offset in byte
//
// input  - pointer to UINT32 (LCN)
//
// output - pointer to UINT64 variable that volume offset (LBO)

//===================================================================
//
// IOCTL_GET_VOLUME_LBO2LCN
//
// Translates volume offset sector to volume logical cluster
//
// input  - pointer to UINT64 (LBO)
//
// output - pointer to UINT32 (LCN)


//===================================================================
//
// IOCTL_GET_DIRTY
//
// input  - none
//
// output - pointer to int
//
// This function returns the dirty flag

//===================================================================
//
// IOCTL_SET_DIRTY
//
// input  - none
//
// output - none
//
// This function sets dirty flag


//===================================================================
//
// IOCTL_CLEAR_DIRTY
//
// input  - none
//
// output - none
//
// This function clears dirty flag


//===================================================================
//
// IOCTL_GET_STATISTICS
//
// input  - none.
//
// output - structure UFSD_FILESYSTEM_STATISTICS.
//
// This function retrieves the information from various file system performance counters.

//
// Filesystem performance counters
//

struct UFSD_NTFS_STATISTICS{

  // Number of bytes read from the master file table.
  UINT64        MftReadBytes;
  // Number of bytes written to the master file table.
  UINT64        MftWriteBytes;
  // Number of bytes written to the master file table mirror.
  UINT64        Mft2WriteBytes;
  UINT64        RootIndexReadBytes;
  UINT64        RootIndexWriteBytes;
  UINT64        BitmapReadBytes;
  UINT64        BitmapWriteBytes;
  UINT64        MftBitmapReadBytes;
  UINT64        MftBitmapWriteBytes;
  UINT64        UserIndexReadBytes;
  UINT64        UserIndexWriteBytes;

  // Number of read operations on the master file table.
  size_t        MftReads;
  // Number of write operations on the master file table.
  size_t        MftWrites;
  // Number of write operations on the master file table mirror.
  size_t        Mft2Writes;
  size_t        RootIndexReads;
  size_t        RootIndexWrites;
  size_t        BitmapReads;
  size_t        BitmapWrites;
  size_t        MftBitmapReads;
  size_t        MftBitmapWrites;
  size_t        UserIndexReads;
  size_t        UserIndexWrites;
  // Number of comparisons in indexes
  size_t        Comparison;

  struct {
    size_t        Calls;                // number of individual calls to allocate clusters
    size_t        Clusters;             // number of clusters allocated
    size_t        Hints;                // number of times a hint was specified

    size_t        RunsReturned;         // number of runs used to satisfy all the requests

    size_t        HintsHonored;         // number of times the hint was useful
    size_t        HintsClusters;        // number of clusters allocated via the hint

  } Allocate;

};

struct UFSD_FAT_STATISTICS{

  // Number of create operations
  size_t        CreateHits;
  // Number of successful create operations.
  size_t        SuccessfulCreates;
  // Number of failed create operations.
  size_t        FailedCreates;

};

// There are two types of files: user and metadata.
// User files are available for the user.
// Metadata files are system files that contain information,
// which the file system uses for its internal organization.
// The number of read and write operations measured as calls
// See FILESYSTEM_STATISTICS from SDK
struct UFSD_FILESYSTEM_STATISTICS{

  // Number of bytes read from user files.
  UINT64        UserFileReadBytes;
  // Number of bytes written to user files.
  UINT64        UserFileWriteBytes;
  // Number of bytes read from metadata files.
  UINT64        MetaDataReadBytes;
  // Number of bytes written to metadata files.
  UINT64        MetaDataWriteBytes;

  // Number of read operations on user files.
  size_t        UserFileReads;
  // Number of read operations
  size_t        DiskReads;
  // Number of write operations on user files.
  size_t        UserFileWrites;
  // Number of write operations
  size_t        DiskWrites;
  // Number of read operations on metadata files.
  size_t        MetaDataReads;
  // Number of write operations on metadata files.
  size_t        MetaDataWrites;

  union {
    struct UFSD_NTFS_STATISTICS  Ntfs;
    struct UFSD_FAT_STATISTICS   Fat;
  }fs;

  // FS_FAT12/FS_FAT16/FS_FAT32 - use fs.Fat
  // FS_NTFS                    - use fs.Ntfs
  unsigned short FileSystemType;
  // This member is set to 1 (one).
  unsigned short Version;

  unsigned char  Align[4];  // To align struct on 8 byte
};


//===================================================================
//
// IOCTL_GET_MEMORY_USAGE
//
// input  - none
//
// output - structure UFSD_MEMORY_USAGE
//
// This function returns usage memory in bytes

struct UFSD_MEMORY_USAGE{
  size_t TotalBytes;
  size_t BytesPerDirs;
  size_t BytesPerFiles;
  size_t Reserved;
};


//===================================================================
//
// IOCTL_GET_VOLUME_INFO
//
// input  - none
//
// output - structure UFSD_VOLUME_INFO.
//
// This function is used to get volume information
//
// NOTE: Look WIN32 API for GetVolumeInformation() for details
//

struct UFSD_VOLUME_INFO{
  // The name of file system
  char          FileSystemName[16];
  // Zero terminated volume name
  // If native name of volume is UNICODE it will be converted
  // with current string manager
  char          VolumeName[128];
  // Volume serial number
  // Some volumes have 32 bits other 64 bits per serial number
  UINT64        SerialNum;
  // Maximum size of file in bytes
  UINT64        MaxFileSize;
  // Maximum size of sparse file in bytes
  UINT64        MaxSparseFileSize;
  // End of directory (ufsd specific)
  UINT64        Eod;

  // The maximum length, in chars, of a file name component supported by the specified file system.
  // A file name component is that portion of a file name between backslashes.
  unsigned int  MaximumComponentLength; //
  // Flags associated with the specified file system.
  // See VOLUME_FLAGS_XXX for details
  unsigned int  FileSystemFlags;

  // Total number of files and directories (ufsd specific)
  UINT64        TotalFilesAndDirs;    // == kstatfs::f_files - kstatfs::f_ffree
  // Maximum available number of files and directories  (ufsd specific)
  UINT64        MaxTotalFilesAndDirs; // == kstatfs::f_files
};

//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_VOLUME_INFO,SerialNum)%8) );

// The file system supports case-sensitive file names.
#define VOLUME_FLAGS_FS_CASE_SENSITIVE              0x00000001
// The file system preserves the case of file names when it places a name on disk.
#define VOLUME_FLAGS_FS_CASE_IS_PRESERVED           0x00000002
// The file system supports Unicode in file names as they appear on disk.
#define VOLUME_FLAGS_FS_UNICODE_STORED_ON_DISK      0x00000004
// The file system preserves and enforces ACLs. For example, NTFS preserves and enforces ACLs, and FAT does not.
#define VOLUME_FLAGS_FS_PERSISTENT_ACLS             0x00000008
// The file system supports file-based compression.
#define VOLUME_FLAGS_FS_FILE_COMPRESSION            0x00000010
// The file system supports disk quotas.
#define VOLUME_FLAGS_FILE_VOLUME_QUOTAS             0x00000020
// The file system supports sparse files.
#define VOLUME_FLAGS_FILE_SUPPORTS_SPARSE_FILES     0x00000040
// The file system supports reparse points.
#define VOLUME_FLAGS_FILE_SUPPORTS_REPARSE_POINTS   0x00000080
//The file system supports object identifiers.
#define VOLUME_FLAGS_FILE_SUPPORTS_OBJECT_IDS       0x00010000
// The file system supports the Encrypted File System (EFS).
#define VOLUME_FLAGS_FS_FILE_ENCRYPTION             0x00020000
//The file system supports named streams.
#define VOLUME_FLAGS_FILE_NAMED_STREAMS             0x00040000
// The specified volume is read-only.
#define VOLUME_FLAGS_FILE_READ_ONLY_VOLUME          0x00080000
// The specified volume supports hard links.
#define VOLUME_FLAGS_FILE_SUPPORTS_HARD_LINKS       0x00400000
// The specified volume supports extended attributes.
#define VOLUME_FLAGS_FILE_SUPPORTS_XATTR            0x00800000
// The file system supports open by FileID
#define VOLUME_FLAGS_FILE_SUPPORTS_OPEN_BY_FILE_ID  0x01000000
// The specified volume supports update sequence number (USN) journals
#define VOLUME_FLAGS_FILE_SUPPORTS_USN_JOURNAL      0x02000000

// UFSD specific flags:
// Volume is dirty
#define VOLUME_FLAGS_UFSD_DIRTY                     0x80000000
#define VOLUME_FLAGS_UFSD_JOURNAL_NOT_EMPTY         0x40000000


//===================================================================
//
// IOCTL_GET_8_DOT_3
//
// input  - none
//
// output - pointer to int
//
// This function returns the flag indicated that short name creation is enabled


//===================================================================
//
// IOCTL_SET_8_DOT_3
//
// input  - pointer int
//
// output - pointer to int
//
// This function enables/disables short name creation


//===================================================================
//
// IOCTL_FS_QUERY_EXTEND
//
// input  - none
//
// output - struct UFSD_EXTEND_INFO
//

struct UFSD_EXTEND_INFO{
  //
  // Specifies the size in bytes to which the volume can be extended up
  //
  UINT64  BytesPerVolume;
};


//===================================================================
//
// IOCTL_FS_EXTEND
//
// input  - UFSD_FS_EXTEND
//
// output - none
//
// NOTE: Resize up an existing UFSD volume
//       New size of volume should be greater than current size

struct UFSD_FS_EXTEND{

  UINT64                        BytesPerVolume;             // New volume size

  struct api::IMessage*         pMsg;                       // Outlet for messages
  size_t                        bVerify;                    // Should the new added space verified for bad blocks?

};


//===================================================================
//
// IOCTL_FS_RESET_JOURNAL
//
// input  - none
//
// output - none
//
// NOTE: This function resets NTFS's "$LogFile"
//

//===================================================================
//
// IOCTL_FS_WIPE
//
// input  - struct UFSD_FS_WIPE (optional)
//
// output - struct UFSD_FS_WIPE_STAT (optional)
//
// NOTE: This function wipes free space and tails
//

typedef void (UFSD_CALLBACK* WIPE_FUNCTION)(
    IN unsigned char* Buffer,
    IN size_t         BytesPerBuffer
    ); // Callback function for filling buffer


struct UFSD_FS_WIPE{

  struct api::IMessage*  IMsg;             // Outlet for messages
  size_t                 Options;          // See UFSD_FS_WIPE_XX
  WIPE_FUNCTION          WipeFunc;         // Function to fill buffer (if NULL then buffer will be filled by zero)

};

C_ASSERT2( 3*sizeof(size_t) == sizeof(UFSD_FS_WIPE) );

#define UFSD_FS_WIPE_TAIL     0x00000001    // Wipe file (dirs) tail (default true)
#define UFSD_FS_WIPE_CLST     0x00000002    // Wipe free clusters (default true)
#define UFSD_FS_WIPE_SHRT     0x00000004    // Remove short names (default false)

struct UFSD_FS_WIPE_STAT{
  size_t  WipedTailsR;            // Number of records for which tail was wiped
  size_t  WipedTailsA;            // Number of attributes for which tail was wiped
  size_t  WipedTailsI;            // Number of indexes for which tail was wiped
  size_t  WipedClst;              // Number of wiped clusters
  size_t  ScannedShrt;            // Number of records scanned for short name
  size_t  RemovedShrt;            // Number of records for which short name was removed
};



//===================================================================
//
// IOCTL_FS_QUERY_SHRINK
//
// input  - pointer to size_t of UFSD_FS_SHRINK_XXX (optional)
//          If NULL then UFSD_FS_SHRINK_FAST is assumed
//
// output - struct UFSD_SHRINK_INFO
//

struct UFSD_SHRINK_INFO{
  //
  // Specifies the size in bytes to
  // which the volume  can be or is
  // going  to be  reduced  down to
  //
  UINT64  BytesPerVolume;
};


//===================================================================
//
// IOCTL_FS_SHRINK
//
// input  - struct UFSD_SHRINK_INFO (optional)
//
// output - none
//
// NOTE: if input struct is NULL then volume will be shrunk (fast) for maximum
//

struct UFSD_FS_SHRINK{
  struct api::IMessage*         IMsg;             // Outlet for messages
  size_t                        Options;          // See UFSD_FS_SHRINK_XXX
  struct UFSD_SHRINK_INFO       Info;
};

#define UFSD_FS_SHRINK_FAST   0x00000001  // Fast shrink (do not move any file). Default
#define UFSD_FS_SHRINK_META   0x00000002  // Shrink that does not move meta files (as Vista)
                                          // Mutually exclusive with UFSD_FS_SHRINK_FAST
#define UFSD_FS_SHRINK_NAMES  0x00000004  // Show names of reclaimed files
#define UFSD_FS_SHRINK_NONJL  0x00000008  // Turn off journal while shrinking

#define UFSD_FS_SHRINK_SAVEFS 0x00000010  // Save current FS type after shrink (IOCTL_FS_QUERY_SHRINK for FAT32, FAT16)


//===================================================================
//
// IOCTL_CLUSTERS_MARK_USE
//
// input  - struct RETRIEVAL_POINTERS_BUFFER
//
// output - none
//
// NOTE: Mark clusters as used. If any cluster is used then return ERR_CANCELLED.
//


//===================================================================
//
// IOCTL_CLUSTERS_MARK_FREE
//
// input  - struct RETRIEVAL_POINTERS_BUFFER
//
// output - none
//
// NOTE: Mark clusters as free. If any cluster is free then return ERR_CANCELLED.
//


//===================================================================
//
// IOCTL_FS_MERGE
//
// input  - struct UFSD_MERGE
//
// output - none
//
// NOTE: merges this volume and right volume into one volume
//        RwBlock for current 'left' volume should be able to read/write right volume
//

struct UFSD_MERGE {
  UINT64                  RightVolumeOffset;  // Offset of right volume in sectors
  struct api::IMessage*   IMsg;               // Outlet for messages
  size_t                  Options;            // See UFSD_FS_MERGE_XXX
  size_t                  CodeForQuestion;    // Code which will be used in IMsg->SyncRequest(CodeForQuestion, const unsigned short RootDir[256] )
                                              // to ask for new name. If 0 == CodeForQuestion and RootDir is existed name
                                              // then operation will be canceled

  unsigned short    RootDir[257];             // Directory name on parent volume where child volume will be placed
};

#define UFSD_FS_MERGE_PARENT_RIGHT    0x00000001  // if right volume should be parent for merged volume

//===================================================================
//
// IOCTL_FS_SIEVE
//
// input  - pointer to UFSD_SIEVE (OPTIONAL)
//
// output - pointer to size_t (OPTIONAL). Number of set bits
//
// NOTE: makes a volume like a sieve
//
struct UFSD_SIEVE {
  struct api::IMessage*  IMsg;                // Outlet for messages
  unsigned int Step;                          // The maximum length of free block
  unsigned int Flags;
};

// keep volume consistent. If not set then modify bitmap. if set then create file /$Sieve that sieves volume
#define UFSD_FS_SIEVE_KEEP_CONSISTENT   0x00000001


//===================================================================
//
// IOCTL_FS_MOVE_LEFT
//
// input  - UFSD_FS_MOVE_LEFT
//
// output - none
//
// NOTE: Moves left boundary of volume
//       If BytesToMove > 0 - shrink
//       If BytesToMove < 0 - extend CRWBlock must be able to process negative blocks
//                e.g. ReadBlock( 0xffffffffffffffff, ... ) means read block '-1'
//

struct UFSD_FS_MOVE_LEFT{
  struct api::IMessage*  IMsg;         // Outlet for messages
  INT64                  BytesToMove;  // Move left boundary. If BytesToMove > 0 - shrink, If BytesToMove < 0 - extend
};


//===================================================================
//
// IOCTL_FS_GET_BADBLOCKS
//
// Returns bad blocks on volume
//
// input  - NONE
//
// output - BADBLOCKS_BUFFER structure
//
//          If size of buffer not enough for all bad blocks
//          ERR_MORE_DATA error will be returned
//          and field ExtentCount will has real number of bad blocks extents

struct BADBLOCKS_BUFFER {
  size_t ExtentCount;    // Count of Extents structure elements in the present BADBLOCKS_BUFFER structure
  struct {
      UINT64  BadLcn;     // The LCN at which the current bad blocks begins on the volume.
      UINT64  Len;        // The number of continious bad blocks
  } Extents[1];
};


#define BYTES_TO_MAXBADBLOCKS( nBytes ) ( 1 + (nBytes - sizeof( struct UFSD::BADBLOCKS_BUFFER))/sizeof(((UFSD::BADBLOCKS_BUFFER*)0)->Extents[0]) )
#define MAXBADBLOCKS_TO_BYTES( nExt   ) ( sizeof( struct UFSD::BADBLOCKS_BUFFER) + (nExt - 1) * sizeof(((UFSD::BADBLOCKS_BUFFER*)0)->Extents[0]) )

//===================================================================
//
// IOCTL_FS_SET_BADBLOCKS
//
// input  - BADBLOCKS_BUFFER
//
// output - none
//
// NOTE: Updates full bad blocks information.
//  To Reset bad blocks use 'ExtentCount = 0'
//


struct UFSD_TRIM_RANGE {
  UINT64 start;   // in bytes
  UINT64 len;     // in bytes
  UINT64 minlen;  // in bytes
};


struct UFSD_TRIM{
  UFSD_HANDLE       Handle;
  struct UFSD_TRIM_RANGE Range;
};


//===================================================================
//
// IOCTL_FS_TRIM_RANGE
//
// input  - struct UFSD_TRIM
//
// output - struct UFSD_TRIM_RANGE (only len is changed)
//


//===================================================================
//
// IOCTL_GET_RETRIEVAL_POINTERS2
//
// input  - UFSD_GET_RETRIEVAL_POINTERS structure
//
// output - RETRIEVAL_POINTERS_BUFFER structure
//
// NOTE: If returned value is ERR_MORE_DATA then
// the output buffer contains a partial list of VCN-to-LCN mappings for the file.
// More entries exist beyond this list, but the buffer was too small to include them.
// The caller should call again with either a larger buffer, a higher starting VCN, or both.
// The first member of the return structure contains a count of extents actually returned.
// See FSCTL_GET_RETRIEVAL_POINTERS in MSDN
//

struct RETRIEVAL_POINTERS_BUFFER{
  size_t  ExtentCount;          // Count of Extents structure elements in the present RETRIEVAL_POINTERS_BUFFER structure
  UINT64  StartingVcn;          // Starting VCN returned by the function call. This is not necessarily the VCN requested by the function call,
                                // as the file system driver may round down to the first VCN of the extent in which the requested starting VCN is found.
  struct EXTENTS{
      UINT64  NextVcn;          // The VCN at which the next extent begins. This value minus either StartingVcn (for the first Extents array member)
                                // or the NextVcn of the previous member of the array (for all other Extents array members) is the length, in clusters, of the current extent.
      UINT64  Lcn;              // The LCN at which the current extent begins on the volume.
                                // On NTFS, the value 0xFFFFFFFF indicates either a compression unit which is partially allocated, or an unallocated region of a sparse file.
  } Extents[1];
};

//C_ASSERT2( sizeof(struct RETRIEVAL_POINTERS_BUFFER) == 32 );

#define BYTES_TO_MAXEXTENTS( nBytes ) ( 1 + (nBytes - sizeof( struct UFSD::RETRIEVAL_POINTERS_BUFFER))/sizeof(((UFSD::RETRIEVAL_POINTERS_BUFFER*)0)->Extents[0]) )
#define MAXEXTENTS_TO_BYTES( nExt   ) ( sizeof( struct UFSD::RETRIEVAL_POINTERS_BUFFER) + (nExt - 1) * sizeof(((UFSD::RETRIEVAL_POINTERS_BUFFER*)0)->Extents[0]) )


// Each file/directory on NTFS volumes is a set of named attributes
// The "default" attributes for file is the nameless attribute "data" (0x80)
// The "default" attributes for directory is the "$I30" index allocation attribute 0xA0

struct UFSD_GET_RETRIEVAL_POINTERS{
  UFSD_HANDLE       Handle;         //
  UINT64            StartingVcn;
  unsigned short    NameLen;        // Stream name length in bytes
  unsigned short    DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned int      Type;           // Stream type to get extents. 0 means default stream.
  unsigned short    Instance;       // Instance of attribute. Used only when Type is not 0.
  unsigned char     UseInstance;    // if not 0 then use type/name/instance of stream
  unsigned char     Flags;

  //unsigned short    Name[1];    // A UNICODE Name of length NameLen/sizeof(short)
  // Other data
};

//
// Use UFSD_GET_RESIDENT_LVO to get the volume's offset of resident attribute
// If
// - stream is resident
// - 0 ==  StartingVcn
// - 0 != (Flags & UFSD_GET_RESIDENT_LVO)
// Then IOCTL_GET_RETRIEVAL_POINTERS[2] returns 64 bit of logical volume offset (LVO)
//
#define UFSD_GET_RESIDENT_LVO   0x01
#define UFSD_GET_ALLOCATED      0x02

//===================================================================
//
// IOCTL_SET_RETRIEVAL_POINTERS2
//
// input  - UFSD_SET_RETRIEVAL_POINTERS
//
// output - none
//
// Allocates file in specified clusters
// NOTE: the content of new clusters is not specified
//

struct UFSD_SET_RETRIEVAL_POINTERS{
  UFSD_HANDLE       Handle;
  unsigned short    NameLen;        // Stream name length in bytes
  unsigned short    DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned int      Type;           // Stream type to set extents. 0 means default stream.
  unsigned short    Instance;       // Instance of attribute. Used only when Type is not 0.
  unsigned char     UseInstance;    // if not 0 then use type/name/instance of stream
  unsigned char     Res;

  //unsigned short    Name[1];   // A UNICODE Name of length NameLen/sizeof(short)
  // struct RETRIEVAL_POINTERS_BUFFER  rp;
};


//===================================================================
//
// IOCTL_GET_COMPRESSION2
//
// input  - Handle to file or directory to get compression state
//
// output - UFSD_COMPRESSION enum

enum UFSD_COMPRESSION{
  UFSD_COMPRESSION_FORMAT_NONE,    // Uncompress the file or directory
  UFSD_COMPRESSION_FORMAT_LZNT1,   // Compress the file or directory, using the LZNT1 compression format.
  UFSD_COMPRESSION_FORMAT_LZFSE,
  UFSD_COMPRESSION_FORMAT_ZLIB,
  UFSD_COMPRESSION_FORMAT_PLACEHOLDER = ~0U  // to be sure that sizeof(UFSD_COMPRESSION) == sizeof(int)
};


//===================================================================
//
// IOCTL_SET_COMPRESSION2
//
// input  - UFSD_SET_COMPRESSION structure
//
// output - none


struct UFSD_SET_COMPRESSION{

  UFSD_HANDLE     Handle;       // Name of file/directory to set compression
  enum UFSD_COMPRESSION  State; // New compression state

};


//===================================================================
//
// IOCTL_GET_SPARSE2
//
// input  - UFSD_HANDLE
//
// output - pointer to "int" that accepts sparse flag
//
// NOTE: This functions just checks standard file attribute for
// bit STD_FILE_ATTRIBUTE_SPARSE_FILE 0x00000200
//


//===================================================================
//
// IOCTL_SET_SPARSE2
//
// input  - UFSD_SET_SPARSE structure
//
// output - none
//
// See FSCTL_SET_SPARSE (https://msdn.microsoft.com/en-us/aa8f5880-f831-49b6-8359-fe07c78c032f)
//

struct UFSD_SET_SPARSE{

  UFSD_HANDLE   Handle;       // Name of file/directory to set sparse
  int           SetSparse;    // 0 - clear sparse, 1 - set sparse
};


//===================================================================
//
// IOCTL_GET_SECURITY_DESC2
//
// input  - UFSD_HANDLE
//
// output - UFSD_SECURITY_DESCRIPTOR structure
//
// NOTE: If returned value is ERR_MORE_DATA then
// the caller should use BytesPerDescriptor + sizeof(UFSD_SECURITY_DESCRIPTOR) bytes
// to allocate the required buffer and call this function again
//
// if function is OK and BytesPerDescriptor is zero then file/directory has no
// security descriptor


struct UFSD_SECURITY_DESCRIPTOR{

  unsigned int  SecurityId;
  unsigned int  BytesPerDescriptor;
//  SECURITY_DESCRIPTOR_RELATIVE  SecurityDescriptor;
};


//===================================================================
//
// IOCTL_SET_SECURITY_DESC2
//
// input  - UFSD_SET_SECURITY_DESCRIPTOR
//
// output - none
//
// Sets new security descriptor
// if BytesPerDescriptor is zero then any security is removed
// Can't be applied to metafiles

struct UFSD_SET_SECURITY_DESCRIPTOR{
  UFSD_HANDLE       Handle;
  struct UFSD_SECURITY_DESCRIPTOR Security;
};


//===================================================================
//
// IOCTL_MOVE_FILE2
//
// Relocates one or more virtual clusters of a file from one logical cluster
// to another within the same volume.
// This operation is used during defragmentation.
//
// input  - structure MOVE_FILE_DATA
//
// output - NULL


struct MOVE_FILE_DATA{

  UFSD_HANDLE             Handle;         //
  unsigned short          NameLen;        // Stream name length in bytes
  unsigned short          DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned int            Type;           // Stream type to move. 0 means default stream.
  unsigned short          Instance;       // Instance of attribute. Used only when Type is not 0.
  unsigned short          Res;            // Reserved.
  unsigned int            StartingVcn;    // VCN (cluster number relative to the beginning of the file) of the first cluster to be moved.
  unsigned int            StartingLcn;    // LCN (cluster number on the volume) to which the VCN is to be moved.
  unsigned int            ClusterCount;   // Count of clusters to be moved
  struct api::IMessage*   IMsg;           // Outlet for messages, optional

  //unsigned short    Name[1];   // A UNICODE Name of length NameLen/sizeof(short)
  // Other data

};


//===================================================================
//
// IOCTL_GET_TIMES2
//
// input  - UFSD_HANDLE structure
//
// output - UFSD_TIMES structure.
//

//
// On-disk times
// E.g. NTFS: 64 bit number of 100 nanoseconds seconds since 1601 year (GMT)
//      HFS+: seconds since 1904 (GMT)
//      Fat:  seconds since 1980 (local)
//
struct UFSD_TIMES{
  UINT64    CrTime;       // time of creation file
  UINT64    ModiffTime;   // time of last file modification
  UINT64    ChangeTime;   // time of last time any attribute was modified.
  UINT64    ReffTime;     // time of last access to file
};


//===================================================================
//
// IOCTL_SET_TIMES2
//
// input  - UFSD_SET_TIMES structure
//
// output - none.
//

struct UFSD_SET_TIMES{
  UFSD_HANDLE       Handle;
  struct UFSD_TIMES Times;
};


//===================================================================
//
// IOCTL_IS_META_FILE2
//
// input  - UFSD_HANDLE structure. UFSD_HANDLE::Name is used
//
// output - pointer to int.
//
// This function is used to determine meta files
// It returns 1 in output variable for meta files.
// It returns 0 for other files


//===================================================================
//
// IOCTL_SET_SHORT_NAME2
//
// input  - UFSD_SHORT_NAME structure
//
// output - none.
//
// This function is used to set new short name


struct UFSD_SHORT_NAME{
  UFSD_HANDLE     Handle;
  unsigned short  NameLen;      // Name length in bytes
  char            Type;         // Type of Name

  // Here name is placed
};


//===================================================================
//
// IOCTL_GET_OBJECT_ID2
//
// input  - UFSD_HANDLE structure
//
// output - UFSD_OBJECT_ID structure.
//
// This function is used to get file object id

struct UFSD_OBJECT_ID{
  unsigned char ObjectId[16];
  unsigned char BirthVolumeId[16];
  unsigned char BirthObjectId[16];
  unsigned char DomainId[16];
};


//===================================================================
//
// IOCTL_SET_OBJECT_ID2
//
// input  - UFSD_SET_OBJECT_ID structure
//
// output - none.
//
// This function is used to set file object id

struct UFSD_SET_OBJECT_ID{
  UFSD_HANDLE           Handle;
  struct UFSD_OBJECT_ID Id;
};


//===================================================================
//
// IOCTL_DELETE_OBJECTID2
//
// input  - UFSD_HANDLE structure
//
// output - none.
//
// This function is used to delete file object id


//===================================================================
//
// IOCTL_GET_SIZES2
//
// input  - UFSD_HANDLE structure
//
// output - pointer to UFSD_SIZES

struct UFSD_SIZES{
  UINT64        AllocatedSize;  // The allocated size of attribute in bytes (multiple of cluster size)
  UINT64        DataSize;       // The size of attribute  in bytes <= AllocatedSize
  UINT64        ValidSize;      // The size of valid part in bytes <= DataSize
  UINT64        TotalAllocated; // The sum of the allocated clusters for a compressed file
};


//===================================================================
//
// IOCTL_SET_VALID_DATA2
//
// input  - UFSD_SET_VALID_DATA structure
//
// output - none.
//
// This function sets the valid data length of the specified file.
// The file cannot be either compressed or sparse.
//
// NOTE: See comments to function 'SetFileValidData'
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa365544%28v=vs.85%29.aspx


struct UFSD_SET_VALID_DATA{
  UFSD_HANDLE   Handle;
  // New valid data length for the file.
  // This parameter must be a positive value that is greater than
  // the current valid data length, but less than the current file size.
  UINT64        ValidSize;
};

//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_SET_VALID_DATA,ValidSize)%8) );


//===================================================================
//
// IOCTL_SET_ZERO_DATA2
//
// input  - UFSD_SET_ZERO_DATA structure
//
// output - none.
//
// This function fills a specified range of a file with zeros.
// If the file is sparse or compressed, NTFS may deallocate disk space within the file.
// This sets the range of bytes to zeros without extending the file size.
// See FSCTL_SET_ZERO_DATA in MSDN
//

struct UFSD_SET_ZERO_DATA{
  UFSD_HANDLE   Handle;
  // File offset of the start of the range to set to zeros, in bytes
  UINT64        FileOffset;
  // Byte offset of the first byte beyond the last zeroed byte
  UINT64        BeyondFinalZero;
};


//===================================================================
//
// IOCTL_GET_ATTRIBUTES2
//
// input  - UFSD_HANDLE structure
//
// output - array of unsigned ints (32 bit). At least one unsigned int requires
//
// This function returns file attributes
// NOTE: NTFS returns attributes stored in standard_attribute and then in file_names
//       FAT  attributes always fit in 8 bits
// See FILE_ATTRIBUTE_XXX (winnt.h) for possible values


//===================================================================
//
// IOCTL_SET_ATTRIBUTES2
//
// input  - UFSD_SET_ATTRIBUTES structure
//
// output - none
//
// This function sets file attributes
// See FILE_ATTRIBUTE_XXX (winnt.h) for possible values


struct UFSD_SET_ATTRIBUTES{
  UFSD_HANDLE   Handle;
  unsigned int  Attributes;
}__attribute__ ((packed));

C_ASSERT2( 2*sizeof(void*) + sizeof(int) == sizeof( UFSD_SET_ATTRIBUTES ) );


//===================================================================
//
// IOCTL_DROP_INTERGITY2
//
// input  - UFSD_HANDLE structure
//
// output - none
//
// This function drops integrity stream


//===================================================================
//
// IOCTL_SET_CLUMP2
//
// input  - UFSD_SET_CLUMP structure
//
// output - none
//
// This function sets clump size for files

struct UFSD_SET_CLUMP{
  UFSD_HANDLE   Handle;
  unsigned int  BytesPerClump;
} __attribute__ ((packed));

C_ASSERT2( 2*sizeof(void*) + sizeof(int) == sizeof( UFSD_SET_CLUMP ) );


///////////////////////////////////////////////////////////
// Microsoft reparse buffer. (see DDK for details)
//
///////////////////////////////////////////////////////////
struct UFSD_REPARSE_DATA_BUFFER{

  unsigned int    ReparseTag;             // 0x00:
  unsigned short  ReparseDataLength;      // 0x04:
  unsigned short  Reserved;

  union {
      // If ReparseTag == 0
      struct {
          unsigned short  SubstituteNameOffset;   // 0x08
          unsigned short  SubstituteNameLength;   // 0x0A
          unsigned short  PrintNameOffset;        // 0x0C
          unsigned short  PrintNameLength;        // 0x0E
          unsigned short  PathBuffer[1];          // 0x10
      } SymbolicLinkReparseBuffer;

      // If ReparseTag == 0xA0000003U
      struct {
          unsigned short  SubstituteNameOffset;   // 0x08
          unsigned short  SubstituteNameLength;   // 0x0A
          unsigned short  PrintNameOffset;        // 0x0C
          unsigned short  PrintNameLength;        // 0x0E
          unsigned short  PathBuffer[1];          // 0x10
      } MountPointReparseBuffer;

      // If ReparseTag == IO_REPARSE_TAG_SYMLINK2       (0xA000000CU) //https://msdn.microsoft.com/en-us/library/cc232006.aspx
      struct {
          unsigned short  SubstituteNameOffset;   // 0x08
          unsigned short  SubstituteNameLength;   // 0x0A
          unsigned short  PrintNameOffset;        // 0x0C
          unsigned short  PrintNameLength;        // 0x0E
          unsigned int    Flags;                  // 0x10 //0-absolute path 1- relative path
          unsigned short  PathBuffer[1];          // 0x14
      } SymbolicLink2ReparseBuffer;

      // If ReparseTag == 0x80000017U
      struct {
          unsigned int WofVersion;                // 0x08 == 1
          unsigned int WofProvider;               // 0x0C  1, WIM backing provider ("WIMBoot"), 2 - System compressed file provider
          unsigned int ProviderVer;               // 0x10: == 1 WOF_FILE_PROVIDER_CURRENT_VERSION == 1
          unsigned int CompressionFormat;         // 0x14: 0, 1, 2, 3. See WOF_COMPRESSION_XXX
      } CompressReparseBuffer;

      struct {
          unsigned char DataBuffer[1];            // 0x08
      } GenericReparseBuffer;
  };

};


///////////////////////////////////////////////////////////
//
// The reparse GUID structure is used by all 3rd party layered drivers to
// store data in a reparse point. For non-Microsoft tags, The GUID field
// cannot be GUID_NULL.
// The constraints on reparse tags are defined below.
// Microsoft tags can also be used with this format of the reparse point buffer.
//
///////////////////////////////////////////////////////////
struct UFSD_REPARSE_GUID_DATA_BUFFER{

  unsigned int    ReparseTag;             // 0x00:
  unsigned short  ReparseDataLength;      // 0x04:
  unsigned short  Reserved;

  char            Guid[16];               // 0x08:

  //
  // Here GenericReparseBuffer is placed
  //

};


//
// Macro to determine whether a reparse point tag corresponds to a tag
// owned by Microsoft.
//
#ifndef IsReparseTagMicrosoft
#define IsReparseTagMicrosoft(_tag) ( ((_tag) & 0x80000000)  )
#endif

//
// Macro to determine whether a reparse point tag is a name surrogate
//
#ifndef IsReparseTagNameSurrogate
#define IsReparseTagNameSurrogate(_tag) ( ((_tag) & 0x20000000) )
#endif


//===================================================================
//
// IOCTL_GET_REPARSE_POINT2
//
// input  - UFSD_HANDLE structure
//
// output - UFSD_REPARSE_GUID_DATA_BUFFER or UFSD_REPARSE_DATA_BUFFER
//
// This function returns files/directory reparse information
// If the output buffer is smaller than sizeof(unsigned int) then
// ERR_INSUFFICIENT_BUFFER is returned
// otherwise function return ERR_MORE_DATA or ERR_NOERROR
// If the file/directory does not contain reparse information
// the returned length is 0
//


//===================================================================
//
// IOCTL_GET_STREAMS2
//
// input  - UFSD_HANDLE structure
//
// output - buffer contained UFSD_STREAM_INFORMATION structures
//
// NOTE:
// If the output buffer is too small to return any data, then the call fails,
// IoControl returns the error code ERR_INSUFFICIENT_BUFFER,
// and the returned byte count is zero.
// If the output buffer is too small to hold all of the data but
// can hold some entries, then UFSD returns as much as fits.
// IoControl returns the error code ERR_MORE_DATA, and BytesReturned indicates
// the amount of data returned.

struct UFSD_STREAM_INFORMATION{
  // Number of bytes that must be skipped to get to the next entry.
  // A value of zero indicates that this is the last entry
  unsigned int    NextEntryOffset;
  // The size in bytes of the name of the stream
  unsigned int    NameLength; // Can't be greater than 255*sizeof(short)
  // The type of the stream (see ATTRIBUTE_TYPE)
  unsigned int    Type;
  // The flags of attribute
  unsigned short  Flags;
  unsigned short  Info;                 // UFSD_STREAM_INFO_XXX flags
  unsigned short  Instance;             // Used to distinguish the same attributes
  unsigned short  Reserved;
  unsigned int    Res2;
  UINT64          ValidSize;            // Written bytes
  UINT64          DataSize;             // The size of stream in bytes
  UINT64          AllocationSize;       // Number of bytes allocated to the stream

  // The name of stream
  //unsigned short  Name[1];
};

// This stream can not be modified
#define UFSD_STREAM_INFO_META     0x0001
// The valid size of the stream is sector aligned
// (f.e. non resident, encrypted)
#define UFSD_STREAM_INFO_ALIGN    0x0002

//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_STREAM_INFORMATION,StreamSize)%8) );
//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_STREAM_INFORMATION,StreamAllocationSize)%8) );
C_ASSERT2( sizeof(struct UFSD_STREAM_INFORMATION) == 0x30 );


//===================================================================
//
// IOCTL_SET_STREAMS2
//
// input  - UFSD_HANDLE structure
//          Then UFSD_STREAM_INFORMATION is placed
//
// output - none
//
// NOTE:
// This code just updates flags of attributes using StreamFlags field.


//===================================================================
//
// IOCTL_READ_STREAM2
//
// input  - UFSD_STREAM_IO structure. UFSD_HANDLE::Handle is used
//
// output - buffer contained stream data
//
// NOTE: The size of bytes to read is got from nOutBufferSize
//

//===================================================================
//
// IOCTL_WRITE_STREAM2
//
// input  - UFSD_STREAM_IO structure. UFSD_HANDLE::Handle is used
//
// output - pointer to size_t WrittenBytes
//
// NOTE: Data to write is placed after structure UFSD_STREAM_IO
//       Its size if nInBufferSize - DataOffset
//


struct UFSD_STREAM_IO{

  UFSD_HANDLE       Handle;
  UINT64            Offset;
  unsigned int      Type;           // Stream type to process.
  unsigned short    NameLen;        // Stream name length in bytes
  unsigned short    DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned short    Instance;       // Instance of attribute.
  unsigned char     UseInstance;    // if not 0 then read/write stream by type/name/instance
  unsigned char     Res;            // Reserved.

  //unsigned short    Name[1];   // A UNICODE Name of length NameLen/sizeof(short)
  // Other data
};

//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_STREAM_IO,Offset)%8) );


//===================================================================
//
// IOCTL_CREATE_STREAM2
//
// input  - UFSD_CREATE_STREAM structure. UFSD_HANDLE::Handle is used
//
// output - none
//
//

struct UFSD_CREATE_STREAM{

  UFSD_HANDLE       Handle;
  unsigned int      Type;           // Stream type to create.
  unsigned short    NameLen;        // Stream name length in bytes
  unsigned short    DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned short    Flags;
  unsigned short    Res;

  //unsigned short    Name[1];   // A UNICODE Name of length NameLen/sizeof(short)

};


//===================================================================
//
// IOCTL_DELETE_STREAM2
//
// input  - UFSD_DELETE_STREAM structure. UFSD_HANDLE::Handle is used
//
// output - none
//
//


struct UFSD_DELETE_STREAM{

  UFSD_HANDLE       Handle;
  unsigned int      Type;           // Stream type to delete.
  unsigned short    NameLen;        // Stream name length in bytes
  unsigned short    DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned short    Instance;       // Instance of attribute.
  unsigned char     UseInstance;    // if not 0 then delete stream by type/name/instance
  unsigned char     Res;

  //unsigned short    Name[1];   // A UNICODE Name of length NameLen/sizeof(short)

};


//===================================================================
//
// IOCTL_SET_STREAM_SIZE2
//
// input  - UFSD_SET_STREAM_SIZE structure. UFSD_HANDLE::Handle is used
//
// output - none
//
//


struct UFSD_SET_STREAM_SIZE{

  UFSD_HANDLE       Handle;
  UINT64            ValidSize;      // Written bytes
  UINT64            DataSize;       // The size of stream in bytes
  unsigned int      Type;           // Stream type to set new size.
  unsigned short    NameLen;        // Stream name length in bytes
  unsigned short    DataOffset;     // Offset to additional data (from the struct beginning). Should be 8 bytes aligned
  unsigned short    Instance;       // Instance of attribute.
  unsigned char     UseInstance;    // if not 0 then use type + name + instance
  unsigned char     Res;

  //unsigned short    Name[1];   // A UNICODE Name of length NameLen/sizeof(short)

};

//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_SET_STREAM_SIZE,StreamSize)%8) );


//===================================================================
//
// IOCTL_OPEN_FORK2
//
// input  - UFSD_HANDLE
//
// output - CFile* pointer
//


//===================================================================
//
// IOCTL_GET_RAW_RECORD2
//
// input  - UFSD_HANDLE
//
// output - UFSD_RAW_RECORD structure
//
// Returns raw (fixed) records


struct UFSD_RAW_RECORD{
  unsigned int  BaseRecord;
  unsigned int  nRecords;
  unsigned int  BytesPerRecord;
  // Raw data of size nRecords*BytesPerRecord is placed here
};

//===================================================================
//
// IOCTL_GET_RAW_RECORD_NUM
//
// input  - pointer to UINT32 (a record number)
//
// output - UFSD_RAW_RECORD structure
//
// Returns raw (fixed) records


//===================================================================
//
// IOCTL_GET_MFT_ZONE
//
// input  - none
//
// output - structure UFSD_MFT_ZONE
//
// This function returns information about MFT zone

struct UFSD_MFT_ZONE{
  unsigned int   StartLcn;      // First cluster of MFT zone
  unsigned int   Clusters;      // Total clusters of MFT zone
};


//===================================================================
//
// IOCTL_GET_NTFS_INFO
//
// input  - none
//
// output - structure UFSD_VOLUME_NTFS_INFO.
//
// This function retrieves information about the specified NTFS volume
//
// NOTE: Look WIN32 API NTFS_VOLUME_DATA_BUFFER and FSCTL_GET_NTFS_VOLUME_DATA
// for details
//

struct UFSD_VOLUME_NTFS_INFO{

  //Serial number of the volume. This is a unique number assigned to the volume media by the operating system.
  UINT64        VolumeSerialNumber;
  //Number of sectors in the specified volume
  UINT64        NumberSectors;
  // Number of used and free clusters in the specified volume
  UINT64        TotalClusters;
  // Number of free clusters in the specified volume.
  UINT64        FreeClusters;
  // Number of reserved clusters in the specified volume
  UINT64        TotalReserved;    // Always zero
  // Number of bytes in a sector on the specified volume
  unsigned int  BytesPerSector;
  // Number of bytes in a cluster on the specified volume. This value is also known as the cluster factor
  unsigned int  BytesPerCluster;
  // Number of bytes in a file record segment
  unsigned int  BytesPerFileRecordSegment;
  // Number of clusters in a file record segment.
  unsigned int  ClustersPerFileRecordSegment;
  // Length of the master file table, in bytes
  UINT64        MftValidDataLength;
  // Starting logical cluster number of the master file table
  UINT64        MftStartLcn;
  // Starting logical cluster number of the master file table mirror.
  UINT64        Mft2StartLcn;
  // Starting logical cluster number of the master file table zone
  UINT64        MftZoneStart;
  // Ending logical cluster number of the master file table zone
  UINT64        MftZoneEnd;

  struct UFSD_FS_VERSION Version;

};

//C_ASSERT2( 0 == (FIELD_OFFSET2(UFSD_VOLUME_NTFS_INFO,MftValidDataLength)%8) );


//===================================================================
//
// IOCTL_GET_APFS_INFO
//
// input  - none
//
// output - structure UFSD_VOLUME_APFS_INFO.
//
// This function retrieves information about the specified APFS volume(s)
//

struct UFSD_VOLUME_APFS_INFO{
    UINT64        BlocksUsed;
    UINT64        BlocksReserved;
    UINT64        FilesCount;
    UINT64        DirsCount;
    UINT64        SymlinksCount;
    UINT64        SpecFilesCount;
    UINT64        SnapshotsCount;
    unsigned int  Index;
    bool          Encrypted;
    bool          CaseSensitive;

    unsigned char SerialNumber[16];
    char          VolumeName[0x100];
    char          Creator[0x20];
};

//===================================================================
//
// IOCTL_COMPACT_MFT
//
// input  - UFSD_COMPACT_MFT structure (optional)
//
// output - UFSD_COMPACT_MFT_STAT (optional)
// NOTE: this function compacts $MFT records
//

struct UFSD_COMPACT_MFT{
  struct api::IMessage*   IMsg;             // Outlet for messages
  size_t                  Options;          // See UFSD_COMPACT_MFT_XXX
  size_t                  CodeForReport;    // Code which will be used in IMsg->SyncRequest(CodeForReport, const UFSD_COMPACT_MFT_STAT*) to show report
                                            // 0 means ignore request
};

#define UFSD_COMPACT_MFT_MOVE       0x00000001    // Compact records (moving to the head)
#define UFSD_COMPACT_MFT_TRUNCATE   0x00000002    // Truncate MFT after compacting

C_ASSERT2( 3*sizeof(size_t) == sizeof(UFSD_COMPACT_MFT) );

//
// This struct is used to show the state of $MFT before and after operation
//

struct UFSD_COMPACT_MFT_STAT{
  size_t  TotalRecords;               // Total number of records before operation
  size_t  FreeRecords;                // Number of free records before operation
  size_t  MovedRecords;               // Number of moved records
  size_t  TotalRecords2;              // Total number of records after operation
  size_t  FreeRecords2;               // Number of free records after operation
};



//===================================================================
//
// IOCTL_COMPACT_DIR
//
// input  - UFSD_COMPACT_DIR  structure (optional)
//
// output - none
// NOTE: this function compacts all directories
//

struct UFSD_COMPACT_DIR{
  struct api::IMessage*   IMsg;             // Outlet for messages
  size_t                  Options;          // See UFSD_COMPACT_DIR_XXX
};

#define UFSD_COMPACT_DIR_FORCE    0x00000001  // Do not ask before on disk compacting


//===================================================================
//
// IOCTL_GET_NTFS_ENTRIES_COUNT
//
// input  - optional pointer to IMessage
//
// output - structure UFSD_VOLUME_NTFS_ENTRIES_COUNT.
// NOTE: this function calculates total count of files and directories
//

struct UFSD_VOLUME_NTFS_ENTRIES_COUNT{

  unsigned int  MFTRecords;     // MFTRecords on volume
  unsigned int  TotalFiles;     // Total number of files
  unsigned int  UserFiles;      // Number of unique user's file
  unsigned int  UserFileNames;  // Number of user's file names in directories
  unsigned int  TotalDirs;      // Number of directories
  unsigned int  UserDirs;       // Number of directories user can enumerate

};


//===================================================================
//
// IOCTL_DEFRAG_MFT
//
// input  - UFSD_DEFRAG_MFT structure (optional)
//
// output - none
// NOTE: this function defrags $MFT
//

struct UFSD_DEFRAG_MFT{
  struct api::IMessage*   IMsg;             // Outlet for messages
  size_t                  CodeForReport;    // Code which will be used in IMsg->SyncRequest(CodeForReport, const UFSD_DEFRAG_MFT_REPORT*) to show report
                                            // 0 means ignore request
};

//
// This struct is used to show fragmentation of $MFT
// in case when not possible to allocate one continues fragment for new $MFT
//
struct UFSD_DEFRAG_MFT_REPORT{
  size_t OldMftFragments;
  size_t NewMftFragments;
};


//===================================================================
//
// IOCTL_GET_NTFS_NAMES
//
// input  - struct UFSD_NTFS_NAMES_INPUT
//
// output - struct UFSD_NTFS_NAMES
//
// NOTE: this function returns the array of names in ntfs
//

struct UFSD_NTFS_NAMES_INPUT{
  unsigned int    FirstMftRecord;
  unsigned short  FirstName;    // First name to return
  unsigned short  Flags;        // See UFSD_NTFS_NAMES_XXX
};

// Convert names from UTF16 to current format
#define UFSD_NTFS_NAMES_CONVERT   0x00000001

struct UFSD_NTFS_NAMES_ENTRY{
  // Number of bytes that must be skipped to get to the next entry.
  // A value of zero indicates that this is the last entry
  unsigned short  NextEntryOffset;
  unsigned short  BytesPerName; // the length of name in bytes
  unsigned int    MftRecord;
  unsigned int    ParentMftRecord;
  unsigned int    Attributes;   // See UFSD_SUBDIR, UFSD_SYSTEM and so on
  UINT64          BytesPerFile;
  UINT64          CrTime;       // time of creation file
  UINT64          ModiffTime;   // time of last file modification
  UINT64          ChangeTime;   // time of last time any attribute was modified.
  UINT64          ReffTime;     // time of last access to file
  unsigned int    SecurityId;   // Security id in SDS. 0 means local security
  union{
    unsigned short  uName[1];   // 'Raw'/UTF16 name
    unsigned char   aName[1];   // Converted from UTF16 to current format
  };

};

struct UFSD_NTFS_NAMES{

  // Next MftRecord to continue enumeration
  // 0xffffffff indicates that this is the last portion of entries
  //
  unsigned int    NextMftRecord;
  unsigned short  NextName;
  unsigned short  Res;

  // The array of variable length structures
  struct UFSD_NTFS_NAMES_ENTRY  Entry[1];

};

//C_ASSERT2( 8 == FIELD_OFFSET2(UFSD_NTFS_NAMES, Entry) );

//===================================================================
//
// IOCTL_GET_INODE_NAMES
//
// input  - struct UFSD_INODE_NAMES_INPUT
//
// output - struct UFSD_INODE_NAMES
//
// NOTE: this function returns the array of names in unixfs
//

struct UFSD_INODE_NAMES_INPUT
{
  UINT64          FirstObjectId;
  unsigned short  FirstName;    // First name to return
  unsigned short  Flags;
};

// enumerate 'dead' entries from the 'private-dir' folder (APFS)
#define UFSD_INODE_NAMES_READ_DEAD  0x00000001

struct UFSD_INODE_NAMES_ENTRY
{
  // Number of bytes that must be skipped to get to the next entry.
  // A value of zero indicates that this is the last entry
  unsigned short  NextEntryOffset;
  unsigned short  BytesPerName; // the length of name in bytes
  UINT64          ObjectId;
  UINT64          ParentId;
  unsigned int    Attributes;   // See UFSD_SUBDIR, UFSD_SYSTEM and so on
  UINT64          BytesPerFile;
  UINT64          CrTime;       // time of creation file
  UINT64          ModiffTime;   // time of last file modification
  UINT64          ChangeTime;   // time of last time any attribute was modified.
  UINT64          ReffTime;     // time of last access to file
  unsigned char   aName[1];     // FileName
};

struct UFSD_INODE_NAMES
{

  // Next ObjectId to continue enumeration
  // 0xffffffff indicates that this is the last portion of entries
  //
  UINT64          NextObjectId;
  unsigned short  NextName;
  unsigned short  EntriesCount;

  // The array of variable length structures
  struct UFSD_INODE_NAMES_ENTRY  Entry[1];
};


//===================================================================
//
// IOCTL_GET_INODES_COUNT
//
// output - struct UFSD_UNIXFS_INODES_COUNT
//
// NOTE: this function returns the amount of files, folders, symlinks, subvolumes
//

struct UFSD_UNIXFS_INODES_COUNT
{
  UINT64  TotalVolumes;   // Number of subvolumes
  UINT64  TotalFiles;     // Total number of files
  UINT64  TotalDirs;      // Number of directories
  UINT64  TotalSymLinks;  // Number of symlinks
  UINT64  TotalSpecFiles; // Number of fifos, sockets, devices
  UINT64  TotalDeadInodes;// Number of 'dead' inodes from 'private-dir' folder (APFS)
};


//===================================================================
//
// IOCTL_CREATE_USN_JOURNAL
//
// input  - UFSD_CREATE_USN_JOURNAL structure (optional)
//
// output - none
// see notes for FSCTL_CREATE_USN_JOURNAL

struct UFSD_CREATE_USN_JOURNAL{
  UINT64 MaximumSize;
  UINT64 AllocationDelta;
};


//===================================================================
//
// IOCTL_DELETE_USN_JOURNAL
//
// input  - none
//
// output - none


//===================================================================
//
// IOCTL_GET_REGTYPE2
//
// input  - UFSD_HANDLE
//
// output - pointer to unsigned int (RegType)

//===================================================================
//
// IOCTL_SET_REGTYPE2
//
// input  - UFSD_SET_REGTYPE
//
// output - none

struct UFSD_SET_REGTYPE{
  UFSD_HANDLE   Name;
  unsigned int  RegType;
};

//===================================================================
//
// IOCTL_ENCODE_FH
//
// input  - UFSD_HANDLE
//
// output - buffer to be used in IOCTL_DECODE_FH

//===================================================================
//
// IOCTL_DECODE_FH
//
// input  - buffer from IOCTL_ENCODE_FH
//
// output - array of two pointers CFSObject* (Fso and its parent)
//
// NOTE: both objects returned with not incremented references


//===================================================================
//
// IOCTL_SET_PASSWORD
//
// input:
//      pInBuffer - pointer to ASCII password string
//      nInBufferSize  - length of password string without termination character
//
// output - none


#ifdef _IOW
  #define UFSD_IOC_GETCOMPR         _IOR('f', 55, unsigned int)
  #define UFSD_IOC_SETCOMPR         _IOW('f', 57, unsigned int)
  #define UFSD_IOC_GETSPARSE        _IOR('f', 59, unsigned int)
  #define UFSD_IOC_SETSPARSE        _IOW('f', 61, unsigned int)
  #define UFSD_IOC_GETSIZES         _IOR('f', 89, UFSD_SIZES)
  #define UFSD_IOC_SETVALID         _IOW('f', 91, UINT64)
  #define UFSD_IOC_SETTIMES         _IOW('f', 75, UFSD_TIMES)
  #define UFSD_IOC_GETTIMES         _IOR('f', 73, UFSD_TIMES)
  #define UFSD_IOC_SETATTR          _IOW('f', 99, unsigned int)
  #define UFSD_IOC_GETATTR          _IOR('f', 95, unsigned int)
  #define UFSD_IOC_GETMEMUSE        _IOR('v', 20, UFSD_MEMORY_USAGE )
  #define UFSD_IOC_GETVOLINFO       _IOR('v', 21, UFSD_VOLUME_INFO )
  #define UFSD_IOC_GETNTFSSTAT      _IOR('v', 19, UFSD_NTFS_STATISTICS )
  #define UFSD_IOC_CREATE_USN       _IOW('v', 160, UFSD_CREATE_USN_JOURNAL )
  #define UFSD_IOC_DELETE_USN       _IOC(_IOC_WRITE,'v', 161, 0 )
#endif

#ifndef NO_PRAGMA_PACK
#pragma pack( pop )
#endif

#undef C_ASSERT2
#undef FIELD_OFFSET2

#ifdef __cplusplus
} // namespace UFSD {
#endif
