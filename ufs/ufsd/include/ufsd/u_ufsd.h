// <copyright file="u_ufsd.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File u_ufsd.h
 *
 *  UFSD Core is defined here.
 *  Here is CUFSD class definition for UFSD.
 *  Also here is other support stuff for UFSD.
 *
 ****************************************************/

// Forward declaration
namespace api {
  class IInterface;
};

namespace UFSD {

class CDir;
class CFile;
class CFSObject;
class CEntryNumerator;
//struct api::IMessage;

#ifndef SLASH_CHAR
  #define SLASH_CHAR  '/'
  #define SLASH_ASCII "/"
#endif


////////////////////////////////
// File open flags
////////////////////////////////
#define OPEN_CREATE 0x00000001  // Create file if it doesn't exist
#define OPEN_TRUNC  0x00000002  // If file exists, truncate it to zero
#define OPEN_NEW    0x00000004  // File must not exist


//
// Additional options for CUFSD::FormatFs
//
struct OptFormat {
  unsigned char Id[16];             // Identifier of volume (a-ka GUID)
  unsigned char Id_len;             // The length of identifier
  unsigned char Res[3];             // To align next members

  union {
    struct {
      unsigned int    HiddenSectors;      // Number of hidden sectors for bootable volume ( default 0 )
      unsigned short  wSectorsPerTrack;   // Sectors per track ( default 63 )
      unsigned short  wHeadNumber;        // Number of heads ( default 255 )
      unsigned char   MediaByte;          // Default 0xF8 - harddisk
      unsigned char   bEnableCompression; // Mark root directory as compressed ( default false )
      unsigned char   BcFs;               // BCFS signature
      unsigned char   Res;
      int             Version;            // See below
      unsigned int    BytesPerRecord;     // Desired size of MFT record ( 0 means default )
      unsigned int    BytesPerLogFile;    // Desired size of $LogFile( 0 means default )

      //
      // Version
      //  610         - 3.1 Windows 7/8
      //   61         - 3.1 Windows 7/8 without /$Extend/$RmMetadata
      //  600         - 3.1 Vista
      //   60         - 3.1 Vista without /$Extend/$RmMetadata
      //   other      - 3.1 2K/XP
      //
    } Ntfs;

    struct {
      unsigned int    HiddenSectors;      // Number of hidden sectors for bootable volume ( default 0 )
      unsigned short  wSectorsPerTrack;   // Sectors per track ( default 63 )
      unsigned short  wHeadNumber;        // Number of heads ( default 255 )
      unsigned char   MediaByte;          // Default 0xF8 - harddisk
      int             Version;            // See below

      //
      // Version
      //   61         - Windows 7/8
      //   other      - 2K
      //
    } Fat;

    struct {
      UINT64          HiddenSectors;      // Number of hidden sectors for bootable volume ( default 0 )
      unsigned int    AllocationUnit;     // In bytes (default 0)
      unsigned char   TexFat;             // Format TexFAT
      unsigned char   BcFs;               // BCFAT signature
      unsigned char   Oem;                // Format OEM sectors (erase block size == AllocationUnit)
    } exFat;

    struct {
      unsigned char   Integrity;          // Enable(1)/Disable(0xff) integrity on the new volume
      unsigned char   BcFs;               // BCFS signature
      unsigned char   Minor;              // Minor version. 0 means default
    } Refs;

    struct {
      unsigned int    cNodeSize;          // Catalog node size
      unsigned int    eNodeSize;          // Extents node size
      unsigned int    aNodeSize;          // Attributes node size
      unsigned int    JSize;              // Journal size (0 means default)
      //
      // j, c
      // 0, 0 - Mac OS Extended
      // 0, 1 - Mac OS Extended (Case-sensitive)
      // 1, 0 - Mac OS Extended (Journaled)
      // 1, 1 - Mac OS Extended (Journaled,Case-sensitive)
      //
      bool Journal;                 // default false
      bool CaseSensitive;           // default false
    } Hfs;

    struct {
      unsigned int  InodeSize;       //0-def(128), min value 128 bytes, max block size, power of 2
      unsigned int  InodeRatio;      //one Inodes per slice of volume, min 1024(block size) max 65536 * 1024
      unsigned int  ReservedRatio;   //percent of blocks reserved for superuser, def 5%, max 50%
      bool          Journal;         //0-ext2 without journal
      bool          LazyFormat;      //0-full format, 1-not format some structures
      bool          RootWriteAccess; //0-umask 022(default) else umask 0
      //ext4
      bool          Ext4;            //1-with extents and other features
      unsigned int  FlexBGsize;      //0-standart, 2...pow(2)-number of groups that will be packed together, 16-default, 1024-maximum

    } Ext;

    struct {
      unsigned int Version;         //0-new version(default), 0xffffffff-old version
    } LSwap;

    struct {
      bool bTestRegime;
    } Apfs;
  };
};

//C_ASSERT( FIELD_OFFSET(OptFormat,Ntfs.HiddenSectors)    == FIELD_OFFSET(OptFormat,Fat.HiddenSectors) );
//C_ASSERT( FIELD_OFFSET(OptFormat,Ntfs.wSectorsPerTrack) == FIELD_OFFSET(OptFormat,Fat.wSectorsPerTrack) );
//C_ASSERT( FIELD_OFFSET(OptFormat,Ntfs.wHeadNumber)      == FIELD_OFFSET(OptFormat,Fat.wHeadNumber) );
//C_ASSERT( FIELD_OFFSET(OptFormat,Ntfs.MediaByte)        == FIELD_OFFSET(OptFormat,Fat.MediaByte) );


struct OptCheck{
  size_t  Flags;          // See CHK_OPTIONS_XXX
  size_t  MaxMem;         // Amount of memory to use guard. 0 - no limits
  bool    bFix;           // true to fix inconsistencies
  bool    bCheckForBad;   // true to check for bad blocks
};


///////////////////////////////////////////
//Top level structure for file operations
///////////////////////////////////////////
struct UFSD_File
{
  UINT64       Offset;    // Current file position
  CFile*       File;      // Pointer to file system file object
  list_head    Entry;
};


struct UInterface {
  api::IBaseMemoryManager*  m_Mm;
#ifdef UFSD_DRIVER_LINUX
  api::IBaseStringManager*  m_Strings;
  api::IBaseLog*            m_Log;
#else
  api::IStringManager*      m_Strings;
  api::ILog*                m_Log;
#endif
  api::IMessage*            m_Msg;
  api::ITime*               m_Time;

};


#ifndef UFSD_DRIVER_LINUX

////////////////////////////////////////////////////////////////
// CUFSD Class
//
// This is the facade API class for Universal File System Driver SDK
//
// Convention on paths
//
// Member functions of CUFSD class accept path names in the 'absolute'
// form (which is relative to the volume's root directory) as well as
// path names relative to the current directory (that was previously
// set by a call to CUFSD::ChDir). UFSD SDK treats a path as absolute
// if it starts with '/'. Otherwise the path is considered relative.
//
//
// Data safety notes
//
// Note #1
//
// All UFSD classes are not thread safe.
// It is the responsibility of client code to ensure that
// only one thread calls UFSD methods at any given time
// by employing appropriate synchronization method.
//
//
// Note #2
//
// Using several isntances of CUFSD on a single volume at the same time
// will cause data corruption sooner rather than later.
//
// Just don't do that.
//
////////////////////////////////////////////////////////////////
class UFS_UFSD_EXPORT CUFSD : public UMemBased<CUFSD>
{
protected:
    UInterface                m_Interface;
    api::ILog*                m_FsLog;          // Pointer to personal log object for fs
    api::IBaseStringManager*  m_Strings;        // Pointer to Strings Manager
    CFileSystem*              m_FileSystem;     // Pointer to File System
    list_head                 m_OpenFiles;
    list_head                 m_OpenSearches;
    api::IDeviceRWBlock**     m_Rws;
    unsigned int              m_RwBlocksCount;

    PreInitParams            m_Params;

    //Parse Path function checks if the path is valid
    //and opens or closes all subdirectories of the path
    //  type        - path string type UNICODE or ASCII
    //  path        - path
    //  close       - defines if all directories should be closed
    //  tail_skip   - interpret last Item in path
    //  dir         - Pointer to the result directory
    //Returns:
    //  0 if OK
    //  error code otherwise
    int ParsePath(
        IN  api::STRType    type,
        IN  const void*     path,
        OUT CDir *&         dir,
        IN  bool            get_last_item,
        OUT void *          buffer,
        IN  size_t          BytesPerBuffer,
        OUT size_t*         BufferLen
        );

    //Runs to the root directory and increases
    //Reference count on the way
    //  dir  -  pointer to start directory
    //  root - pointer to the root directory
    //Returns: error code
    int OpenDirs(
        IN CDir * dir,
        IN CDir * root
        );

    //Flush certain dir
    //  dir - pointer to dir
    //Returns:
    //  Error
    static int __stdcall FlushDir(
        IN CDir * dir
        );

    // Helper function for GetCwd and GetAltCwd
    // See comments for these functions
    int GetCwd2(
        IN api::STRType     type,
        IN void *           dirname,
        IN size_t           size,
        OUT size_t*         name_len,
        IN bool             bAltName
        );

    // close all searches before remove or rename directory
    int CloseSearches(
        IN CDir * dir
        );

    // Helper function to remove directories resursivle
    int  RmDirHlp(
        IN CDir*            Parent,
        IN api::STRType     Type,
        IN const void*      DirName,
        IN size_t           Name_len
        );

    // Initializes Universal File System Driver and mounts
    // a supported file system found on RWBlock
    //   fs_mask -   expected file system types
    //               (combination of FS_* macros defined in u_fsbase.h)
    //               UFSD SDK will only attempt to recognize and mount
    //               the file system types specified in this parameter
    //   Rw      -   pointer to Read Write Block  object
    //   Ss      -   pointer to Strings management object
    //   Tt      -   pointer to time management object
    //   Log     -   pointer to log service
    //   real_fs -   Contains type of actually initialized file system
    //               (This value is filled only in case of successful
    //               initialization/mount.)
    //   fs_option - parameter passed to filesystem
    //               It is FS dependent (!!)
    //               For internal use only.
    // Returns:
    //   0 - if success
    //   error code otherwise
    //
    // Remarks:
    //   After CUFSD::Init returns error, the instance of CUFSD may not be used.
    //   CUFSD::Init multiple times
    int InitCommon(
        IN  size_t              fs_mask,
        IN  api::IDeviceRWBlock** Rws,
        IN  unsigned int        RwBlocksCount,
        IN  api::ITime*         Tt,
        IN  api::ILog*          Log,
        OUT size_t*             real_fs,
        IN  size_t              fs_option,
        OUT size_t*             fs_flags
        );

public:

    ~CUFSD() { Dtor(); } // non virtual!

    void Dtor(); // Used to cheat gcc

    //Get log object for personal log object depending on filesystem type
    static const char* GetFsLogName(size_t fs);

#if defined UFSD_TRACE && !defined UFSD_USE_STATIC_TRACE
    //Get log object for trace
    api::IBaseLog* GetLog() const
    {
      return m_Interface.m_Log;
    }
#endif

    //Initialize some structures (for example for encryption) before fs init
    int PreInit(const PreInitParams* params);

#ifdef UFSD_USE_SIMPLE_CUFSD

    // =============== Initialization 1 ==================
    CUFSD( api::IBaseMemoryManager* Mm )
      : UMemBased<CUFSD>( Mm )
      , m_FsLog(NULL)
      , m_Strings(NULL)
      , m_FileSystem(NULL)
      , m_Rws(NULL)
    {
      m_Interface.m_Log = NULL;
      m_OpenFiles.init();
      m_OpenSearches.init();
      Memzero2( &m_Params, sizeof(m_Params) );
    }

    //
    // Use this 'Init' if you create CUFSD using the above constructor
    //
    int Init(
        IN  size_t                    fs_mask,
        IN  api::IDeviceRWBlock*      Rw,
        IN  api::IBaseStringManager*  Ss,
        IN  api::ITime*               Tt,
        IN  api::ILog*                Log       = NULL,
        OUT size_t*                   real_fs   = NULL,      // OPTIONAL
        IN  size_t                    fs_option = 0,
        OUT size_t*                   fs_flags  = NULL     // OPTIONAL, See UFSD_FLAGS_XXX
        );

    // =============== End of Initialization 1 ==================

#else

    // =============== Initialization 2 ==================
    CUFSD( api::IBaseMemoryManager* Mm, const UInterface& Interface );
    CUFSD( api::IBaseMemoryManager* Mm, api::IInterface* Interface );

    //
    // Use this 'Init' if you create CUFSD in one of the above constructors
    //
    int Init(
        IN  api::IDeviceRWBlock** Rw,
        IN  unsigned int        RwBlocksCount,
        IN  size_t              fs_mask,
        OUT size_t*             real_fs   = NULL, // OPTIONAL
        IN  size_t              fs_option = 0,
        OUT size_t*             fs_flags  = NULL  // OPTIONAL, See UFSD_FLAGS_XXX
        );

    // Helper method just redirects
    int Init(
        IN  api::IDeviceRWBlock*  Rw,
        IN  size_t              fs_mask,
        OUT size_t*             real_fs   = NULL, // OPTIONAL
        IN  size_t              fs_option = 0,
        OUT size_t*             fs_flags  = NULL  // OPTIONAL, See UFSD_FLAGS_XXX
        )
    {
      return Init( &Rw, 1, fs_mask, real_fs, fs_option, fs_flags );
    }
    // =============== End of Initialization 2 ==================

#endif

    //
    //This routine is used to initialize CUFSD for
    // LiveFS scenarios
    //
    //Initializes Universal File System Driver for specified File System
    //  Ss      - pointer to Strings management object
    //  Fs      - pointer to CFileSystem object
    //Returns:
    //  0 - if success;desc non zero;
    //  error code otherwise.
    int Init(
        IN  CFileSystem*  Fs
        )
    {
      if ( Fs == NULL && m_FileSystem && m_FileSystem->m_RootDir )
        m_FileSystem->m_RootDir->DecReffCount();

      m_FileSystem = Fs;
      if ( m_FileSystem && m_FileSystem->m_RootDir )
        m_FileSystem->m_RootDir->IncReffCount();
      return ERR_NOERROR;
    }

    // Helper function to check file descriptor
    bool DescValid(
        IN void * desc
        );

    // Helper function to check search handle
    bool HandleValid(
        IN void * handle
        );

    // Returns current file system
    CFileSystem* GetFileSystem() const{ return m_FileSystem; }

    // Sets current directory
    bool SetCurrentDir(CDir * dir);

    //Open file with name specified
    //  desc - structure describing current state of opened file;
    //  type  - name type UNICODE/ASCII;
    //  fname - file name;
    //  mode  - File open mode
    //Returns:
    //  0 - if success;desc non zero;
    //  error code otherwise.
    int Open(
        OUT void*&          desc,
        IN api::STRType     type,
        IN const void*      fname,
        IN size_t           flags   // 0,OPEN_CREATE,OPEN_TRUNC,OPEN_NEW
        );

    //Move to specified position
    //desc - structure describing current state of opened file;
    //  offset - move offset
    //  origin - code of start position
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Seek(
        IN void*          desc,
        IN const UINT64&  offset,
        IN SEEKOrig       orig
        );

    //Retriev current file position
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Tell(
        IN void *     desc,
        OUT UINT64&   offset
        );

    //Set end of file into current position
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int SetEOF(
        IN void * desc
        );

    //Tests for end-of-file.
    //Returns:
    //  1             - end of file
    //  0             - if not
    int FEOF(
        IN void * desc
        );

    //Close file
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Close(
        IN void * desc
        );

    //Read file
    //  buffer  - buffer for data read
    //  size    - block size in bytes
    //  bytes_read - number of bytes read
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Read(
        OUT size_t& bytes_read,
        IN void *   buffer,
        IN size_t   size,
        IN void *   desc
        );

    //Write file
    //  buffer  - buffer of data to be written
    //  size    - block size in bytes
    //  bytes_read - number of bytes written
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Write(
        OUT size_t&     written,
        IN const void*  buffer,
        IN size_t       size,
        IN void *       desc
        );

    //Delete file with specified name
    // type - name type - UNICODE or ASCII
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Unlink(
        IN api::STRType     type,
        IN const void*      name
        );

    //Move/rename file/or directory named oldname to newname
    //  type - name type - UNICODE or ASCII
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int Rename(
        IN api::STRType     type,
        IN const void*      oldname,
        IN const void*      newname
        );

    //Create directory dirname
    //  type - name type - UNICODE or ASCII
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int MkDir(
        IN api::STRType     type,
        IN const void*      dirname,
        OUT CDir**          dir = NULL
        );

    //Remove directory dirname
    //(removing all content recursively)
    //  type - name type - UNICODE or ASCII
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int RmDir(
        IN api::STRType     type,
        IN const void*      dirname
        );

    //Change current directory
    //  type - name type - UNICODE or ASCII
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int ChDir(
        IN api::STRType     type,
        IN const void*      dirname
        );

    //Get current working directory
    //  type    - type of directory name (ASCII or UNICODE)
    //  dirname - directory name
    //  size    - maximum number of bytes to be retrieved
    //  name_len- name length in symbols
    //Returns:
    //  0 - success
    //  error code otherwise
    int GetCwd(
        IN  api::STRType    type,
        OUT void *          dirname,
        IN  size_t          size,
        OUT size_t*         name_len = NULL
        );

    //Get current working directory in alternative names
    //  type    - type of directory name (ASCII or UNICODE)
    //  dirname - directory name
    //  size    - maximum number of bytes to be retrieved
    //  name_len- name length in symbols
    //Returns:
    //  0 - success
    //  error code otherwise
    int GetAltCwd(
        IN api::STRType     type,
        OUT void *          dirname,
        IN size_t           size,
        OUT size_t*         name_len = NULL
        );

    //Flush file metadata to the underlying storage
    //  desc - opened file to flush
    //
    //Remarks
    //
    //  1. For journaled file systems (HFS+, Ext3FS, ReFS and so on)
    //     the file system's on-disk structures will be consistent and
    //     up-to-date after the call.
    //
    //  2. For file systems that don't have jornaling features (ExFAT,
    //     FAT32) as well as those that don't have full support for
    //     runtime journaling implemented in UFSD SDK (NTFS), there is no
    //     guarantee of on-disk file system consistency after the call.
    //     Therefore, the effect of the call is file system-specific
    //     and should not be relied upon for general application.
    //
    //Returns:
    //  0 - success
    //  error code otherwise
    int Flush(
        IN void * desc
        );

    // Flushes all metadata for opened files and directories
    //
    // Returns:
    //   0 - success
    //   error code otherwise
    //
    // Remarks:
    //   Essentially, this means that all metadata related to opened files and folders
    //   (directory entries, bitmap buffers (if any), MFT or FAT records,
    //   extents and so on, depending on the underlying file system's type)
    //   are writtent to the underlying storage. This routine may involve flushing
    //   parent directories up to the volume's root. This behavior depends on file
    //   system type.
    //   This ensures that on-disk file system structures are consistent.
    //   Storage media may be safely removed after the call.
    //   The time required to complete this call depends on how many files are opened.
    //
    int FlushAll();

    //Retrieves file attributes for file desc,
    //  attr        - common attributes,
    //                if NULL then it is not retrieved
    //  fs_attrs    - file system specific attributes,
    //                if NULL then it is not retrieved
    //Returns:
    //  0 - success
    //  error code otherwise
    int GetAttr(
        IN api::STRType     type,
        IN const void *     name,
        OUT unsigned int*   attr,
        OUT unsigned int*   fs_attrs = NULL
        );

    //Retrieves file creation, reference and modification times
    //  CrTime,       - utc time of creation file
    //  ReffTime,     - utc time of last access to file
    //  ModiffTime,   - utc time of last file modification
    // if NULL is passed as an argument then this time will not be retrieved.
    //Returns:
    //  0 - success
    //  error code otherwise
    int GetTimes(
        IN api::STRType     type,
        IN const void*      name,
        OUT UINT64*         CrTime,
        OUT UINT64*         ReffTime,
        OUT UINT64*         ModiffTime
        );

    //Set file creation, reference and modification times
    //  CrTime,       - utc time of creation file
    //  ReffTime,     - utc time of last access to file
    //  ModiffTime,   - utc time of last file modification
    // if NULL is passed as an argument then this time will not be retrieved.
    //Returns:
    //  0 - success
    //  error code otherwise
    int SetTimes(
        IN api::STRType     type,
        IN const void*      name,
        IN const UINT64*    CrTime,
        IN const UINT64*    ReffTime,
        IN const UINT64*    ModiffTime
        );

    // Sets file attributes for file desc,
    //  attr    -   attributes, if highest bit is set to one then
    //              file system specific attributes are set
    //Returns:
    //  0 - success
    //  error code otherwise
    int SetAttr(
        IN api::STRType     type,
        IN const void*      name,
        IN unsigned int     attr
        );

    //Find first file matching file mask
    //  type      - name type - UNICODE or ASCII
    //  filemask  - file search mask
    //  fi        - result structure with file information
    //  handle    - pointer to structure describing file
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int FindFirst(
        IN api::STRType     type,
        IN const void*      filemask,
        OUT FileInfo&       fi,
        OUT void*&          handle,
        IN  size_t          options = 0 // See UFSD_ENUM_XXX (u_fsbase.h)
        );

    //Find next file after findfirst
    //  handle  - file found
    //  fi      - result structure with file information
    //Returns:
    //  0             - if found
    //  error code    - otherwise
    int FindNext(
        IN void*      handle,
        OUT FileInfo& fi
        );

    //Close search
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int FindClose(
        IN OUT void*&   handle
        );

    //Opens directory wilt name specified
    //  pointer to the opened dir
    //Returns:
    //  0             - if success
    //  error code    - otherwise
    int OpenDir(
        IN api::STRType     type,
        IN const void*      dir_name,
        OUT void*&          dir
        );

    //Closes directory opened with opendir
    int CloseDir(
        IN void* dir
        );

    // Get volume information.
    int GetVolumeInfo(
        OUT UINT64*       FreeClusters            = NULL,
        OUT UINT64*       TotalClusters           = NULL,
        OUT size_t*       BytesPerCluster         = NULL,
        OUT void*         VolumeSerial            = NULL,
        IN  size_t        BytesPerVolumeSerial    = 0,
        OUT size_t*       RealBytesPerSerial      = NULL,
        IN  api::STRType  VolumeNameType          = api::StrUnknown,
        OUT void*         VolumeNameBuffer        = NULL,
        IN  size_t        VolumeNameBufferLength  = 0,
        OUT VolumeState*  VolState                = NULL,
        OUT size_t*       BytesPerSector          = NULL
        );

    // A short form of 'GetVolumeInfo'
    int  GetVolumeState(
        OUT VolumeState* VolState
        );

    // Set volume information.
    int SetVolumeInfo(
        IN const void*    VolumeSerial,
        IN size_t         BytesPerVolumeSerial,
        IN api::STRType   VolumeNameType,
        IN const void*    VolumeNameBuffer,
        IN VolumeState    NewState = NoChange
        );

    // This function gets file information by its' path
    int GetFSObjInfo(
        IN api::STRType     type,
        IN const void*      Path,
        OUT FileInfo&       fi
        );

    // This function gets FileInfo by handle to CFSObject
    int GetFSObjInfo(
        IN void*      FSObj,
        OUT FileInfo& fi
        );

    // Function a-la DeviceIoControl
    int FsIoControl(
        IN  size_t      FsIoControlCode,
        IN  const void* InBuffer       = NULL,  // OPTIONAL
        IN  size_t      nInBufferSize  = 0,     // OPTIONAL
        OUT void*       OutBuffer      = NULL,  // OPTIONAL
        IN  size_t      nOutBufferSize = 0,     // OPTIONAL
        OUT size_t*     BytesReturned  = NULL   // OPTIONAL
        );

private:
  void FreePasswordsList();
};


///////////////////////////////////////////////////////
//                     GetFsFormatParameters
//
//
// Used to get format parameters
//
// Fs                   - FileSystem, supported by UFSD
// FsSizeInSectors      - Volume size in sectors
// Params               - Format possible parameters
//
// Return value:
// true if parameters can be used to format Fs using UFSD
///////////////////////////////////////////////////////
UFS_UFSD_EXPORT bool __stdcall
GetFsFormatParameters(
    IN size_t           Fs,
    IN const UINT64&    VolumeSizeInSectors,
    IN unsigned short   BytesPerSector,
    IN const OptFormat* Opt,
    IN api::IMessage*   IMsg,
    OUT t_FormatParams* Params
    );

///////////////////////////////////////////////////////
//                           FormatFs
//
// Initialize the speicified FS on the specified block device (partition)
//
// Fs               - FileSystem, supported by UFSD
//                    (one of FS_* macros defined in u_fsbase.h)
// Rw               - interface to the partition that will be formatted
// Interface        - container for interfaces (MemoryManager, StringManager, etc)
// bQuickFormat     - If this parameter is non-zero, the routine will perform
//                    'quick' format, meaning that the storage blocks will not
//                    be tested for error-free read/write access.
//                    If zero, all blocks will be tested for RW access
//                    using the routine implemented in api::IDeviceRWBlock
// BytesPerSector   - Leave 0 to use RWBlock's BytesPerSector
//                    Specify non-zero value to override sector size
//                    in the FS on-disk structures
// BytesPerCluster  - Cluster size in bytes
// VolumeName       - Volume name (label)
// OptFormat        - Additional options
//
// Note: Size of volume is retrieved from api::IDeviceRWBlock
// Return UFSD error code
///////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
FormatFs(
    IN size_t                   Fs,
    IN api::IBaseMemoryManager* Mm,
    IN api::IBaseStringManager* Ss,
    IN api::IDeviceRWBlock*     Rw,
    IN api::ITime *             Tt,
    IN api::IMessage*           Msg,
    IN api::ILog*               Log,
    IN bool                     bQuickFormat,
    IN unsigned short           BytesPerSector,
    IN unsigned int             BytesPerCluster,
    IN api::STRType             VolumeNameType,
    IN const void*              szVolumeName,
    IN size_t                   VolumeNameLen,
    IN const OptFormat*         Opt = NULL,
    IN const char*              Hint = NULL
    );

///////////////////////////////////////////////////////
//            CheckFs  (static function)
//
// Try to detect, check and optionally repair a file system
// among the expected set of file system types on the
// specified block device (partition)
//
// Fs                   - expected file system types
//                        (combination of FS_* macros defined in u_fsbase.h)
// Rw                   - interface to the partition that will be formatted
// Interface            - container for interfaces (MemoryManager, StringManager, etc)
// Opt                  - additional options
// Hint                 - Name of volume. Used to show report
//
// Note: Size of volume is retrieved from api::IDeviceRWBlock
// Return UFSD error code
///////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
CheckFs(
    IN size_t                   Fs,
    IN api::IBaseMemoryManager* Mm,
    IN api::IBaseStringManager* Ss,
    IN api::IDeviceRWBlock*     Rw,
    IN api::ITime *             Tt,
    IN api::IMessage*           Msg,
    IN api::ILog*               Log,
    IN const OptCheck*          Opt,
    IN const char*              Hint,
    OUT CHKDSK_INFO&            ChkInfo
    );


///////////////////////////////////////////////////////
//            DefragFs  (static function)
//
// Used to defragment fs
//
// Try to detect and defragment a file system
// among the expected set of file system types on the
// specified block device (partition)
//
// Fs                   - expected file system types
//                        (combination of FS_* macros defined in u_fsbase.h)
// Rw                   - interface to the partition that will be defragmented
// Interface            - container for interfaces (MemoryManager, StringManager, etc)
// FsDefrag             - Input parameters of defragment task
// Report               - Output struct for report
// BytesPerReport       - The size of report structure
// nBytesReturned       - Filled bytes in report,
// Stat                 - FileSystem statistic during operation
// Return UFSD error code
///////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
DefragFs(
    IN  size_t                    Fs,
    IN  api::IBaseMemoryManager*  Mm,
    IN  api::IDeviceRWBlock*      Rw,
    IN  api::IBaseStringManager*  Ss,
    IN  api::ITime*               Tt,
    IN  api::IMessage*            Msg,
    IN  const UFSD_FS_DEFRAG*     FsDefrag,
    OUT UFSD_DEFRAG_REPORT*       Report,
    IN  size_t                    BytesPerReport,
    OUT size_t*                   nBytesReturned,
    OUT UFSD_FILESYSTEM_STATISTICS* Stat = NULL
    );


#define UFSD_FSDUMPOPT_INCLUDE_DATA     0x0001
#define UFSD_FSDUMPOPT_INCLUDE_FORK     0x0002
#define UFSD_FSDUMPOPT_HEAD             0x0004

///////////////////////////////////////////////////////
//            GetVolumeMetaBitmap (static function)
//
// Used to dump filesystem
//
// Fs                   - mask of possible filesystems, supported by UFSD
// Interface            - container of interfaces
// Rw                   - services used to try volume
// Bitmap               - bitmap of size '(ClustersPerVolume+7)/8' bytes
// ClustersPerVolume    - Total clusters in bitmap
// Log2OfCluster        - Log( Bytes per cluster )
// Errors               - Count of errors while operation
//
// Return UFSD error code
///////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
GetVolumeMetaBitmap(
    IN  size_t                    Fs,
    IN  api::IBaseMemoryManager*  Mm,
    IN  api::IDeviceRWBlock*      Rw,
    IN  api::IMessage*            Msg, // OPTIONAL
    OUT unsigned char**           Bitmap,
    OUT UINT64*                   ClustersPerVolume,
    OUT unsigned int*             Log2OfCluster,
    OUT unsigned int*             Errors,
    OUT size_t*                   real_fs = NULL,
    IN  size_t                    FsDumpOpt = 0   // See UFSD_FSDUMPOPT_XXX
    );


///////////////////////////////////////////////////////////
// DoFsDump
//
// Do fsdump image from rw block
///////////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
DoFsDump(
    IN api::IBaseMemoryManager* Mm,
    IN api::IDeviceRWBlock*     Rw,
    IN api::IDeviceRWBlock*     FileRw,
    IN api::IMessage*           Msg,
    IN const char*              szDevice,
    IN int (__cdecl * Sprintf)(char *, const char *, ...)
    );


///////////////////////////////////////////////////////////
// DoFsRestore
//
// Restores fsdump image into rw block
///////////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
DoFsRestore(
    IN api::IBaseMemoryManager* Mm,
    IN api::IDeviceRWBlock*     Rw,
    IN api::IDeviceRWBlock*     FileRw,
    IN api::IMessage*           Msg,
    IN const char*              szDevice,
    IN int (__cdecl * Sscanf)(const char *, const char *, ...)
    );


///////////////////////////////////////////////////////////
// GetFullName
//
// Returns the length of filled buffer
// If FullName is NULL then the returned value is the length
// (not including last zero!) required for full name
// chSize         - Maximum number of symbols in buffer (including last zero)
///////////////////////////////////////////////////////////
UFS_UFSD_EXPORT size_t __stdcall
GetFullName(
    IN  CFSObject*                Fso,
    IN  api::IBaseStringManager*  Str,
    OUT char*                     FullName,
    IN  size_t                    chSize
    );

///////////////////////////////////////////////////////
//                     GetFsMinSize
//
//
// Estimate minimal volume size
//
// Fs                     - FileSystem, supported by UFSD
// BytesPerSector
// BytesPerCluster
// DataCluster            - Size of used data in clusters on volume
// NumFiles               - Number of files on volume
// NumSubdirs             - Number of subdirs on volume
// MinVolumeSizeInSectors - returns minimal volume size for these params
//
// Return value:
///////////////////////////////////////////////////////
UFS_UFSD_EXPORT int __stdcall
GetFsMinSize(
    IN size_t           Fs,
    IN unsigned short   BytesPerSector,
    IN unsigned int     BytesPerCluster,
    IN unsigned int     DataClusters,
    IN size_t           NumFiles,
    IN size_t           NumSubdirs,
    OUT UINT64*         MinVolumeSizeInSectors
    );

// CFile*
// Handle2File(
//     IN void* h  // handle returned by CUFSD::Open
//     );
#define Handle2File(h) ((UFSD::CFile*)*(void**)(Add2Ptr(h,sizeof(UINT64))))

// CDir*
// Handle2Dir(
//     IN void* h  // handle returned by CUFSD::OpenDir
//     );
#define Handle2Dir(h) ((UFSD::CDir*)(h))


// CEntryNumerator*
// Handle2Enum(
//     IN void* h // handle returned by CUFSD::FindFirst
//     );
#define Handle2Enum(h) ((UFSD::CEntryNumerator*)*(void**)(h))

#endif // #ifndef UFSD_DRIVER_LINUX

} // namespace UFSD {
