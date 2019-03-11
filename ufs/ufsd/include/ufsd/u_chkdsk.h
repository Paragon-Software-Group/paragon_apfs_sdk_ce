// <copyright file="u_chkdsk.h" company="Paragon Software Group">
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
// filesystem repairing.
//
////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
// NTFS specific information
//
///////////////////////////////////////////////////////////
struct CHKNTFS_INFO{

  size_t        UsedMftRecords;       // Total number of MFT records
  size_t        FilesWithEa;          // Total number of files with extended attributes (EA)
  size_t        BadFiles;             // Total number of nonreadable files
  unsigned char MajorVersion;
  unsigned char MinorVersion;
  unsigned char Dirty;
  unsigned char Res;                  // Just to align
};


///////////////////////////////////////////////////////////
// REFS specific information
//
///////////////////////////////////////////////////////////
struct CHKNTFS_REFS{

  unsigned char MajorVersion;
  unsigned char MinorVersion;

};


///////////////////////////////////////////////////////////
// Common structure filled by chkdsk utility
//
///////////////////////////////////////////////////////////

struct CHKDSK_INFO{

  unsigned int  ExitStatus;         // See UFSD_CHK_XXX
  unsigned int  Res;                // Just to align
  size_t        Fs;                 // Checked Fs. See FS_XXX
  size_t        TotalClusters;      // Total clusters on volume
  size_t        FreeClusters;       // Available(free) clusters on volume
  size_t        BytesPerCluster;    // Bytes per cluster
  size_t        TotalFiles;         // Total number of files on volume
  size_t        UserFiles;          // Total number of user files
  size_t        TotalDirs;          // Total number of directories on volume
  size_t        BadClusters;        // Total number of bad clusters on volume
  size_t        FragmentsPerBad;    // How many fragments per bad blocks

  UINT64        BytesPerUserData;
  UINT64        BytesPerDirs;
  UINT64        BytesPerMetaFiles;
  UINT64        BytesPerLogFile;    // Bytes per journal (hfs) or logfile (ntfs)

  union{
    CHKNTFS_INFO  Ntfs;             // NTFS specific information
    CHKNTFS_REFS  Refs;
  }info;

};


///////////////////////////////////////////////////////////
//
//    Additional options for checking
//
///////////////////////////////////////////////////////////
#define CHK_OPTIONS_SHOWMINOR     0x00000001
#define CHK_OPTIONS_NOORPHANS     0x00000002
#define CHK_OPTIONS_SAFE          0x00000004
#define CHK_OPTIONS_SHORT         0x00000008


///////////////////////////////////////////////////////////
//
//    Possible values returned in CHKDSK_INFO::ExitStatus
//
///////////////////////////////////////////////////////////

#define UFSD_CHK_SUCCESS          0 // No errors found
#define UFSD_CHK_FAILED           1 // Could not use CUFSD::CheckFs(). See return code for details.
#define UFSD_CHK_NOT_FIXED        2 // Errors are found but not fixed for some reason.
#define UFSD_CHK_FIXED            3 // CheckFs fixed errors
#define UFSD_CHK_MINOR            4 // CheckFs found minor errors
#define UFSD_CHK_BAD_BLOCKS       5 // CheckFs failed due to bad blocks
#define UFSD_CHK_NEED_REPLAY      6 // No errors in read-only but journal is not reaplayed
#define UFSD_CHK_UNSUPPORTED      7 // File system has unsupported features and can not be checked

