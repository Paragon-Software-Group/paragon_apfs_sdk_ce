// <copyright file="u_fsbase.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File UFSD/include/fsbase.h
 *
 *  Here are the definitions for Base classes
 *  of file, directory and file system
 *  in Universal File System Driver (UFSD).
 *  For every new file system supported in
 *  UFSD developer of this file system must derive
 *  his own File, Dir, FileSystem classes from
 *  classes described here

 ****************************************************/

struct super_block;

namespace api
{
  class ICipherFactory;
}

namespace UFSD{

//Known file systems
#define FS_NTFS     0x00000002
#define FS_FAT16    0x00000004
#define FS_FAT32    0x00000008
#define FS_EXFAT    0x00000010
#define FS_HFSPLUS  0x00000020
#define FS_EXT2     0x00000040

#define FS_LSWAP    0x00000080
#define FS_FAT12    0x00000200
#define FS_REFS3    0x00001000
#define FS_XFS      0x00002000
#define FS_REFS     0x00004000
#define FS_BTRFS    0x00008000
#define FS_APFS     0x00010000

//
// File systems mask (exclude reg,iso,zip,fb,...)
//
#define FS_MASK     (FS_NTFS|FS_FAT12|FS_FAT16|FS_FAT32|FS_EXFAT|FS_HFSPLUS|FS_EXT2|FS_LSWAP|FS_REFS|FS_XFS|FS_REFS3|FS_BTRFS|FS_APFS)

//virtual FS
#define FS_REG      0x00100000
#define FS_ISO      0x00200000
#define FS_ZIP      0x00400000
#define FS_FB       0x00800000



///////////////////////////////////////////////////////////
// GetFsName
//
// Translates FS_XXX into string form
///////////////////////////////////////////////////////////
const char*
GetFsName(
    IN size_t Fs
    );


enum VolumeState {
    NoChange, // Used in SetVolumeInfo function to skip dirty flag changing
    Dirty,    // Used in (Get/Set)VolumeInfo
    Clean     // Used in (Get/Set)VolumeInfo
};


//Seek origins
enum SEEKOrig {
    UFSD_SEEKCUR=0,
    UFSD_SEEKEND,
    UFSD_SEEKSET
};


//struct for PreInit function
struct PreInitParams
{
  size_t                  FsType;                //Fs type: FS_APFS, FS_BTRFS, FS_XFS and etc
  api::ICipherFactory*    Cf;                    //Pointer to cipher factory
  char**                  PwdList;               //Pointer to passwords array. All passwords is NULL-terminated
  unsigned int            PwdSize;               //Number of passwords
  UINT64                  CheckpointsAgo;        //We will try init fs from CurrentCheckpoint - CheckpountsAgo
};


//
// File attributes
//
#define UFSD_NORMAL       0x00000000  //  Normal file - read/write permitted
#define UFSD_RDONLY       0x00000001  //  Read-only file
#define UFSD_HIDDEN       0x00000002  //  Hidden file
#define UFSD_SYSTEM       0x00000004  //  System file
#define UFSD_VOLID        0x00000008  //  Volume-ID entry
#define UFSD_SUBDIR       0x00000010  //  Subdirectory
#define UFSD_ARCH         0x00000020  //  Archive file
#define UFSD_SPARSED      0x00000200  //  Sparsed file (NTFS and APFS)
#define UFSD_LINK         0x00000400  //  File is link (Reparse point)
#define UFSD_COMPRESSED   0x00000800  //  NTFS Compressed file
#define UFSD_ENCRYPTED    0x00004000  //  NTFS Encrypted file
#define UFSD_INTEGRITY    0x00008000  //  REFS integrity file
#define UFSD_DEDUPLICATED 0x00010000  //  NTFS/REFS file is deduplicated
#define UFSD_ATTRIB_MASK  0x0001cfff

//
// Virtual attributes
//
#define UFSD_VALENCE      0x00400000  //  Valence field is valid
#define UFSD_TASIZE       0x00800000  //  TotalAllocSize field is valid
#define UFSD_NSUBDIRS     0x01000000  //  Subdirs count is valid
#define UFSD_OBJID        0x02000000  //  File contains ObjId attributes. For NTFS use IOCTL_GET_OBJECT_ID2
#define UFSD_VSIZE        0x04000000  //  FileInfo contains valid field 'ValidSize'
#define UFSD_UGM          0x08000000  //  FileInfo contains valid fields 'Mode','Uid','Gid'
#define UFSD_STREAMS      0x10000000  //  File contains additional data streams. For NTFS use CFSObject::GetObjectInfo
#define UFSD_EA           0x20000000  //  File contains Extended attributes. For NTFS use CFSObject::GetObjectInfo
#define UFSD_RESIDENT     0x40000000  //  NTFS resident default data stream. Use CFSObject::GetObjectInfo
#define UFSD_FSSPEC       0x80000000  //  FileInfo contains valid 'FSAttrib'


//
// FileInfo::Mode
//
#define U_IFMT   0170000
#define U_IFSOCK 0140000
#define U_IFLNK  0120000
#define U_IFREG  0100000
#define U_IFBLK  0060000
#define U_IFDIR  0040000
#define U_IFCHR  0020000
#define U_IFIFO  0010000
#define U_ISUID  0004000
#define U_ISGID  0002000
#define U_ISVTX  0001000

#define U_ISLNK(m)  (((m) & U_IFMT) == U_IFLNK)
#define U_ISREG(m)  (((m) & U_IFMT) == U_IFREG)
#define U_ISDIR(m)  (((m) & U_IFMT) == U_IFDIR)
#define U_ISCHR(m)  (((m) & U_IFMT) == U_IFCHR)
#define U_ISBLK(m)  (((m) & U_IFMT) == U_IFBLK)
#define U_ISFIFO(m) (((m) & U_IFMT) == U_IFIFO)
#define U_ISSOCK(m) (((m) & U_IFMT) == U_IFSOCK)

#define U_IRWXU 00700
#define U_IRUSR 00400
#define U_IWUSR 00200
#define U_IXUSR 00100

#define U_IRWXG 00070
#define U_IRGRP 00040
#define U_IWGRP 00020
#define U_IXGRP 00010

#define U_IRWXO 00007
#define U_IROTH 00004
#define U_IWOTH 00002
#define U_IXOTH 00001

#define U_IRWXUGO (U_IRWXU|U_IRWXG|U_IRWXO)
#define U_IALLUGO (U_ISUID|U_ISGID|U_ISVTX|U_IRWXUGO)
#define U_IRUGO   (U_IRUSR|U_IRGRP|U_IROTH)
#define U_IWUGO   (U_IWUSR|U_IWGRP|U_IWOTH)
#define U_IXUGO   (U_IXUSR|U_IXGRP|U_IXOTH)


//File name length definitions
#define MAX_FILENAME    256           // FAT can store 256 symbols per file name
                                      // NTFS can store 255 symbols per file name
#define MAX_DOSFILENAME 13

#ifndef NO_PRAGMA_PACK
#pragma pack( push, 1 )
#endif

//
// File information structure
//
struct FileInfo{

  UINT64          Id;               // Volume unique file/folder identificator

  // All times are 64 bit number of 100 nanoseconds  seconds since 1601 year
  UINT64          CrTime;           // utc time of file creation
  UINT64          ReffTime;         // utc time of last access to file (atime)
  UINT64          ModiffTime;       // utc time of last file modification (mtime)
  UINT64          ChangeTime;       // utc time of last attribute modification (ctime)
  UINT64          AllocSize;        // Allocated bytes for file
  UINT64          FileSize;         // Length of file in bytes
  UINT64          ValidSize;        // Valid size. Valid if FlagOn(Attrib,UFSD_VSIZE)
  UINT64          TotalAllocSize;   // The sum of the allocated clusters for a file (usefull for sparse/compressed files) Valid if FlagOn(Attrib,UFSD_TASIZE)

#ifndef UFSD_DRIVER_LINUX
  // This field can be obtained only via CFSObject::GetObjectInfo()
  // CDir::FindNext() does not fill it!
  UINT64          Usn;              // NTFS only. Last Update Sequence Number of the file. This is a direct index into the file $UsnJrnl. If zero, the USN Journal is disabled.
#endif

  unsigned int    Attrib;           // File attributes. See UFSD_SUBDIR, UFSD_SYSTEM and so on
  unsigned int    FSAttrib;         // File system specific attributes. Valid if FlagOn(Attrib,UFSD_FSSPEC)

  unsigned int    Valence;          // Valence. valid if FlagOn(Attrib,UFSD_VALENCE)
  unsigned int    nSubDirs;         // Subdirs count. valid if FlagOn(Attrib,UFSD_NSUBDIRS)
  unsigned int    Uid;              // user ID    valid if FlagOn(Attrib,UFSD_UGM)
  unsigned int    Gid;              // group ID   valid if FlagOn(Attrib,UFSD_UGM)
  unsigned int    Dev;              // if U_IFBLK == mode || U_IFCHR == mode  valid if FlagOn(Attrib,UFSD_UGM)
  unsigned short  Mode;             // mode       valid if FlagOn(Attrib,UFSD_UGM)

  unsigned short  HardLinks;        // Count of hardlinks. NOTE: NTFS does not fill this member in FindFirst/FindNext. Use CFSObject::GetObjectInfo instead.
  unsigned short  Gen;              // Generation of file: Full NTFS Id == (Id | (UINT64)Gen << 48)

  unsigned short  NameLen;          // Length of Name in characters

#ifdef UFSD_DRIVER_LINUX
  // Driver does not uses the bullshit below
  unsigned short  Name[MAX_FILENAME+1];     // Zero terminated file name (always unicode)
#else
  unsigned short  AltNameLen;               // Length of AltName (if exists) in characters

  unsigned short  Name[MAX_FILENAME+1];     // Zero terminated long file name
  unsigned short  AltName[MAX_DOSFILENAME]; // Alternative DOS name

  api::STRType    NameType;         // Name type (UNICODE or ASCII)
  api::STRType    AltNameType;      // Alternative name type (UNICODE or ASCII)
  char            Res[2];           // To align struct
#endif

};

typedef FileInfo  t_FileInfo;
//C_ASSERT( 0 == (sizeof(FileInfo)%8) );

#ifndef NO_PRAGMA_PACK
#pragma pack( pop )
#endif


//Flags for file info modification
#define SET_ATTRIBUTES        0x00000001    // Set file attributes
#define SET_FSATTRIBUTES      0x00000002
#define SET_MODTIME           0x00000004    // Set modification time
#define SET_CRTIME            0x00000008    // Set creation time
#define SET_REFFTIME          0x00000010    // Set reference time
#define SET_CHTIME            0x00000020    // Set last time any attribute was modified
#define SET_ADD_ATTRIBUTES    0x00000080    // Add specified attributes
#define SET_REM_ATTRIBUTES    0x00000100    // Remove specified attributes
#define SET_UGM               0x00000200    // Set uid/gid/mode
#define SET_VSIZE_SIZE        0x00000400    // Set sizes
#define SET_ASIZE_KEEP        0x00000800    // Keep allocated if truncate. Used only if SET_VSIZE_SIZE is set

// Possible flags for SetEa
#define EXATTR_CREATE    0x1     // set value, fail if attr already exists
#define EXATTR_REPLACE   0x2     // set value, fail if attr does not exist
#define EXATTR_NEED_EA   0x4     // file to which the EA belongs cannot be interpreted without understanding the associated extended attributes.

// Possible flags for CFile::Allocate
#define UFSD_ALLOCATE_KEEP_SIZE   0x01

// Possible flags for CFile::GetMap
// Allocate clusters if requested range is not allocated yet
#define UFSD_MAP_VBO_CREATE             0x0001
// Allow CFile::GetMap( Vbo, Bytes, ... ) to allocate only 'continues' fragment
// Must be combined with UFSD_MAP_VBO_CREATE
#define UFSD_MAP_VBO_CREATE_CONTINUES   0x0002
// Allow CFile::GetMap( Vbo, Bytes, ... ) to allocate less than 'Bytes'
// Must be combined with UFSD_MAP_VBO_CREATE
#define UFSD_MAP_VBO_CREATE_PARTITIAL   0x0004
// Do not call any lock operations
#define UFSD_MAP_VBO_NOLOCK             0x0008
// Zero new allocated clusters (sparse only)
#define UFSD_MAP_VBO_ZERO               0x0010


// Possible flags for MapInfo::Flags
#define UFSD_MAP_LBO_NEW                0x0001
#define UFSD_MAP_LBO_NOTINITED          0x0002
#define UFSD_MAP_LBO_CLONED             0x0004

// Special values for MapInfo::Lbo
// Fragment of file is not allocated (sparsed)
#define UFSD_VBO_LBO_HOLE         ((UINT64)-1)
// File is resident
#define UFSD_VBO_LBO_RESIDENT     ((UINT64)-2)
// Fragment is really compressed (need decompress)
#define UFSD_VBO_LBO_COMPRESSED   ((UINT64)-3)
// File is encrypted
#define UFSD_VBO_LBO_ENCRYPTED    ((UINT64)-4)


//
// Layout of file fragment
//
struct MapInfo{
  UINT64  Lbo;        // Logical byte offset
  UINT64  Len;        // Length of map in bytes
  UINT64  Alloc;      // The allocated size for file
  UINT64  TotalAlloc; // The sum of the allocated clusters for a file (used for sparsed and compressed files)
  UINT64  Head;       // How many bytes left from Lbo in the same fragment
  size_t  Flags;      // Properties of fragment
};


struct FILE_ID_128{
  UINT64 Lo;
  UINT64 Hi;
};


//////////////////////////////////////////////
//Class enumerator for file search procedures
//////////////////////////////////////////////
class BASE_ABSTRACT_CLASS CEntryNumerator : public UMemBased<CEntryNumerator>
{
protected:
  UINT64  m_Position;
  CEntryNumerator(api::IBaseMemoryManager * mm) : UMemBased<CEntryNumerator>(mm), m_Position(0){}
  ~CEntryNumerator(){}; // non virtual!

public:
  virtual void Destroy() = 0;
#ifdef UFSD_DRIVER_LINUX
  UINT64 GetPosition(){ return m_Position; }
  //Set next entry position
  virtual void SetPosition( const UINT64& ) = 0;
#else
  virtual UINT64 GetPosition(){ return m_Position; }
  //Set next entry position
  virtual void SetPosition( const UINT64& Position){ m_Position = Position; }
  //Reset entry position to Zero
  virtual void Reset(){ m_Position = 0; }
#endif
};


class CDir;
class CFSObject;


//
// Select which trace interface to use: ufsd_vtrace or api::IBaseLog.
// NOTE: Use only external defines to select
//
#if !defined UFSD_NO_PRINTK && !defined NO_LOGGING && !defined UFSD_DRIVER_LINUX && !defined UFSD_USE_STATIC_TRACE
  // Use api::IBaseLog
  #define UFSD_GET_LOG
#else
  // Use ufsd_vtrace/ufsd_error
#endif


//Base class for all file systems
class UFS_UFSD_EXPORT BASE_ABSTRACT_CLASS CFileSystem : public UMemBased<CFileSystem>
{
protected:
    char*                     m_TraceBuffer;    // Buffer for 1024 chars

public:
    CDir*                     m_RootDir;
    CDir*                     m_CurrentDir;
    api::IBaseStringManager*  m_Strings;
    api::ITime*               m_Time;

#ifdef UFSD_GET_LOG
    api::IBaseLog*            m_Log;            //
#endif

#ifdef UFSD_DRIVER_LINUX
    IDriverRWBlock*           m_Rw;             // 1st RwBlock
#else
    api::IDeviceRWBlock*      m_Rw;             // 1st RwBlock
    api::IDeviceRWBlock**     m_Rws;            // All of the RwBlocks
    unsigned int              m_RwBlocksCount;
#endif
    size_t                    m_Options;        // See UFSD_OPTIONS_XXX
#ifdef UFSD_DRIVER_LINUX
    void (UFSDAPI_CALL *      m_OnSetDirty)( super_block* Sb );
    super_block*              m_Sb;
#endif
    unsigned int              m_BytesPerEraseBlock; // erase block size. Must be setup before Init()
    bool                      m_bDirty;         // Simple flag for internal use

protected:
    ////////////////
    // NOTE: Every instance of CFileSystem derived class
    // must be created as new( mm ) CFileSystemXXX( mm, ss, tt );
    // operator UMemBased::operator new() zeroes allocated memory
    // so we can skip zeroing any members
    ////////////////

    CFileSystem( IN api::IBaseMemoryManager* mm, IN api::IBaseStringManager* ss, IN api::ITime* tt, IN api::IBaseLog* log )
      : UMemBased<CFileSystem>( mm )
      , m_Strings( ss )
      , m_Time( tt )
#ifdef UFSD_GET_LOG
      , m_Log( log )
#endif
    {
      (void)(log);  // UNREFERENCED_PARAMETER( log );
    }

    ~CFileSystem();  // non virtual!
public:

    //Closes all opened files and directories
    void CloseAll();

    const char* GetFullNameA( IN const CFSObject* obj );
    const char* U2A( IN const unsigned short* szUnicode, IN size_t Len, OUT char szBuffer[256] = NULL );

#ifdef UFSD_GET_LOG
    //Get log object for trace
    api::IBaseLog* GetLog() const { return m_Log; }
    #define GetVcbLog( Vcb ) (Vcb)->m_Log
#else
    #define GetVcbLog( Vcb ) (api::IBaseLog*)NULL
#endif

    // Helper function for exfat/ntfs
    int CreateIntxLink(
        IN  const char* Link,
        IN  size_t      LinkLen,
        OUT void**      IntxLnk,
        OUT size_t*     IntxLen
        );

    //=================================================================
    //          CFileSystem virtual functions
    //=================================================================

    virtual void Destroy() = 0;

    //Initializes File System
    //  RwBlock - Read/Write object
    //  Option  - FS specific options, see UFSD_OPTIONS_XXX
    //  Flags   - Events while init, see UFSD_FLAGS_XXX
    //Returns:
    //  0 - if success;desc non zero;
    //  error code otherwise.
    virtual int Init(
        IN api::IDeviceRWBlock**  RwBlocks,
        IN unsigned int           RwBlocksCount,
        IN size_t                 Options = 0,
        OUT size_t*               Flags   = NULL,
        IN const PreInitParams*   Params  = NULL
        ) = 0;

    // This function is used to reinit FileSystem by new options
    virtual int ReInit(
        IN size_t               Options = 0,
        OUT size_t*             Flags   = NULL,
        IN const PreInitParams* Params  = NULL
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Flushes all opened files and directories as well as all
    // file system metadata and buffers in CRWBlock to ensure consistency
    // of FS data structures on the underlying storage.
    //
    // Parameters:
    //   bWait   -   false (default) => the implementation may choose
    //               to exit immediately after queueing write requests rather
    //               than waiting for all write requests to complete
    //               true => the call will return after all write request are
    //               completed
    //               The effect of the parameter depends on the implementation
    //               of CRWBlock layer.
    // Remarks:
    //   1. Depending on the implementation (the currently mounted FS type)
    //      and option passed to CFileSystem::Init (e.g.
    //      UFSD_OPTIONS_META_ORDERED or UFSD_OPTIONS_JNL or none) on -disk
    //      data structures may be inconsistent after a write operation until
    //      CFileSystem::Flush(true) operation has completed.
    //
    //   2. For journaling file systems we have the following behavior:
    //          bWait -  false => the implementation will write metadata to journal
    //          and exit immediately after that
    //          bWait -  true => the implementation will write metadata to journal
    //          and wait until journal content is committed to the volume, then return.
    //          In both cases the FS on-disk structures will be consistent upon the following mount
    //          operation in case of power or link failure
    //
    virtual int Flush( bool bWait = false ) UFSD_DRIVER_PURE_VIRTUAL;

    //Gets allocation information
    virtual int GetVolumeInfo(
         OUT UINT64*        FreeClusters,
         OUT UINT64*        TotalClusters,
         OUT size_t*        BytesPerCluster,
         OUT void*          VolumeSerial            = NULL,
         IN  size_t         BytesPerVolumeSerial    = 0,
         OUT size_t*        RealBytesPerSerial      = NULL,
         IN  api::STRType   VolumeNameType          = api::StrUnknown,
         OUT void*          VolumeNameBuffer        = NULL,
         IN  size_t         VolumeNameBufferLength  = 0,
         OUT VolumeState*   VolState                = NULL,
         OUT size_t*        BytesPerSector          = NULL
        ) = 0;

    // Set volume information.
    virtual int SetVolumeInfo(
         IN const void*     VolumeSerial,
         IN size_t          BytesPerVolumeSerial,
         IN api::STRType    VolumeNameType,
         IN const void*     VolumeNameBuffer,
         IN VolumeState     NewState
        ) = 0;

    // Function a-la DeviceIoControl
    virtual int IoControl(
        IN  size_t          FsIoControlCode,
        IN  const void*     InBuffer       = NULL, // OPTIONAL
        IN  size_t          InBufferSize   = 0,    // OPTIONAL
        OUT void*           OutBuffer      = NULL, // OPTIONAL
        IN  size_t          OutBufferSize  = 0,    // OPTIONAL
        OUT size_t*         BytesReturned  = NULL  // OPTIONAL
        ) = 0;

#ifdef UFSD_DRIVER_LINUX
    // Returns 0 if names are not equal
    //        +1 if names are equal
    //        -1 if some error occurs
    virtual int IsNamesEqual(
        IN const unsigned short*  Name1,
        IN size_t                 Len1,
        IN const unsigned short*  Name2,
        IN size_t                 Len2
        ) = 0;

    // Returns hash of name
    virtual unsigned int NameHash(
        IN const unsigned short*  Name,
        IN size_t                 Len
        ) = 0;
#endif

    // Opens file or dir by its id
    virtual int OpenByID(
        IN  UINT64      ID,
        OUT CFSObject** Fso,
        IN  int         Flags = 0 // 1 == follow link, 2 == open existed
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Returns name of file system
    virtual const char* GetFsName() const = 0;

    // Returns any parent for given file/dir
    virtual int GetParent(
        IN  CFSObject*  Fso,
        OUT CFSObject** Parent
        ) UFSD_DRIVER_PURE_VIRTUAL;

#if 0 //def UFSD_DRIVER_LINUX
    // Set callback function which is called for every change of free block count
    virtual void SetFreeSpaceCallBack(
        IN  void (*FreeSpaceCallBack)(
            IN size_t Lcn,
            IN size_t Len,
            IN int    AsUsed,
            IN void*  Arg
            ),
        IN void* Arg
        ) = 0;
#endif

    //=================================================================
    //          END OF CFileSystem virtual functions
    //=================================================================

};


///////////////////////////////////////////
//Base class for File and Directory classes
///////////////////////////////////////////
class UFS_UFSD_EXPORT BASE_ABSTRACT_CLASS CFSObject : public UMemBased<CFSObject>
{
protected:
    CFSObject( api::IBaseMemoryManager* Mm )
      : UMemBased<CFSObject>( Mm )
    {
#ifdef UFSD_DRIVER_LINUX2
      m_DoNotUse.init();
#endif
      m_Entry.init();
    }

    ~CFSObject()
    {
#ifdef UFSD_DRIVER_LINUX2
      assert( m_DoNotUse.is_empty() );
#endif
#ifndef UFSD_DRIVER_LINUX
      Free2( m_aName );
      Free2( m_aAltName );
#endif
      m_Entry.remove();
    }

public:
    CDir*             m_Parent;
#ifdef UFSD_DRIVER_LINUX2
    // DO not move this member! ufsd.ko will not build if moved
    // Driver uses this member in ufsd_evict_inode to avoid additional memory allocation
    list_head         m_DoNotUse;
//    inode*            m_Inode;
#endif
    // this item is used to represent the current FSObject in its parent's list of open items (m_OpenFiles/m_OpenFolders)
    list_head         m_Entry;

    enum FSObjectType{FSObjUnknown = 0, FSObjDir = 1, FSObjFile = 2};

#if !defined UFSD_DRIVER_LINUX || defined APPLE
    size_t            m_Cookie;                   // Extra information to be used by consumer.
#endif

    int               m_Reff_count;               // Reference count

#ifdef UFSD_DRIVER_LINUX
    char              m_tObjectType;              // Object type - directory or file
    unsigned char     LastRef;                    // LastReference, 0, 1, 2
#else
    unsigned short*   m_aName;                    // Name for file system object
    unsigned short*   m_aAltName;                 // Alternative DOS name

    unsigned short    m_NameLen;                  // The length of name in symbols
    unsigned short    m_AltNameLen;               // The length of alternative name in symbols

    char              m_tObjectType;              // Object type - directory or file
    char              m_tNameType;                // Name type - ASCII or UNICODE
    char              m_tAltNameType;             // Alternative name type (UNICODE or ASCII)
    char              Res;                        // Reserved
#endif

    //Creates object of specified type with name and name_type
    //with parent dir parent.
    //Returns:
    //  0 if ok
    //  error code otherwise
    int Init(
        IN CDir*            parent,
        IN api::STRType     name_type,
        IN const void*      name,
        IN FSObjectType     obj_type,
        IN size_t           name_len,
        IN api::STRType     AltName_type  = api::StrUnknown,
        IN const void*      AltName       = NULL,
        IN size_t           AltNameLen    = 0
        );

    // Returns the length of filled buffer
    // If FullName is NULL then the returned value is the length
    // (not including last zero!) required for full name
    // chSize         - Maximum number of symbols in buffer (including last zero)
    size_t GetFullName(
        IN  api::IBaseStringManager*  Str,
        OUT char*                     FullName,
        IN  size_t                    chSize
        ) const;

    //Sets 'Name' and 'NameType'
    //Returns:
    //  0 if ok
    //  error code otherwise
    int SetName(
        IN api::STRType     type,
        IN const void*      name,
        IN size_t           name_len
        );

    //Sets 'AltName' and 'AltNameType'
    //Returns error code
    int SetAltName(
        IN api::STRType     type,
        IN const void*      name,
        IN size_t           name_len
        );

    // Returns pointer to 'name'
    // and sets type value into NameType value
    const void* GetName(
        OUT api::STRType&   type,
        IN  bool            bAltName = false,
        OUT size_t*         NameLen  = NULL
        ) const;

    //Compares name with the name of the FSObject
    //  type -  string type (UNICODE / ASCII)
    //  name -  pointer to the name to compare
    //Returns:
    //  0          - if equal
    //  error code - otherwise
    int CompareName(
        IN api::IBaseStringManager* str,
        IN api::STRType             type,
        IN const void *             name,
        IN size_t                   name_len
        );

#ifndef UFSD_DRIVER_LINUX
    //Returns pointer to reference Count
    int GetReffCount()const{ return m_Reff_count; }

    //Increment reference count
    void IncReffCount( int Refs=1 ) { m_Reff_count += Refs; }

    //Decrement reference count
    void DecReffCount( int Refs=1 ) { m_Reff_count -= Refs; }

    //Returns object type for current object (File or Directory)
    FSObjectType GetObjectType()const{ return static_cast<FSObjectType>(m_tObjectType); }

    // Returns unique USN (ntfs/refs) ID
    virtual void GetId128(
        OUT FILE_ID_128* Id
        ) const;
#endif

    //=================================================================
    //          CFSObject virtual functions
    //=================================================================

    virtual void Destroy() = 0;

    // Gets file/directory information
    // fi - file information to get
    // Returns:
    //   UFSD error code
    virtual int GetObjectInfo(
        OUT FileInfo& fi
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Sets file/directory information
    // fi - file information to set
    // Flags     - bit mask of fields to set. See SET_ATTRIBUTES/SET_FSATTRIBUTES and others
    // Returns:
    //   UFSD error code
    virtual int SetObjectInfo(
        IN const FileInfo&  fi,
        IN size_t           Flags
        ) = 0;

    // Returns unique ID
    virtual UINT64 GetObjectID() const = 0;

    // Returns parent unique ID
    // -1 means error
    virtual UINT64 GetParentID(
        IN unsigned int Pos = 0
        ) const UFSD_DRIVER_PURE_VIRTUAL;

    // Returns name
    // NULL means error
    virtual const void* GetNameByPos(
        IN  unsigned int    Pos,
        OUT api::STRType&   Type,
        OUT size_t&         NameLen
        ) const UFSD_DRIVER_PURE_VIRTUAL;

    // Flush file/directory
    virtual int Flush(
        IN bool bDelete = false
        ) = 0;

    //=================================================================
    // Functions to work with named streams
    //=================================================================

    // Copy a list of available stream names into the buffer
    // provided, or compute the buffer size required
    // 'Buffer' - is a set of zero terminated strings of type 'NameType'
    // If ERR_MORE_DATA - *Bytes will contain the necessary size in bytes
    virtual int ListStreams(
        OUT void*           Buffer,
        IN  size_t          BytesPerBuffer,
        OUT size_t*         Bytes,
        OUT api::STRType*   NameType
        );

    // Reads named stream
    virtual int ReadStream(
        IN  api::STRType    NameType,
        IN  const void*     Name,
        IN  size_t          NameLen,
        IN  const UINT64&   Offset,
        OUT void*           Buffer,
        IN  size_t          BytesPerBuffer,
        OUT size_t*         Read
        );

    // Creates/Writes named stream
    virtual int WriteStream(
        IN  api::STRType    NameType,
        IN  const void*     Name,
        IN  size_t          NameLen,
        IN  const UINT64&   Offset,
        IN  const void*     Buffer,
        IN  size_t          BytesPerBuffer,
        OUT size_t*         Written
        );

    // Delete named stream
    virtual int RemoveStream(
        IN  api::STRType    NameType,
        IN  const void*     Name,
        IN  size_t          NameLen
        );

    //=================================================================
    // Functions to work with extended attributes (looks like named streams)
    //=================================================================

    // Copy a list of extended attribute names into the buffer
    // provided, or compute the buffer size required
    virtual int ListEa(
        OUT void*       Buffer,
        IN  size_t      BytesPerBuffer,
        OUT size_t*     Bytes
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Reads extended attribute
    virtual int GetEa(
        IN  const char* Name,
        IN  size_t      NameLen,
        OUT void*       Buffer,
        IN  size_t      BytesPerBuffer,
        OUT size_t*     Len
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Creates/Writes extended attribute
    virtual int SetEa(
        IN  const char* Name,
        IN  size_t      NameLen,
        IN  const void* Buffer,
        IN  size_t      BytesPerBuffer,
        IN  size_t      Flags // EXATTR_XXX
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Reads symbolic link
    virtual int ReadSymLink(
        OUT char*       Buffer,
        IN  size_t      BufLen,
        OUT size_t*     Read
        ) UFSD_DRIVER_PURE_VIRTUAL;

    //=================================================================
    //          END OF CFSObject virtual functions
    //=================================================================

};


/////////////////////////////////
// Base class for file objects
/////////////////////////////////
class UFS_UFSD_EXPORT BASE_ABSTRACT_CLASS CFile : public CFSObject
{
protected:
    CFile( api::IBaseMemoryManager* Mm ) : CFSObject( Mm ) {}
    ~CFile(){}
public:

    //=================================================================
    //          CFile  virtual functions
    //=================================================================

    //Retrieves file size
    //Returns:
    //  0 - if ok
    //  error code otherwise
    virtual int GetSize(
        OUT UINT64& BytesPerFile,
        OUT UINT64* ValidSize = NULL,
        OUT UINT64* AllocSize = NULL
        ) = 0;

    //Truncates or Expands file
    //Returns:
    //  0 if everything is fine
    //  error code - otherwise
    // if ValidSize is not NULL then update 'valid' size too (if FS supports it)
    virtual int SetSize(
        IN const UINT64&  BytesPerFile,
        IN const UINT64*  ValidSize = NULL,
        OUT UINT64*       AllocSize = NULL
        ) = 0;

    //Read len bytes from file from specified position
    //  buffer  - output buffer
    //  len - number of bytes to read
    //  read - number of bytes read from file
    //Returns:
    // 0 if all ok
    // error code otherwise
    virtual int Read(
        IN const UINT64&  Offset,
        OUT size_t        &read,
        OUT void *        buffer,
        IN size_t         len
        ) = 0;

    //Write len bytes to file from buffer
    //to file from specified position
    //  buffer  - output buffer
    //  len - number of bytes to read
    //  written - number of bytes written
    //Returns:
    // 0 if all ok
    // error code otherwise
    virtual int Write(
        IN  const UINT64&   Offset,
        OUT size_t&         written,
        IN  const void*     buffer,
        IN  size_t          len
        ) = 0;

    //
    // Translates Virtual Byte Offset into Logical Byte Offset
    //
    // if ( FlagOn( Flags, UFSD_MAP_VBO_CREATE ) ) {
    //   host requires [Vbo, Vbo+Bytes) for writing
    // } else {
    //   host requires [Vbo, Vbo+Bytes) for reading
    // }
    //
    virtual int GetMap(
        IN  const UINT64& Vbo,
        IN  const UINT64& Bytes,
        IN  size_t        Flags,
        OUT MapInfo*      Map
        ) UFSD_DRIVER_PURE_VIRTUAL;

#ifndef UFSD_DRIVER_LINUX
    // Allocates the disk space within the range [Offset,Offset+Bytes)
    virtual int fAllocate(
        IN const UINT64&  Offset,
        IN const UINT64&  Bytes,
        IN size_t         Flags = 0   // see UFSD_ALLOCATE_XXX flags
        );
#endif

#if defined UFSD_NTFS_DELETE_SUBFILE | defined UFSD_HFS_DELETE_SUBFILE
    // Deletes the range of file
    virtual int Delete(
        IN const UINT64&  Offset,
        IN const UINT64&  Bytes
        ) UFSD_DRIVER_PURE_VIRTUAL;
#endif

    //=================================================================
    //          END OF CFile  virtual functions
    //=================================================================
};


/////////////////////////////////////////
//Base class for directory objects
/////////////////////////////////////////
class UFS_UFSD_EXPORT BASE_ABSTRACT_CLASS CDir : public CFSObject
{
protected:
    CDir( api::IBaseMemoryManager* Mm )
      : CFSObject( Mm )
    {
      m_OpenFiles.init();
      m_OpenDirs.init();
    }
    ~CDir() { Dtor(); }

    void Dtor(); // gcc bug (two identical dtors)

public:
    list_head   m_OpenFiles;      // List of open files
    list_head   m_OpenDirs;       // List of open dirs


    // Helper function for Rename
    void  Relink(
        IN CFSObject* Fso
        );

    //
    // This function closes file or directory
    //
    int Close(
        IN CFSObject* Fso
        );

    // This function looks opened object by name
    CFSObject* LookupOpenObject(
        IN api::IBaseStringManager*  str,
        IN api::STRType              type,
        IN const void*               name,
        IN size_t                    name_len,
        IN int                       IsDir // < 0 if any (file or dir)
        );

    // This function enumerates opened directories
    size_t EnumOpenedDirs(
        IN bool (*fn)(
            IN CDir*  dir,
            IN void*  Param
            ),
        IN void*  Param,
        OUT bool* bBreak
        ) const __attribute__ ((noinline));

    // This function enumerates opened files
    size_t  EnumOpenedFiles(
        IN bool (*fn)(
            IN CFile* file,
            IN void*  Param
            ),
        IN void*  Param,
        OUT bool* bBreak
        ) const __attribute__ ((noinline));

    //=================================================================
    //          CDir  virtual functions
    //=================================================================

    //
    // This function opens file or directory
    //
    virtual int Open(
        IN  api::STRType    type,
        IN  const void*     name,
        IN  size_t          name_len,
        OUT CFSObject**     Fso,
        OUT FileInfo*       fi
        ) UFSD_DRIVER_PURE_VIRTUAL;

    //
    // Create file/dir/node
    //
    virtual int Create(
        IN api::STRType     type,
        IN const void*      name,
        IN size_t           name_len,
        IN unsigned short   mode,
        IN unsigned int     uid,
        IN unsigned int     gid,
        IN const void*      data,
        IN size_t           data_len,
        OUT CFSObject**     fso
        ) UFSD_DRIVER_PURE_VIRTUAL;

#ifndef UFSD_DRIVER_LINUX
    //Creates sub directory with the specified name
    //  type - defines name type (UNICODE or ASCII)
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int CreateDir(
        IN api::STRType     type,
        IN const void *     name,
        IN size_t           name_len,
        OUT CDir**          dir = NULL // OPTIONAL
        );

    //Opens directory with specified name and places it to the list of
    //open directories.
    //  type - defines type of name (UNICODE or ASCII)
    //  dir  - pointer to the opened directory
    //Returns:
    //  0 - on success
    //  error code otherwise
    virtual int OpenDir(
        IN  api::STRType    type,
        IN  const void*     name,
        IN  size_t          name_len,
        OUT CDir *&         dir
        );

    //Closes opened directory 'dir'
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int CloseDir(
        IN CDir * dir
        );

    //Creates file in this dir
    //  type - defines name type (UNICODE or ASCII)
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int CreateFile(
        IN api::STRType     type,
        IN const void *     name,
        IN size_t           name_len,
        OUT CFile**         file = NULL // OPTIONAL
        );

    //Opens file in this directory
    //  type - defines type of 'name' (UNICODE or ASCII)
    //  file - pointer to the opened file
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int OpenFile(
        IN  api::STRType    type,
        IN  const void*     name,
        IN  size_t          name_len,
        OUT CFile *&        file
        );

    //Closes opened file 'file'
    //Return:
    //  0 - if successful
    //  error code otherwise
    virtual int CloseFile(
        IN CFile * file
        );

    //Set file information
    //  type - file name type (ASCII or UNCODE)
    //  name - file name
    //  fi   - file information
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int SetFileInfo(
        IN api::STRType     type,
        IN const void *     name,
        IN size_t           name_len,
        IN const FileInfo&  fi,
        IN size_t           flags
        );

    // This function gets unique ID of FS object by its' path
    virtual int GetFSObjID(
      IN  api::STRType      type,
      IN  const void*       name,
      IN  size_t            name_len,
      OUT UINT64&           ID,
      IN  int               fFollowSymlink = 1
      );

    //This function creates symbolic link 'name' to symlink
    virtual int CreateSymLink(
        IN api::STRType     type,
        IN const void*      name,
        IN size_t           name_len,
        IN const char*      symlink,
        IN size_t           sym_len,
        OUT UINT64*         pId
        );

    //This function reads symbolic link name
    virtual int GetSymLink(
        IN  api::STRType    type,
        IN  const void*     name,
        IN  size_t          name_len,
        OUT char*           buffer,
        IN  size_t          buflen,
        OUT size_t*         read
        );

    //This function checks is link name referenced to existing file object
    virtual int CheckSymLink(
        IN  api::STRType    type,
        IN  const void*     name,
        IN  size_t          name_len
        );
#endif // #ifndef UFSD_DRIVER_LINUX

    //Deletes file or empty directory with name 'name' in dir;
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int Unlink(
        IN api::STRType     type,
        IN const void *     name,
        IN size_t           name_len,
        IN CFSObject*       Fso = NULL
        ) = 0;

    // Creates a hard link to the file/directory
    // NOTE: hard link to directory usually is not allowed
    virtual int Link(
        IN CFSObject*       Fso,
        IN api::STRType     type,
        IN const void *     name,
        IN size_t           name_len
        ) UFSD_DRIVER_PURE_VIRTUAL;

    //Finds file or directory corresponding to name_mask specified
    //  type - defines ASCII or UNICODE
    //  name_maks - name mask informational, may be ignored
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int StartFind(
        OUT CEntryNumerator*& dentry_num,
        IN  size_t            options = 0 // See UFSD_ENUM_XXX
        ) = 0;

    //Finds file or directory corresponding to name_mask specified
    //  type - defines ASCII or UNICODE
    //  name_maks - name mask informational, may be ignored
    //  fi - file information found
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    //Remark:
    //  Using methods that modify directory content (Create*, Unlink, etc.)
    //  between FindFirst() / FindNext() will lead to unpredictable
    //  search results
    virtual int FindNext(
        IN  CEntryNumerator*  dentry_num,
        OUT FileInfo&         fi
        ) = 0;

    //Retrieve file information
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int GetFileInfo(
        IN  api::STRType    type,
        IN  const void*     name,
        IN  size_t          name_len,
        OUT FileInfo&       fi
        ) = 0;

    //Rename file or directory
    //  src_name -  source name
    //  dst_name -  target name
    //  src      -  ?????
    //  tgt_dir  -  target directory
    //Returns:
    //  0 - if successfully
    //  error code otherwise
    virtual int Rename(
        IN api::STRType     type,
        IN const void*      src_name,
        IN size_t           src_len,
        IN CFSObject*       src,
        IN CDir*            tgt_dir,
        IN const void*      dst_name,
        IN size_t           dst_len
        ) = 0;

    // Counts subdirectories and/or files
    virtual int GetSubDirAndFilesCount(
        OUT bool*   IsEmpty,        // may be NULL
        OUT size_t* SubDirsCount,   // may be NULL
        OUT size_t* FilesCount      // may be NULL
        ) UFSD_DRIVER_PURE_VIRTUAL;

    // Retrieves directory size
    virtual UINT64 GetSize() UFSD_DRIVER_PURE_VIRTUAL;

    //=================================================================
    //          END OF CDir  virtual functions
    //=================================================================

};


// This function returns string like "UFSD version 3.3 (Mar 11 2003 , 20:05:38)"
UFS_UFSD_EXPORT const char*
__stdcall
GetUFSDVersion();


// This function returns string like "Warning: little endian library, big endian platform"
// or NULL if OK
UFS_UFSD_EXPORT const char*
__stdcall
CheckUFSDEndian();


///////////////////////////////////////////////////////////
// Log2
//
// Simple function that returns log2 of MSB (most significant bit)
///////////////////////////////////////////////////////////
static inline unsigned char
Log2( IN size_t Num )
{
  unsigned Ret = 0;
  while( 0 != ( Num >>= 1 ) )
    Ret += 1;

  return (unsigned char)Ret;
}

//===================================
// Functions to work with names
//===================================
// This function translates long name to short one
UFS_UFSD_EXPORT int
GetShortName(
    IN api::IBaseStringManager* Strings,
    IN api::STRType             Type,
    IN const void*              LongName,
    IN size_t                   LongNameLen,
    OUT void*                   ShortName,
    IN size_t                   MaxShortNameLen,
    IN bool                     bDoUpCase,
    OUT size_t*                 ShortNameLen
    );

// This function creates first "~n" name
UFS_UFSD_EXPORT int
GetFirstUniqueName(
    IN api::STRType             Type,
    IN OUT void*                ShortName,
    OUT size_t*                 ShortNameLen,
    IN api::IBaseMemoryManager* Mm
    );

// This function creates next "~n" name
UFS_UFSD_EXPORT int
GetNextUniqueName(
    IN api::STRType             Type,
    IN OUT void*                ShortName,
    OUT size_t*                 ShortNameLen,
    IN api::IBaseMemoryManager* Mm
    );

// This function returns number from "~n" name
UFS_UFSD_EXPORT int
GetUniqueNameNumber(
    IN api::STRType             Type,
    IN void*                    ShortName,
    OUT size_t*                 Number
    );

// This function creates pseudo short name
UFS_UFSD_EXPORT int
GetPseudoShortName(
    IN api::IBaseStringManager* Strings,
    IN api::STRType             Type,
    IN const void*              LongName,
    IN size_t                   LongNameLen,
    OUT void*                   ShortName,
    IN size_t                   MaxShortNameLen,
    IN size_t                   UniqueNum,
    IN bool                     bDoUpCase,
    OUT size_t*                 ShortNameLen,
    IN api::IBaseMemoryManager* Mm
    );


// Most CDir functions gets arguments "type" and "name"
// This function allows to print this name
// NOTE: this function is implemented via U2A()
extern "C"
const char* GetNameA(
    IN CFileSystem* Vcb,
    IN api::STRType type,
    IN const void * name,
    IN size_t       name_len
    );

//===================================
// End of functions to work with names
//===================================

// This function Dumps memory into trace
extern "C" void
UFSD_DumpMemory(
    IN const void*  Mem,
    IN size_t       nBytes
    );

// Returns Gb,Mb to print with "%u.%02u Gb"
UFS_UFSD_EXPORT unsigned int
FormatSizeGb(
    IN  const UINT64& Bytes,
    OUT unsigned int* Mb
    );

// This struct is used in CUFSD::Format functions
// The last bad block has size 0
struct t_BadBlock{
  UINT64  Lbo;
  UINT64  Bytes;
};

// This struct is used to get available format parameters
struct t_FormatParams{
  unsigned int  MinBytesPerCluster;
  unsigned int  MaxBytesPerCluster;
  unsigned int  DefBytesPerCluster;
  unsigned int  BytesPerClusterMask; // possible cluster sizes
};


///////////////////////////////////////////////////////////
//  VerifyBytes
//
//  Lbo       - Start byte to verify
//  Bytes     - Total bytes to verify (relative Start)
//  BadBlocks - The array of bad sectors
//  Msg       - Outlet for messages and progress bar
//
// Verifies sectors for bad blocks
//
//  NOTE: If the function returns ERR_NOERROR
//        then caller should free the returned array BadBlocks
///////////////////////////////////////////////////////////
extern "C" UFS_UFSD_EXPORT int
VerifyBytes(
    IN const UINT64&            Lbo,
    IN const UINT64&            Bytes,
    IN api::IDeviceRWBlock*     Rw,
    IN api::IBaseMemoryManager* Mm,
    IN api::IMessage*           Msg,
    IN const char*              Hint,
    OUT t_BadBlock**            BadBlocks
    );


///////////////////////////////////////////////////////////
// DiscardRange
//
// Discards (not necessary zeroing) range of bytes (at least 512 bytes aligned)
///////////////////////////////////////////////////////////
extern "C" UFS_UFSD_EXPORT int
DiscardRange(
    IN UINT64               Offset,
    IN UINT64               Bytes,
    IN api::IDeviceRWBlock* Rw,
    IN api::IMessage*       Msg,
    IN const char*          Hint
    );


///////////////////////////////////////////////////////////
// ComputeVolId
//
// This function calculates unique volume id
///////////////////////////////////////////////////////////
UFS_UFSD_EXPORT unsigned int
ComputeVolId(
    IN unsigned int Seed,
    IN api::ITime*  Time
    );


// A-la _snwprintf( Buf, MaxChars, L"%u", N )
UFS_UFSD_EXPORT int
ItoW(
    IN  unsigned int    N,
    OUT unsigned short* Buf,
    IN  int             MaxChars
    );

// A-la swscanf( Buf, L"%u", &N )
UFS_UFSD_EXPORT unsigned int
WtoI(
    IN  const unsigned short* Buf,
    IN  int                   Len,
    OUT bool*                 Error
    );


// A-la _snprintf( Buf, MaxChars, "%u", N )
UFS_UFSD_EXPORT int
ItoA(
    IN  unsigned int    N,
    OUT unsigned char*  Buf,
    IN  int             MaxChars
    );

// A-la sscanf( Buf, "%u", &N )
UFS_UFSD_EXPORT unsigned int
AtoI(
    IN  const unsigned char*  Buf,
    IN  int                   Len,
    OUT bool*                 Error
    );

// A-la _sntprintf( Buf, "%llx", N )
UFS_UFSD_EXPORT char*
U64toAh(
    IN  UINT64  N,
    OUT char*   Buf = NULL
    );


///////////////////////////////////////////////////////////
// UFSD_UuidCreate
//
// Simple method of creating GUID's
///////////////////////////////////////////////////////////
extern "C" UFS_UFSD_EXPORT void
UFSD_UuidCreate(
    IN api::ITime*  Tt,
    OUT GUID*       Guid
    );

//
// The minimum size of array to store GUID in string form
// E.g. char szGuid[STRING_GUID_LEN+1]
//      assert( STRING_GUID_LEN == Guid2String( &Guid, szGuid, ARRSIZE(szGuid) ) );
//
#define STRING_GUID_LEN   (16*2 + 4)

///////////////////////////////////////////////////////////
// UFSD_Guid2String
//
// Converts GUID into XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX form
// Returns strlen(szGuid) or 0 if buffer is too small.
///////////////////////////////////////////////////////////
extern "C" UFS_UFSD_EXPORT int
UFSD_Guid2String(
    IN  const GUID* Guid,
    OUT char*       szGuid,
    IN  int         MaxChars,
    IN  bool        bUpCase
    );

#ifdef BASE_BIGENDIAN
  #define INTX_SYMBOLIC_LINK      PU64(0x496E74784C4E4B01)  // "IntxLNK\1"
  #define INTX_CHARACTER_DEVICE   PU64(0x496E747843485200)  // "IntxCHR\0"
  #define INTX_BLOCK_DEVICE       PU64(0x496E7478424C4B00)  // "IntxBLK\0"
#else
  #define INTX_SYMBOLIC_LINK      PU64(0x014B4E4C78746E49)  // "IntxLNK\1"
  #define INTX_CHARACTER_DEVICE   PU64(0x0052484378746E49)  // "IntxCHR\0"
  #define INTX_BLOCK_DEVICE       PU64(0x004B4C4278746E49)  // "IntxBLK\0"
#endif

///////////////////////////////////////////////////////////
// minor_dev
//
// Retrieves minor from encoded device
///////////////////////////////////////////////////////////
static inline unsigned int
minor_dev(
    IN unsigned int dev
    )
{
  return (dev & 0xff) | ((dev >> 12) & 0xfff00);
}


///////////////////////////////////////////////////////////
// major_dev
//
// Retrieves major from encoded device
///////////////////////////////////////////////////////////
static inline unsigned int
major_dev(
    IN unsigned int dev
    )
{
  return (dev & 0xfff00) >> 8;
}


///////////////////////////////////////////////////////////
// encode_dev
//
// Retrieves encoded dev from major/minor
///////////////////////////////////////////////////////////
static inline unsigned int
encode_dev(
    IN unsigned int minor,
    IN unsigned int major
    )
{
  return (minor & 0xff) | (major << 8) | ((minor & ~0xff) << 12);
}

//
// Environment hint
//
extern const char szUFSDVersionEnv[];

//
// Possible options for CDir::StartFind
// Range 0x10000000 - 0x80000000 is reserved for internal use
//

#define UFSD_ENUM_SKIP_SHORT_NAMES    0x00000001    // Do not find short names while enumerating directory
#define UFSD_ENUM_SHOW_META           0x00000002    // Show meta files
#define UFSD_ENUM_SKIP_HIDDEN         0x00000004    // Skip hidden files
#define UFSD_ENUM_NAME_ID_ATTR_ONLY   0x00000008    // collect only name, Id and base attrib (file, dir, symlink)
#define UFSD_ENUM_UNIQUE_ID           0x00000010    // return only unique Id else return UFSD_NOT_UNIQUE_ID

#define UFSD_NOT_UNIQUE_ID            ~0u           // Id if it is't unique

//
// Possible options for CFileSystem::Init()
//

//
// NTFS specific:
//
#define NTFS_OPTIONS_ENABLE_SHORTNAME     0x00000001    // Generate short names when create/rename files
#define NTFS_OPTIONS_ENABLE_PSEUDONAME    0x00000002    // Use pseudo names to search in directories
#define NTFS_OPTIONS_CASE_SENSITIVE       0x00000004    // Use case sensitive names
#define NTFS_OPTIONS_BEST_COMPRESSION     0x00000008    // Use best compression instead of standard
#define NTFS_OPTIONS_CREATE_SPARSE        0x00000020    // By default create sparse file
#define NTFS_OPTIONS_SKIP_INHERIT_PARENT  0x00000040    // Do not inherit parent attributes for new files/dirs (compress/sparse/encrypt)

//
// ExFAT specific:
//
#define EXFAT_OPTIONS_FIX_ONLINE        0x00000040     // Fix some exfat errors online

//
// REFS specific:
//
#define REFS_OPTIONS_CASE_SENSITIVE       0x00000004    // Use case sensitive names
#define REFS_OPTIONS_CREATE_SPARSE        0x00000020    // By default create sparse file
#define REFS_OPTIONS_SKIP_INHERIT_PARENT  0x00000040    // Do not inherit parent attributes for new files/dirs (compress/sparse/encrypt)


//
// ZIP specific:
//

//  0x00000001    // Bit 1
//  0x00000002    // Bit 2
//  0x00000004    // Bit 3
//  0x00000008    // Bit 4

#define ZIP_OPTIONS_WRITE               0x00000010    // 0 read only, 1 write only
#define ZIP_OPTIONS_WRITE_DEFAULT_COMPRESSION   (ZIP_OPTIONS_WRITE | 0x0000000F)

//
// EXTFS2 specific:
//
#define EXTFS2_OPTIONS_NOREPLAY_JNL     0x00000080    // don't mount filesystem if replaying journal required

#define EXTFS2_FLAGS_UNSUPPORTED_FEATURE  0x00100000  // feature not implemented, mounting disabled
#define EXTFS2_FLAGS_READONLY_SUPPORT     0x00200000  // volume is read only compatible
#define EXTFS2_FLAGS_UNSUPPORTED_REPLAY   0x00400000  // journal replaying not supported
#define EXTFS2_FLAGS_UNSUPPORTED_BIGALLOC 0x00800000  // feature bigalloc unsupported
#define EXTFS2_FLAGS_UNSUPPORTED_INLINE_DATA 0x01000000  // feature inline_data unsupported

#define EXTFS2_FLAGS_INCOMPATIBLE_MASK    0x000FFFFF  //mask for incompatible features (s_feature_incompat field of extfs superblock for more information)


//
// APFS specific:
//
#define APFS_FLAGS_UNSUPPORTED_SNAPSHOT   0x00010000  // volume mounted as RO because it has a snapshot
#define APFS_FLAGS_UNSUPPORTED_ENCRYPTED  0x00020000  // volume is encrypted

//
// Common
//
//#define UFSD_OPTIONS_NOBUFFERING          0x80000000    // Do not buffer not aligned read/write
#define UFSD_OPTIONS_RECOGNIZE_FS         0x40000000    // Just recognize file system
#define UFSD_OPTIONS_JNL                  0x20000000    // Activate journal
#define UFSD_CHECKFS_MODE                 0x10000000    // Init FS from checkfs process
#define UFSD_OPTIONS_META_ORDERED         0x08000000    // Write metadata in order to minimize data loss
#define UFSD_OPTIONS_FAST_MOUNT           0x04000000    // Do not scan full bitmap at mount. Do it later. In this case tree of free extents is not used
#define UFSD_OPTIONS_MOUNT_ALL_VOLUMES    0x02000000    // Mount all volumes and display all volumes except 0th in emulated system dir. Otherwise only 0th volume will be mounted
#define UFSD_OPTIONS_IGNORE_RWBLOCK_SIZE  0x01000000    // Ignore check if rwblock size is less than filesystem
#define UFSD_OPTIONS_NO_CREATE_TIME       0x00800000    // "create time" is not used by host

//
// Events while volume initialization/reinitialization
// Lower 24 bits are specific for every filesystem
//
#define UFSD_FLAGS_JOURNAL              0x01000000    // Volume with journal
#define UFSD_FLAGS_REPLAYED             0x02000000    // Volume was replayed
#define UFSD_FLAGS_NEED_REPLAY          0x04000000    // Read-only volume requires replay
#define UFSD_FLAGS_LOGICAL_SECTOR       0x08000000    // Logical sector size does not match physical sector size ( HFSJ and more then 2^31 of sectors )
#define UFSD_FLAGS_NEED_REPLAY_NATIVE   0x10000000    // Native journal is not empty and not possible to replay
#define UFSD_FLAGS_INVALID_JOURNAL      0x20000000    // Native journal is not valid or not supported
#define UFSD_FLAGS_BAD_PASSWORD         0x40000000    // Password is wrong
#define UFSD_FLAGS_ENCRYPTED_VOLUMES    0x80000000    // some volumes is unencrypted (wrong passwords or not specified)

} // namespace UFSD
