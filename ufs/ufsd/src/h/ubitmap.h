// <copyright file="ubitmap.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains declarations of classes:
//
// - CWndBitmap  - windowed bitmap + zone
// - CSpBitmap   - sparsed bitmap  + zone
//
////////////////////////////////////////////////////////////////

namespace UFSD {

#if defined UFSD_NTFS | defined UFSD_EXFAT | defined UFSD_FAT | defined UFSD_BTRFS | defined UFSD_APFS | defined UFSD_EXTFS2
  // NTFS/exFAT use little endian bits order
  #define UFSD_NEEDS_BITMAP_LE
#endif

#if defined UFSD_HFS | defined UFSD_EXTFS2
  // HFS uses big endian bits order
  #define UFSD_NEEDS_BITMAP_BE
#endif

#if defined UFSD_NEEDS_BITMAP_BE | defined UFSD_NEEDS_BITMAP_LE

struct bitmap_operations {
  int (*read)( void* Arg, const UINT64& Vbo, size_t Bytes, void* Buf );
  int (*write)( void* Arg, const UINT64& Vbo, size_t Bytes, const void* Buf );
#ifdef UFSD_DRIVER_LINUX
  int (*map)( void* Arg, const UINT64& Vbo, size_t Bytes, BcbBuf* BhBuf );
  void (*umap)( void* Arg, void* bh );
  int (*get_access)( void* Arg, void* bh );
  int (*dirty)( void* Arg, void* bh );
  void (*mark_free_meta)( void* Arg, int (*cb_func)( IN void*  ctx, IN size_t start, IN size_t end ), void* ctx );
#endif
};

typedef const bitmap_operations* RWFUN;

typedef
void( *CBFUN )(
    IN size_t FirstBit,
    IN size_t Bits,
    IN int    AsUsed,
    IN void*  CbArg
    );

#if defined UFSD_NTFS_STAT
  #define BITMAP_STAT(x) x
#else
  #define BITMAP_STAT(x)
#endif


///////////////////////////////////////////////////////////
// GetBitmapSizeQuadAligned
//
// NTFS uses quad aligned bitmaps
///////////////////////////////////////////////////////////
static inline size_t
GetBitmapSizeQuadAligned(
    IN size_t Bits
    )
{
  //
  // Which code is faster on all platforms?
  //
  return QuadAlign( (Bits+7) >> 3 );

//  return ((size_t)(((UINT64)Bits +7) >> 3) + 7) & ~7;
//  return QuadAlign( (Bits>>3) + ((Bits&7)?1 : 0) );
}


///////////////////////////////////////////////////////////
// GetBitmapSize
//
// exFAT/HFS uses byte aligned bitmaps
///////////////////////////////////////////////////////////
static inline unsigned int
GetBitmapSize(
    IN unsigned int Bits
    )
{
  return Bits >= 0xFFFFFFF9? 0x20000000 : ((Bits+7) >> 3);
}

#if defined UFSD_NTFS || defined UFSD_HFS
  #define UFSD_USE_BITMAP_ZONE
#endif


// Possible values for 'Flags' in 'FindFree'
#define BITMAP_FIND_MARK_AS_USED  0x01
#define BITMAP_FIND_FULL          0x02

#if defined UFSD_NEEDS_BITMAP_BE & defined UFSD_NEEDS_BITMAP_LE
struct BitmapFuncs
{
  size_t (*m_GetNumberOfSetBits)(
      IN const unsigned char* Map,
      IN size_t               FirstBit,
      IN size_t               nBits
      );

  size_t (*m_GetFreeBitPos)(
      IN const unsigned char* Map,
      IN size_t               FirstBit,
      IN size_t               nBits
      );

  size_t  (*m_GetUsedBitPos)(
      IN const unsigned char* Map,
      IN size_t               FirstBit,
      IN size_t               nBits
      );

  bool (*m_GetBit)(
      IN const unsigned char* Map,
      IN size_t               Bit
      );

  void (*m_SetBits)(
      IN OUT unsigned char* Map,
      IN size_t             FirstBit,
      IN size_t             nBits
      );

  void (*m_ClearBits)(
      IN OUT unsigned char* Map,
      IN size_t             FirstBit,
      IN size_t             nBits
      );

  bool (*m_AreBitsClear)(
      IN const unsigned char* Map,
      IN size_t               FirstBit,
      IN size_t               nBits
      );

  bool (*m_AreBitsSet)(
      IN const unsigned char* Map,
      IN size_t               FirstBit,
      IN size_t               nBits
      );

//   size_t (*m_FindClearBits)(
//       IN const unsigned char* Map,
//       IN size_t               TotalBits,
//       IN size_t               FirstBit,
//       IN size_t               BitsToFind,
//       IN bool                 bOnlyUp
//       );

//   size_t (*m_GetTailFree)(
//       IN const unsigned char* Map,
//       IN size_t               TotalBits
//       );

  void InitFuncs(
      bool bBigEndian
      );
};
#endif // #if defined UFSD_NEEDS_BITMAP_BE & defined UFSD_NEEDS_BITMAP_LE


#ifdef UFSD_VIRTUAL_BITMAP

struct BASE_ABSTRACT_CLASS IBitmap
{
  size_t  m_nBits;
  CBFUN   m_CBFunc;
  void*   m_CBArg;

  virtual int Init(
      IN size_t       nBits,
      IN unsigned int BytesPerWnd = 4096,
      IN RWFUN        RwFun       = NULL,
      IN void*        RwArg       = NULL
      ) = 0;

  virtual void    Destroy() = 0;

  virtual bool    IsDirty()     const = 0;
  virtual size_t  GetOnes()     const = 0;
  virtual size_t  GetZeros()    const = 0;

  BITMAP_STAT(
    virtual void    GetReadWriteStat(
      OUT UINT64* BytesRead,
      OUT size_t* Reads,
      OUT UINT64* BytesWrite,
      OUT size_t* Writes
      ) const = 0;
  )

  // Copy bitmap to supplied buffer
  virtual int GetBitmap(
      IN unsigned char* Buf,
      IN size_t         BytesOffset,
      IN size_t         Bytes
      ) const = 0;

  // Returns true if all clusters [FirstBit, FirstBit+nBits) are free
  virtual bool IsFree(
      IN size_t FirstBit,
      IN size_t nBits = 1
      ) = 0;

  // Returns true if all clusters [FirstBit, FirstBit+nBits) are used
  virtual bool IsUsed(
      IN size_t FirstBit,
      IN size_t nBits = 1
      ) = 0;

#if defined UFSD_HFS
  virtual size_t  GetFirstFree() = 0;
#endif

  // This function looks for free space
  // Returns 0 if not found
  virtual size_t FindFree(
      IN  size_t          ToAllocate,
      IN  size_t          Hint,
      IN  const size_t*   MaxAlloc,
      IN  const unsigned* Align,    // If not NULL then aligned request
      IN  size_t          Flags,    // See BITMAP_FIND_XXX flags
      OUT size_t*         Allocated
      ) = 0;

  // Marks the bits range from FirstBit to FirstBit + Bits as free
  virtual int  MarkAsFree(
      IN size_t FirstBit,
      IN size_t Bits = 1
      ) = 0;

  // Marks the bits range from FirstBit to FirstBit + Bits as used
  virtual int  MarkAsUsed(
      IN size_t FirstBit,
      IN size_t Bits = 1
      ) = 0;

  // Write bitmap
  virtual int Write(
      IN RWFUN  RwFun,
      IN void*  RwArg
      ) = 0;

  // Resize bitmap
  virtual int Resize(
      IN size_t nBits
      ) = 0;

#ifdef UFSD_USE_BITMAP_ZONE
  virtual size_t  GetZoneBit()  const = 0;
  virtual size_t  GetZoneLen()  const = 0;

  virtual void SetZone(
      IN size_t Lcn,
      IN size_t Len
      ) = 0;
#endif

#if defined UFSD_FAT_DEFRAG | defined UFSD_NTFS_DEFRAG | defined UFSD_NTFS_DEFRAG2
  virtual void GetBitmapStat(
      OUT BitmapStat* Stat
      ) = 0;
#endif

#ifdef UFSD_USE_GET_MEMORY_USAGE
  // Returns the size of allocated memory in bytes
  virtual size_t GetMemSize(
        IN bool bItSelf
        ) const = 0;
#endif

#ifdef UFSD_NTFS_SHRINK
  virtual int GetNumberOfTailBits(
      IN size_t   Lcn,
      OUT size_t* Used
      ) = 0;
#endif

#if defined UFSD_TRACE && !defined NDEBUG
  // Traces information about current bitmap
  virtual void Trace() = 0;
#endif
};


///////////////////////////////////////////////////////////
// CompareBitmaps
//
// Returns:
// - 1  can't get bitmap
// + 1  bitmaps differ
//   0  bitmaps identical
///////////////////////////////////////////////////////////
int
CompareBitmaps(
    IN const IBitmap*   Bitmap1,
    IN const IBitmap*   Bitmap2
    );

#endif // #ifdef UFSD_VIRTUAL_BITMAP


#ifdef UFSD_USE_SPBITMAP

#define UFSD_SPBITMAP_DIRTY  1

//=============================================================================
//
// Sparse Bitmap wrapper
//
//=============================================================================
struct CSpBitmap : public UMemBased<CSpBitmap>
#ifdef UFSD_VIRTUAL_BITMAP
    , public IBitmap
#endif
#if defined UFSD_NEEDS_BITMAP_BE & defined UFSD_NEEDS_BITMAP_LE
    , public BitmapFuncs
#endif
{
  // Declare but not implement
  CSpBitmap( const CSpBitmap& );
  CSpBitmap& operator=( const CSpBitmap& );

  size_t*           m_Bitmaps;
  unsigned short*   m_FreeBits;

  unsigned int      m_BitsPerWnd;           // <= 8*4096
  unsigned char     m_Log2OfBits;           // (1 << m_Log2OfBits) == m_BitsPerWnd
  unsigned char     Res[sizeof(size_t)-1];  // Align

  size_t            m_nFreeBits;            // Keep track total number of free bits
  unsigned int      m_nBitsInLastWnd;       // Count of bits in last window
  size_t            m_nWnd;                 // The total number of windows

  bool              m_bSetTail;
  bool              m_bBigEndian;           // Big endian bits order

  size_t            m_nAllocated;           // Number of windows with allocated buffers
#ifdef UFSD_TRACE
  size_t            m_nMaxAllocated;        // Maximum number of allocated windows
#endif

#ifdef UFSD_USE_BITMAP_ZONE
  //--------------------
  // Zone [Lcn, End)
  //--------------------
  size_t            m_ZoneBit;
  size_t            m_ZoneEnd;

  size_t  GetZoneBit()  const { return m_ZoneBit; }
  size_t  GetZoneLen()  const { return m_ZoneEnd - m_ZoneBit; }

  void SetZone( IN size_t Lcn, IN size_t Len )
  {
    m_ZoneBit = Lcn;
    m_ZoneEnd = Lcn + Len;
  }
#endif

  //--------------------
  // Statistics
  //--------------------
  BITMAP_STAT (
    size_t    m_Reads;
    UINT64    m_BytesRead;
    UINT64    m_BytesWrite;
    size_t    m_Writes;
  )

#ifndef UFSD_VIRTUAL_BITMAP
  CBFUN   m_CBFunc;
  void*   m_CBArg;

  size_t  m_nBits;
#endif

  bool              m_bDirty;               // true if any of windows is dirty

  unsigned char* GetWndBitmap( IN size_t Wnd ) const {
    return (unsigned char*)(m_Bitmaps[Wnd] & ~UFSD_SPBITMAP_DIRTY);
  }

  unsigned int GetWndBits( IN size_t Wnd ) const{
    return Wnd == m_nWnd-1? m_nBitsInLastWnd : m_BitsPerWnd;
  }

  // Returns 0 if processing was aborted
  bool ProcessBlock( IN size_t FirstBit,
                     IN size_t Bits,
                     IN bool (CSpBitmap::*fn)(
                          IN size_t   iWnd,
                          IN unsigned int FirstBit,
                          IN unsigned int Bits,
                          OUT size_t* Ret
                         ),
                    OUT size_t* Ret = NULL
                  );

  //=======================================================
  //            CALLBACK FUNCTIONS passed to ProcessBlock
  //
  // Each of this function returns 0 if processing should be aborted
  //=======================================================
  bool MarkAsFree2( IN size_t iWnd, IN unsigned int FirstBit, IN unsigned int Bits, OUT size_t* Ret );
  bool MarkAsUsed2( IN size_t iWnd, IN unsigned int FirstBit, IN unsigned int Bits, OUT size_t* Ret );
  bool IsFree2( IN size_t iWnd, IN unsigned int FirstBit, IN unsigned int Bits, OUT size_t* Ret );
  bool IsUsed2( IN size_t iWnd, IN unsigned int FirstBit, IN unsigned int Bits, OUT size_t* Ret );
  bool GetNumberOfTailBits2( IN size_t iWnd, IN unsigned int FirstBit, IN unsigned int Bits, OUT size_t* Ret );
  //=======================================================
  //            END OF CALLBACK FUNCTIONS
  //=======================================================

  bool Check() const; // debug version

  CSpBitmap( api::IBaseMemoryManager* Mm, bool bSetTail = false, bool bBigEndian = false )
      : UMemBased<CSpBitmap>( Mm )
      , m_Bitmaps(NULL)
      , m_FreeBits(NULL)
      , m_nFreeBits(0)
      , m_nWnd( 0 )
      , m_bSetTail( bSetTail )
      , m_bBigEndian( bBigEndian )
      , m_nAllocated( 0 )
#ifdef UFSD_TRACE
      , m_nMaxAllocated( 0 )
#endif
      , m_bDirty( false )
  {
    m_nBits   = 0;
    m_CBFunc  = NULL;
    m_CBArg   = NULL;
  }

  ~CSpBitmap()
  {
    Clear();
  }

  // Changes Rw pointer and arguments
  void SetRwFun(
      IN RWFUN,
      IN void*
      )
  {
  }

  void Clear(
      IN bool bKeepWnd = false
      );

  size_t GetUsedBit(
      IN size_t FirstBit,
      IN size_t Bits
      ) const;

  size_t GetFreeBit(
      IN size_t FirstBit,
      IN size_t Bits
      );

  bool Clone(
      IN const CSpBitmap* Bitmap
      );

  void Swap(
      IN CSpBitmap* Bitmap
      );

  //=======================================================
  //              IBitmap interface
  //=======================================================

  int Init(
      IN size_t       nBits,
      IN unsigned int BytesPerWnd = 4096,
      IN RWFUN        RwFun       = NULL,
      IN void*        RwArg       = NULL
      );

  void    Destroy() { delete this; }

  bool    IsDirty()     const { return m_bDirty; }
  size_t  GetOnes()     const { return m_nBits - GetZeros(); }
  size_t  GetZeros()    const {
#ifndef NDEBUG
    size_t Free = 0;
    for ( size_t i = 0; i < m_nWnd; i++ )
      Free += m_FreeBits[i];

    assert( Free == m_nFreeBits );
    assert( Free <= m_nBits );
#endif
    assert( m_nFreeBits <= m_nBits );
    return m_nFreeBits;
  }

  BITMAP_STAT (
    void   GetReadWriteStat(
        OUT UINT64* BytesRead,
        OUT size_t* Reads,
        OUT UINT64* BytesWrite,
        OUT size_t* Writes
        ) const
    {
      *BytesRead  = m_BytesRead;
      *Reads      = m_Reads;
      *BytesWrite = m_BytesWrite;
      *Writes     = m_Writes;
    }
  )

  // Copy bitmap to supplied buffer
  int GetBitmap(
      IN unsigned char* Buf,
      IN size_t         BytesOffset,
      IN size_t         Bytes
      ) const;

  // Returns true if all clusters [FirstBit, FirstBit+nBits) are free
  bool IsFree(
      IN size_t FirstBit,
      IN size_t nBits = 1
      )
  {
    return ProcessBlock( FirstBit, nBits, &CSpBitmap::IsFree2 );
  }

  // Returns true if all clusters [FirstBit, FirstBit+nBits) are used
  bool IsUsed(
      IN size_t FirstBit,
      IN size_t nBits = 1
      )
  {
    return ProcessBlock( FirstBit, nBits, &CSpBitmap::IsUsed2 );
  }

#if defined UFSD_HFS
  size_t  GetFirstFree();
#endif

  // This function looks for free space
  // Returns 0 if not found
  size_t FindFree(
      IN  size_t          ToAllocate,
      IN  size_t          Hint,
      IN  const size_t*   MaxAlloc,
      IN  const unsigned* Align,    // If not NULL then aligned request
      IN  size_t          Flags,    // See BITMAP_FIND_XXX flags
      OUT size_t*         Allocated
      );

  // Marks the bits range from FirstBit to FirstBit + Bits as free
  int  MarkAsFree(
      IN size_t FirstBit,
      IN size_t Bits = 1
      )
  {
    if ( NULL != m_CBFunc )
      (*m_CBFunc)( FirstBit, Bits, 0, m_CBArg );

    ProcessBlock( FirstBit, Bits, &CSpBitmap::MarkAsFree2 );

    return ERR_NOERROR;
  }

  // Marks the bits range from FirstBit to FirstBit + Bits as used
  int  MarkAsUsed(
      IN size_t FirstBit,
      IN size_t Bits = 1
      )
  {
    if ( NULL != m_CBFunc )
      (*m_CBFunc)( FirstBit, Bits, 1, m_CBArg );

    ProcessBlock( FirstBit, Bits, &CSpBitmap::MarkAsUsed2 );

    return ERR_NOERROR;
  }

  // Write bitmap
  int Write(
      IN RWFUN  RwFun,
      IN void*  RwArg
      );

  // Resize bitmap
  int Resize(
      IN size_t nBits
      );

#if defined UFSD_FAT_DEFRAG | defined UFSD_NTFS_DEFRAG | defined UFSD_NTFS_DEFRAG2
  void GetBitmapStat(
      OUT BitmapStat* Stat
      );
#endif

#if defined UFSD_USE_GET_MEMORY_USAGE | (defined UFSD_TRACE & (defined UFSD_NTFS_CHECK | defined UFSD_HFS_CHECK) )
  // Returns the size of allocated memory in bytes
  size_t GetMemSize(
        IN bool bItSelf
        ) const
  {
    size_t Ret = m_nWnd * (sizeof(short) + sizeof(char*)) + (bItSelf? sizeof(*this) : 0);
    Ret += m_nAllocated * (m_BitsPerWnd>>3);
    return Ret;
  }
#endif

#ifdef UFSD_NTFS_SHRINK
  int GetNumberOfTailBits(
      IN size_t   Lcn,
      OUT size_t* Used
      )
  {
    *Used = 0;
    assert( Lcn < m_nBits );
    assert( m_ZoneBit == m_ZoneEnd );

    if ( Lcn >= m_nBits )
      return ERR_NOERROR;

    verify( ProcessBlock( Lcn, m_nBits - Lcn, &CSpBitmap::GetNumberOfTailBits2, Used ) );

    return ERR_NOERROR;
  }
#endif

#if defined UFSD_TRACE && !defined NDEBUG
  // Traces information about current bitmap
  void Trace();
#endif

  //=======================================================
  //              End of IBitmap interface
  //=======================================================

#if defined UFSD_NTFS_CHECK | defined UFSD_HFS_CHECK | defined UFSD_EXFAT_CHECK
  // Trims right boundary so that total allocated memory will be not greater than 'Bytes'
  // Returns new right boundary or 0 if not trimmed
  size_t TrimRight(
        IN size_t Bytes
        );
#endif

};

#endif // #ifdef UFSD_USE_SPBITMAP


#ifdef UFSD_USE_SPBITMAP_MEM
//=============================================================================
//
// Sparse-InMemory Bitmap wrapper
//
//=============================================================================
struct CSpBitmapMem : public UMemBased<CSpBitmapMem>
{
  // Declare but not implement
  CSpBitmapMem( const CSpBitmapMem& );
  CSpBitmapMem& operator=( const CSpBitmapMem& );

  unsigned char**   m_Bitmaps;
  unsigned short*   m_FreeBits;

  size_t            m_nBits;
  size_t            m_nWnd;                 // The total number of windows

  unsigned short    m_BitsPerWnd;           // <= 8*4096
  unsigned short    m_nBitsInLastWnd;       // Count of bits in last window
  unsigned char     m_Log2OfBits;           // (1 << m_Log2OfBits) == m_BitsPerWnd


  unsigned int GetWndBits( IN size_t Wnd ) const{
    return Wnd == m_nWnd-1? m_nBitsInLastWnd : m_BitsPerWnd;
  }

  bool Check() const; // debug version

  CSpBitmapMem( api::IBaseMemoryManager* Mm )
      : UMemBased<CSpBitmapMem>( Mm )
  {
  }

  ~CSpBitmapMem()
  {
    Dtor();
  }

  void Dtor();

  //=======================================================
  //              IBitmap interface
  //=======================================================
  int Init(
      IN size_t       nBits,
      IN unsigned int BytesPerWnd = 4096,
      IN RWFUN        RwFun       = NULL,
      IN void*        RwArg       = NULL
      );

  // Returns true if all clusters [FirstBit, FirstBit+nBits) are free
  bool IsFree(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

  // Returns true if all clusters [FirstBit, FirstBit+nBits) are used
  bool IsUsed(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

  // This function looks for free space
  // Returns 0 if not found
  size_t FindFree(
      IN  size_t          ToAllocate,
      IN  size_t          Hint,
      IN  const size_t*   MaxAlloc,
      IN  const unsigned* Align,    // If not NULL then aligned request
      IN  size_t          Flags,    // See BITMAP_FIND_XXX flags
      OUT size_t*         Allocated
      );

  // Marks the bits range from FirstBit to FirstBit + Bits as free
  int  MarkAsFree(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

  // Marks the bits range from FirstBit to FirstBit + Bits as used
  int  MarkAsUsed(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

  // Resize bitmap
  int Resize(
      IN size_t nBits
      );

#ifdef UFSD_USE_GET_MEMORY_USAGE
  // Returns the size of allocated memory in bytes
  size_t GetMemSize(
        IN bool bItSelf
        ) const
  {
    size_t Ret = m_nWnd * (sizeof(short) + sizeof(char)) + (bItSelf? sizeof(*this) : 0);
    size_t BytesPerWnd = m_BitsPerWnd>>3;
    for ( size_t i = 0; i < m_nWnd; i++ )
    {
      if ( NULL != m_Bitmaps[i] )
        Ret += BytesPerWnd;
    }

    return Ret;
  }
#endif
  //=======================================================
  //              End of IBitmap interface
  //=======================================================

  void Reset();
};
#endif // #ifdef UFSD_USE_SPBITMAP_MEM


#ifdef UFSD_USE_WNDBITMAP

//=============================================================================
//
// Advanced Bitmap wrapper
//
// Uses memory window to read/write/manage bitmaps
// Zone is stored in member m_Zone(Lcn)(Len)
// The m_BitmapWindow always contains zone marked as used
// Before writing to disk this zone is freed and marks again after
// writing
//
//=============================================================================
struct OnFreeMetaData;

struct CWndBitmap : public UMemBased<CWndBitmap>
#ifdef UFSD_VIRTUAL_BITMAP
    , public IBitmap
#endif
#if defined UFSD_NEEDS_BITMAP_BE & defined UFSD_NEEDS_BITMAP_LE
    , public BitmapFuncs
#endif
{
  struct MAP{ size_t Bit, Len; };

  // Declare but not implement
  CWndBitmap( const CWndBitmap& );
  CWndBitmap& operator=( const CWndBitmap& );

  unsigned short    m_BitsPerWnd;           // <= 8*4096
  unsigned short    m_FreeHolder[6];        // holder for m_FreeBits
  unsigned char     m_Log2OfBits;           // (1 << m_Log2OfBits) == m_BitsPerWnd
  unsigned char     Res;                    // Align

  // NOTE: m_BitmapWindow must followed 'm_Bcb'
#ifdef UFSD_DRIVER_LINUX
#if defined UFSD_NTFS && defined UFSD_NTFS_JNL
  unsigned char*    m_BitmapWindowInMem;    // Modified copy of Bitmap window
#endif
  void*             m_Bcb;
#endif
  unsigned char*    m_BitmapWindow;         // Bitmap window (4K)

  RWFUN             m_RwFun;
  void*             m_RwArg;

  size_t            m_nFreeBits;            // Keep track total number of free bits
  unsigned short*   m_FreeBits;             // Array of free bits informations about windows
  size_t            m_nWnd;                 // The total number of windows
  unsigned int      m_nBitsInLastWnd;       // Count of bits in last window

  unsigned char*    m_LoadedBitmap;         // Bitmap of loaded (read from disk) windows. May be NULL if all loaded
  size_t            m_LoadedWnd;            // Total number of loaded windows. When == m_nWnd then all bitmap is loaded

  avl_tree          m_ExtentStartTree;      // Tree of extents, sorted by 'start'
  avl_tree          m_ExtentCountTree;      // Tree of extents, sorted by 'count + start'
  int               m_Uptodated;            // -1 Tree is activated but not updated (too many fragments)
                                            // 0 - Tree is not activated
                                            // 1 - Tree is activated and updated
  size_t            m_MinExtent;            // Minimal extent used while building
  size_t            m_MaxExtent;            // Upper estimate of biggest free block
  size_t            m_CurrentWnd;           // Current window number
#ifdef UFSD_DRIVER_LINUX
  size_t            m_AccessedWnd;          // Current accessed window number
#endif
  bool              m_bDirty;               // Is current Bitmap window dirty
  bool              m_bSetTail;
  bool              m_bInited;
  bool              m_bBigEndian;           // Big endian bits order
  bool              m_bRwMap;               // Use RW_MAP instead of read/write
  bool              m_bDelayLoad;           // Delay load bitmap

#ifdef UFSD_USE_BITMAP_ZONE
  //--------------------
  // Zone [Lcn, End)
  //--------------------
  size_t            m_ZoneBit;
  size_t            m_ZoneEnd;

  size_t  GetZoneBit()  const { return m_ZoneBit; }
  size_t  GetZoneLen()  const { return m_ZoneEnd - m_ZoneBit; }

  void SetZone(
      IN size_t Lcn,
      IN size_t Len
      );
#endif

  //--------------------
  // Statistics
  //--------------------
  BITMAP_STAT (
    mutable size_t    m_Reads;
    mutable UINT64    m_BytesRead;
    UINT64    m_BytesWrite;
    size_t    m_Writes;
  )

#ifndef UFSD_VIRTUAL_BITMAP
  CBFUN   m_CBFunc;
  void*   m_CBArg;

  size_t  m_nBits;
#endif

  // Increments free bits info of given window
  void IncWndInfo(
      IN size_t wnd,
      IN int    inc
      )
  {
    m_FreeBits[wnd] = (unsigned short)(m_FreeBits[wnd] + inc);
  }

  //--------------------
  // End of Window operations
  //--------------------
  unsigned int GetWndBits( IN size_t Wnd ) const{
    return Wnd + 1 == m_nWnd? m_nBitsInLastWnd : m_BitsPerWnd;
  }

#if defined UFSD_DRIVER_LINUX && defined UFSD_NTFS && defined UFSD_NTFS_JNL
  int OnEachWndFrag(
      IN size_t     start,
      IN size_t     end,
      IN OUT OnFreeMetaData* Fm
      );

  bool    m_bUseMetaJnl;
  // Mark deallocated but not committed blocks as used
  // Returns window bitmap (may be copied + modified)
  const unsigned char* UpdateMetaFreeOn();
#else
  // suppress warning if ( m_bUseMetaJnl ) ....
  #define m_bUseMetaJnl       ((void)0,false)
  #define UpdateMetaFreeOn()  m_BitmapWindow
#endif

  int ReadWindow( IN size_t iWnd );
  int SyncWindow();

  bool Check( bool bFull ); // debug version
  bool CheckTree( IN bool bFull );
  void TraceTree(
      IN unsigned nExtents
      );

  void DestroyTree();

  CWndBitmap( api::IBaseMemoryManager* Mm )
      : UMemBased<CWndBitmap>( Mm )
  {
    assert( 0 == m_nBits );
    assert( NULL == m_CBFunc );
    assert( NULL == m_CBArg );
  }

  ~CWndBitmap() { Dtor(); }

  void Dtor();

  // Changes Rw pointer and arguments
  void SetRwFun(
      IN RWFUN  RwFun,
      IN void*  RwArg
      )
  {
    m_RwFun = RwFun;
    m_RwArg = RwArg;
  }

  void AddFreeExtent(
      IN size_t Bit,
      IN size_t Len,
      IN bool   bBuild  // 1 when building tree
      );

  void RemoveFreeExtent(
      IN size_t Bit,
      IN size_t Len
      );

  int ReScan();

  //=======================================================
  //              IBitmap interface
  //=======================================================

  int Init(
      IN size_t       nBits,
      IN unsigned int BytesPerWnd = 4096,
      IN RWFUN        RwFun       = NULL,
      IN void*        RwArg       = NULL
      );

  void    Destroy() { delete this; }

  bool    IsDirty()     const { return m_bDirty; }
  size_t  GetOnes()     const { return m_nBits - GetZeros(); }
  size_t  GetZeros()    const;

  BITMAP_STAT (
    void   GetReadWriteStat(
        OUT UINT64* BytesRead,
        OUT size_t* Reads,
        OUT UINT64* BytesWrite,
        OUT size_t* Writes
        ) const
    {
      *BytesRead  = m_BytesRead;
      *Reads      = m_Reads;
      *BytesWrite = m_BytesWrite;
      *Writes     = m_Writes;
    }
  )

  // Copy bitmap to supplied buffer
  int GetBitmap(
      IN unsigned char* Buf,
      IN size_t         BytesOffset,
      IN size_t         Bytes
      ) const;

  // Returns true if all clusters [FirstBit, FirstBit+Bits) are free
  bool IsFree(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

#if !defined UFSD_DRIVER_LINUX || !defined NDEBUG ||\
  ( !defined UFSD_NO_USE_IOCTL && ( defined UFSD_NTFS || defined UFSD_HFS || defined UFSD_EXFAT || defined UFSD_APFS ) )
  // Returns true if all clusters [FirstBit, FirstBit+Bits) are used
  bool IsUsed(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );
#endif

#if defined UFSD_HFS || defined UFSD_APFS
  size_t  GetFirstFree();
#endif

  // This function looks for free space
  // Returns 0 if not found
  size_t FindFree(
      IN  size_t          ToAllocate,
      IN  size_t          Hint,
      IN  const size_t*   MaxAlloc,
      IN  const unsigned* Align,    // If not NULL then aligned request
      IN  size_t          Flags,    // See BITMAP_FIND_XXX flags
      OUT size_t*         Allocated
      );

  // Marks the bits range from FirstBit to FirstBit + Bits as free
  int  MarkAsFree(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

  // Marks the bits range from FirstBit to FirstBit + Bits as used
  int  MarkAsUsed(
      IN size_t FirstBit,
      IN size_t Bits = 1
      );

  // Write bitmap
  int Write(
      IN RWFUN  RwFun,
      IN void*  RwArg
      )
  {
    m_RwFun = RwFun;
    m_RwArg = RwArg;
    // Write current window
    return SyncWindow();
  }

  // Resize bitmap
  int Resize(
      IN size_t nBits
      );

#if defined UFSD_FAT_DEFRAG | defined UFSD_NTFS_DEFRAG | defined UFSD_NTFS_DEFRAG2
  void GetBitmapStat(
      OUT BitmapStat*
      )
  {
    assert( !"TODO: Should never be called");
  }
#endif

#ifdef UFSD_USE_GET_MEMORY_USAGE
  // Returns the size of allocated memory in bytes
  size_t GetMemSize(
        IN bool bItSelf
        ) const;
#endif

#ifdef UFSD_NTFS_SHRINK
  int GetNumberOfTailBits(
      IN size_t   Lcn,
      OUT size_t* Used
      );
#endif

#if defined UFSD_TRACE && !defined NDEBUG
  // Traces information about current bitmap
  void Trace();
#endif

  //=======================================================
  //              End of IBitmap interface
  //=======================================================
};
#endif // #ifdef UFSD_USE_WNDBITMAP

#endif // #if defined UFSD_NEEDS_BITMAP_BE | defined UFSD_NEEDS_BITMAP_LE

} //namespace UFSD {
