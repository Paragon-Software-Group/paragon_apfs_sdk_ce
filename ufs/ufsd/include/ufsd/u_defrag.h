// <copyright file="u_defrag.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains declarations and definitions to perform
// defragmentations.
//
// NOTE: UFSD_SDK should be compiled with one of the following define
// - UFSD_NTFS_DEFRAG
// - UFSD_FAT_DEFRAG
////////////////////////////////////////////////////////////////

namespace UFSD {

#ifndef NO_PRAGMA_PACK
#pragma pack( push, 1 )
#endif

//
// This struct is used to present fragmented file/directory in report (UFSD_DEFRAG_REPORT)
//
struct UFSD_FILE_FRAGMENTS
{
  unsigned int    BytesPerStruct;       // The size of struct in bytes. 0 Means the last
  unsigned int    FragmentsPerFile;     // The count of fragments per file
  UINT64          BytesPerFile;         // The size of file in bytes
  unsigned short  wFileNameLen;
  // Here is placed wFileNameLen + 1 bytes of ASCII zero terminated string
};//  __attribute__ ((packed));


//
// This struct is used to show report
// IMsg->SyncRequest( m_pFsDefrag->CodeForReport, const UFSD_DEFRAG_REPORT*)
//
struct UFSD_DEFRAG_REPORT
{
  // Volume
  UINT64          BytesPerVolume;           // Size of volume in bytes
  UINT64          BytesPerUsedSpace;        // Size of used space in bytes
  UINT64          BytesPerBiggestFreeBlock; // Size of biggest free block in bytes
  UINT64          BytesPerBadSpace;         // Size of bad blocks in bytes
  unsigned int    FragmentsPerFreeSpace;    // Free space fragments
  unsigned short  Label[34];                // Unicode label

  unsigned int    BytesPerCluster;          // Cluster size in bytes
  unsigned int    FragmentsPerBad;          // Bad clusters fragments

  // Files
  unsigned int    TotFiles;                 // Total files number
  unsigned int    FragmentedFiles;          // Total fragmented files number

  // Folders
  unsigned int    TotFolders;               // Total folders number
  unsigned int    FragmentedFolders;        // Fragmented folders

  // $MFT
  UINT64          BytesPerMFT;              // $MFT size in bytes
  unsigned int    UsedMFT;                  // Used $MFT records
  unsigned int    FragmentsPerMFT;          // Fragments of $MFT

  // Time
  unsigned int    Duration;                 // Duration of operation. 0 if Volume is not modified

  // Map
  unsigned int    Mapped;                   // Number of mapped clusters
  unsigned int    Remapped;                 // Number of remapped clusters

  unsigned int    Reserved;

  //UFSD_FILE_FRAGMENTS  FileFragments;     // List of UFSD_FILE_FRAGMENTS
}; // __attribute__ ((packed));

//C_ASSERT( 0 == (sizeof(UFSD_DEFRAG_REPORT)%8) );


//
// This struct is used to ask user if bad blocks prevent full defragment
// IMsg->SyncRequest( UFSD_FS_DEFRAG::CodeForBadBlck, const UFSD_DEFRAG_BADBLOCKS* )
//
struct UFSD_DEFRAG_BADBLOCKS
{
//  unsigned int    FragNum;   This member is removed 'cause it is too hard to get the correct value
  unsigned short  wFileNameLen;
  unsigned char   bRes;
  unsigned char   bDir;
  // Here is placed wFileNameLen + 1 bytes of ASCII zero string
};//__attribute__ ((packed));

//C_ASSERT( 0 == (sizeof(UFSD_DEFRAG_BADBLOCKS)%8) );

//
// This struct is used to inform client about
// each read/write/move defragment operation
// IMsg->SyncRequest( UFSD_FS_DEFRAG::CodeForMove, const UFSD_DEFRAG_MOVE* )
//
struct UFSD_DEFRAG_MOVE
{
  unsigned int    OldLcn;   // Original Lcn
  unsigned int    NewLcn;   // New Lcn
  unsigned int    Count;    // The length in clusters
  unsigned int    Oper;     // See DEFRAG_MOVE_OPER_XXX
  // Pointer to internal bitmap of clusters
  // This buffer is at least ((BytesPerVolume/BytesPerCluster) + 7)/8 bytes
  const unsigned char* ClustersBitmap;
};//__attribute__ ((packed));

//C_ASSERT( 0 == (sizeof(UFSD_DEFRAG_MOVE)%4) );


#define DEFRAG_MOVE_OPER_READ     1    // Read from {OldLcn,Count}
#define DEFRAG_MOVE_OPER_WRITE    2    // Write to {NewLcn,Count}
#define DEFRAG_MOVE_OPER_MOVE     3    // Move {OldLcn,Count} to {NewLcn,Count}


//
// m_pFsDefrag->CodeForCancel is used to inform client when it can cancel operation
// IMsg->SyncRequest( m_pFsDefrag->CodeForCancel, bool )
// Return code is ignored
//


//
// Input structure for defragment operation
//
struct UFSD_FS_DEFRAG{

  size_t            BytesPerStruct;     // Size of this structure in bytes
  size_t            Options;            // Options of defragment, see DEFRAG_OPTIONS_XXX
  size_t            Order;              // Full defragment order type, see DEFRAG_ORDER_XXX
  size_t            MaxFilesInReport;   // Maximum number of FileFragments in UFSD_DEFRAG_RESULT

  size_t            CodeForReport;      // Code which will be used in IMsg->SyncRequest(CodeForReport, const UFSD_DEFRAG_REPORT*) to show report
                                        // 0 means ignore request
  size_t            CodeForBadBlck;     // Code which will be used in IMsg->SyncRequest(CodeForBadBlck, const UFSD_UFSD_DEFRAG_BADBLOCKS*) if bad blocks will prevent full defragment
                                        // 0 means ignore request
  size_t            CodeForMove;        // Code which will be used in IMsg->SyncRequest(CodeForMove, const UFSD_DEFRAG_MOVE*) for each read/write/move defragment operation
                                        // 0 means ignore request
  size_t            CodeForCancel;      // Code which will be used in IMsg->SyncRequest(CodeForCancel, bool )
                                        // 0 means ignore request

};//__attribute__ ((packed));

// Defragment options possible bits
#define DEFRAG_OPTIONS_PAGEFILE           0x00000001    // Skip pagefile.sys/hiberfil.sys/swapfile.sys files
#define DEFRAG_OPTIONS_RESTART            0x80000000    // Safe defrag( "restart" included )
#define DEFRAG_OPTIONS_SHOW_NAME          0x40000000    // Show names while defragging
#define DEFRAG_OPTIONS_FAST               0x20000000    // Fast but not safe algorithm
#define DEFRAG_OPTIONS_SKIP_POST_ANALYZE  0x10000000    // Do not perform analyze after defragment


// Defragment order possible bits
#define DEFRAG_ORDER_TIME       0x00000001    // Creation time increasing
#define DEFRAG_ORDER_TIME_INV   0x00000002    // Creation time decreasing
#define DEFRAG_ORDER_SIZE       0x00000004    // Big,Middle,Little file size order
#define DEFRAG_ORDER_SIZE_INV   0x00000008    // Little,Middle,Big file size order
#define DEFRAG_ORDER_DIR        0x00000010    // Directory is always first
#define DEFRAG_ORDER_DIR_INV    0x00000020    // Directory is always last
#define DEFRAG_ORDER_START      0x00000040    // Increasing start cluster
#define DEFRAG_ORDER_START_INV  0x00000080    // Decreasing start cluster
#define DEFRAG_ORDER_ENTRY      0x00000100    // Increasing entry cluster
#define DEFRAG_ORDER_ENTRY_INV  0x00000200    // Decreasing entry cluster


#ifndef NO_PRAGMA_PACK
#pragma pack( pop )
#endif

} // namespace UFSD {

