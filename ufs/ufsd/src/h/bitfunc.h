// <copyright file="bitfunc.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//

namespace UFSD {


//
// Calculates '( n + 7 ) / 8'
// Can be used for n >= 0xfffffff9
//
#define Bits2Bytes(n)   (((n)>>3) + (((n)&7)?1:0))


// "true" when [s,s+c) intersects with [l,l+w)
#define IS_IN_RANGE(s,c,l,w)                            \
  ( ( (c) > 0 && (w) > 0 )                              \
   && ( ( (l) <= (s) && (s) < ((l)+(w)) )                \
     || ( (s) <= (l) && ((s)+(c)) >= ((l)+(w)) )         \
     || ( (l) <  ((s)+(c)) && ((s)+(c)) < ((l)+(w)) ) ) )

// "true" when [s,se) intersects with [l,le)
#define IS_IN_RANGE2(s,se,l,le)          \
  ( ( (se) > (s) && (le) > (l) )         \
   && ( ( (l) <= (s) && (s) < (le) )     \
     || ( (s) <= (l) && (se) >= (le) )   \
     || ( (l) <  (se) && (se) < (le) ) ) )

#if defined UFSD_HFS | defined UFSD_EXTFS2
  #define UFSD_NEEDS_BITFUNC_BE
#endif

#if defined UFSD_NTFS || defined UFSD_EXTFS2 || defined UFSD_BTRFS || defined UFSD_EXFAT || defined UFSD_FAT || defined UFSD_APFS \
  || defined UFSD_REFS || defined UFSD_REFS3 || ( defined UFSD_HFS && defined UFSD_HFS_GET_VOLUME_META_BITMAP )
  #define UFSD_NEEDS_BITFUNC_LE
#endif

//
// Operator Precedence:  [] % << >> &
//

///////////////////////////////////////////////////////////
// GetBit
//
// Get bit value
///////////////////////////////////////////////////////////
static inline bool
GetBit(
    IN const unsigned char* Map,
    IN size_t               Bit
    )
{
  return 0 != ( Map[Bit>>3] >> (Bit & 7) & 1 );
}

///////////////////////////////////////////////////////////
// SetBit
//
// Set bit value
///////////////////////////////////////////////////////////
static inline void
SetBit(
    IN OUT unsigned char* Map,
    IN size_t             Bit
    )
{
  Map[Bit>>3] |= (unsigned char)( 1 << (Bit & 7) );
}

///////////////////////////////////////////////////////////
// ClearBit
//
// Clear bit value
///////////////////////////////////////////////////////////
static inline void
ClearBit(
    IN OUT unsigned char* Map,
    IN size_t             Bit
    )
{
  Map[Bit>>3] &= (unsigned char)( ~(1 << (Bit & 7)) );
}

///////////////////////////////////////////////////////////
// GetBitBe
//
// Get bit value
///////////////////////////////////////////////////////////
static inline bool
GetBitBe(
    IN const unsigned char* Map,
    IN size_t               Bit
    )
{
  return 0 != ( Map[Bit>>3] >> (7 - (Bit & 7)) & 1 );
}

///////////////////////////////////////////////////////////
// SetBitBe
//
// Set bit value
///////////////////////////////////////////////////////////
static inline void
SetBitBe(
    IN OUT unsigned char* Map,
    IN size_t             Bit
    )
{
  Map[Bit>>3] |= (unsigned char)( 1 << (7 - (Bit & 7) ) );
}

///////////////////////////////////////////////////////////
// ClearBitBe
//
// Clear bit value
///////////////////////////////////////////////////////////
static inline void
ClearBitBe(
    IN OUT unsigned char* Map,
    IN size_t             Bit
    )
{
  Map[Bit>>3] &= (unsigned char)( ~(1 << (7 - (Bit & 7)) ) );
}


#ifdef UFSD_NEEDS_BITFUNC_LE

///////////////////////////////////////////////////////////
// SetBits
//
// Set block of bits
///////////////////////////////////////////////////////////
void
SetBits(
    IN OUT unsigned char* Map,
    IN size_t             FirstBit,
    IN size_t             nBits
    );

///////////////////////////////////////////////////////////
// ClearBits
//
// Clear block of bits
///////////////////////////////////////////////////////////
void
ClearBits(
    IN OUT unsigned char* Map,
    IN size_t             FirstBit,
    IN size_t             nBits
    );

///////////////////////////////////////////////////////////
// AreBitsClear
//
// Returns true if all bits [FirstBit, FirstBit+nBits) are zeros "0"
///////////////////////////////////////////////////////////
bool
AreBitsClear(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );

#if !defined UFSD_DRIVER_LINUX || !defined NDEBUG || defined UFSD_EXFAT || defined UFSD_FAT || defined UFSD_BTRFS || defined UFSD_APFS || defined UFSD_REFS3 \
   || ( !defined UFSD_NO_USE_IOCTL && ( defined UFSD_NTFS || defined UFSD_HFS ) )
///////////////////////////////////////////////////////////
// AreBitsSet
//
// Returns true if all bits [FirstBit, FirstBit+nBits) are ones "1"
///////////////////////////////////////////////////////////
bool
AreBitsSet(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );
#endif

///////////////////////////////////////////////////////////
// GetFreeBitPos
//
// This function scans bits [FirstBit, FirstBit+nBits)
// for first zero bit
//
// If there is free bit then function returns its positions relative FirstBit. ( 0,1,2,... nBits - 1)
// It returns nBits if no free bits
///////////////////////////////////////////////////////////
size_t
GetFreeBitPos(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );

///////////////////////////////////////////////////////////
// GetUsedBitPos
//
// This function returns the position of first nonzero bit
// from 0 until nBits-1 position relative FirstBit
// it returns nBits if no used bits
///////////////////////////////////////////////////////////
size_t
GetUsedBitPos(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );


///////////////////////////////////////////////////////////
// FindNextForwardRunSet ( == GetUsedBitPos )
//
// Returns the length and position of next "used" region
///////////////////////////////////////////////////////////
size_t
FindNextForwardRunSet(
    IN const unsigned char* Map,
    IN size_t               TotalBits,
    IN size_t               From,
    OUT size_t*             Bit
    );


///////////////////////////////////////////////////////////
// FindNextForwardRunSetBounded
//
//
///////////////////////////////////////////////////////////
unsigned
FindNextForwardRunSetBounded(
    IN const unsigned char* Map,
    IN unsigned int         TotalBits,
    IN unsigned int         From,
    IN unsigned int         Len,
    OUT unsigned int*       Bit
    );


///////////////////////////////////////////////////////////
// FindNextForwardRunSetBounded
//
//
///////////////////////////////////////////////////////////
unsigned
FindNextForwardRunSetBounded(
    IN const unsigned char* Map,
    IN unsigned int         TotalBits,
    IN unsigned int         From,
    IN unsigned int         Len,
    OUT unsigned int*       Bit
    );


///////////////////////////////////////////////////////////
// FindNextForwardRunClearCapped
//
//
///////////////////////////////////////////////////////////
unsigned
FindNextForwardRunClearCapped(
    IN const unsigned char* Map,
    IN unsigned int         TotalBits,
    IN unsigned int         From,
    IN unsigned int         Len,
    OUT unsigned int*       Bit
    );


///////////////////////////////////////////////////////////
// GetTailFree
//
// Returns the number of free bits in tail of map
// so the range [TotalBits-Ret, TotalBits) if free
///////////////////////////////////////////////////////////
size_t
GetTailFree(
    IN const unsigned char* Map,
    IN size_t               TotalBits
    );

///////////////////////////////////////////////////////////
// GetNumberOfSetBits
//
// This function returns the number of '1' bits in bitmap
// beginning from FirstBit bit until nBits-1 position relative FirstBit
// nBits  - the number of available bits after FirstBit
///////////////////////////////////////////////////////////
size_t
GetNumberOfSetBits(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );

///////////////////////////////////////////////////////////
// FindClearBits
//
// This procedure searches the specified bit map for the specified
// contiguous region of clear bits.
// If a run is not found from the hint to the end of the bitmap
// and 'bOnlyUp' is not set, we will search again from the beginning of the bitmap.
//
//Return Value:
//    The starting index (zero based) of the contiguous region of clear bits found.
//    If not such a region cannot be found a -1 (i.e. MINUS_ONE_T) is returned.
///////////////////////////////////////////////////////////
// size_t
// FindClearBits(
//     IN const unsigned char* Map,
//     IN size_t               TotalBits,
//     IN size_t               FirstBit,
//     IN size_t               BitsToFind,
//     IN bool                 bOnlyUp
//     );

///////////////////////////////////////////////////////////
// FindLongestClear
//
// This procedure finds the largest contiguous range of clear bits
// Returns the length of range
///////////////////////////////////////////////////////////
size_t
FindLongestClear(
    IN const unsigned char* Map,
    IN  size_t              TotalBits,
    OUT size_t*             Pos
    );

//
// Structure for function GetBitmapStatistic
//
struct BitmapStat{
  size_t        FreeFragments;      // The count of free fragments
  size_t        MaxFreeBits;        // The length of maximum free fragment
  size_t        MaxFreePos;         // The position of maximum free fragment
  size_t        TotalUsed;          // The count of used bits
  size_t        LastUsed;           // The last used bit (MINUS_ONE_T if none)
};

///////////////////////////////////////////////////////////
// GetBitmapStatistic
//
// This function analyzes bitmap and returns the statistic BitmapStat
///////////////////////////////////////////////////////////
void
GetBitmapStatistic(
    IN const unsigned char* Map,
    IN size_t               TotalBits,
    OUT BitmapStat*         Stat
    );


#if 0
///////////////////////////////////////////////////////////
// GetNotZeroNibbles
//
// Returns the count of non empty nibbles
///////////////////////////////////////////////////////////
static inline unsigned
GetNotZeroNibbles(
    IN unsigned x
    )
{
  return ((x | ((x & 0x77777777) + 0x77777777)) >> 3 & 0x11111111) * 0x11111111 >> 28;
}
#endif


///////////////////////////////////////////////////////////
// FindSetBits
//
// Searches for a range of set bits of a requested size within a bitmap.
// Either returns the zero-based starting bit index for a set bit range of the requested size,
// or it returns -1 if it cannot find such a range within the given bitmap variable.
///////////////////////////////////////////////////////////
size_t
FindSetBits(
    IN const unsigned char* Map,
    IN size_t               nBits,
    IN size_t               NumberToFind,
    IN size_t               HintIndex
    );

#endif // #ifdef UFSD_NEEDS_BITFUNC_LE

#ifdef UFSD_NEEDS_BITFUNC_BE
///////////////////////////////////////////////////////////
// SetBitsBe
//
// Set block of bits
///////////////////////////////////////////////////////////
void
SetBitsBe(
    IN OUT unsigned char* Map,
    IN size_t             FirstBit,
    IN size_t             nBits
    );

///////////////////////////////////////////////////////////
// ClearBitsBe
//
// Clear block of bits
///////////////////////////////////////////////////////////
void
ClearBitsBe(
    IN OUT unsigned char* Map,
    IN size_t             FirstBit,
    IN size_t             nBits
    );

#if !defined UFSD_DRIVER_LINUX || !defined NDEBUG || defined UFSD_HFS
///////////////////////////////////////////////////////////
// AreBitsClearBe
//
// Returns true if all bits [FirstBit, FirstBit+nBits) are zeros "0"
///////////////////////////////////////////////////////////
bool
AreBitsClearBe(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );
#endif

#if !defined UFSD_DRIVER_LINUX || !defined NDEBUG || defined UFSD_EXFAT || \
   ( !defined UFSD_NO_USE_IOCTL && ( defined UFSD_NTFS || defined UFSD_HFS ) )
///////////////////////////////////////////////////////////
// AreBitsSetBe
//
// Returns true if all bits [FirstBit, FirstBit+nBits) are ones "1"
///////////////////////////////////////////////////////////
bool
AreBitsSetBe(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );
#endif

///////////////////////////////////////////////////////////
// GetFreeBitPosBe
//
// This function scans bits [FirstBit, FirstBit+nBits)
// for first zero bit
//
// If there is free bit then function returns its positions relative FirstBit. ( 0,1,2,... nBits - 1)
// It returns nBits if no free bits
///////////////////////////////////////////////////////////
size_t
GetFreeBitPosBe(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );

///////////////////////////////////////////////////////////
// GetUsedBitPosBe
//
// This function returns the position of first nonzero bit
// from 0 until nBits-1 position relative FirstBit
// it returns nBits if no used bits
///////////////////////////////////////////////////////////
size_t
GetUsedBitPosBe(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );

///////////////////////////////////////////////////////////
// GetTailFreeBe
//
// Returns the number of free bits in tail of map
// so the range [TotalBits-Ret, TotalBits) if free
///////////////////////////////////////////////////////////
// size_t
// GetTailFreeBe(
//     IN const unsigned char* Map,
//     IN size_t               TotalBits
//     );

///////////////////////////////////////////////////////////
// GetNumberOfSetBitsBe
//
// This function returns the number of '1' bits in bitmap
// beginning from FirstBit bit until nBits-1 position relative FirstBit
// nBits  - the number of available bits after FirstBit
///////////////////////////////////////////////////////////
size_t
GetNumberOfSetBitsBe(
    IN const unsigned char* Map,
    IN size_t               FirstBit,
    IN size_t               nBits
    );

///////////////////////////////////////////////////////////
// FindClearBitsBe
//
// This procedure searches the specified bit map for the specified
// contiguous region of clear bits.
// If a run is not found from the hint to the end of the bitmap
// and 'bOnlyUp' is not set, we will search again from the beginning of the bitmap.
//
//Return Value:
//    The starting index (zero based) of the contiguous region of clear bits found.
//    If not such a region cannot be found a -1 (i.e. MINUS_ONE_T) is returned.
///////////////////////////////////////////////////////////
// size_t
// FindClearBitsBe(
//     IN const unsigned char* Map,
//     IN size_t               TotalBits,
//     IN size_t               FirstBit,
//     IN size_t               BitsToFind,
//     IN bool                 bOnlyUp
//     );
#endif // #ifdef UFSD_NEEDS_BITFUNC_BE

} //namespace UFSD {

//#endif //__BITFUNC_H__
