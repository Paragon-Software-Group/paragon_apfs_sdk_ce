// <copyright file="fsattribs.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __FSATTRIBS_H__
#define __FSATTRIBS_H__

#pragma once


//UFSD supported file systems
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
#define UFSD_LINK         0x00000400  //  File is link (Reparse point)
#define UFSD_COMPRESSED   0x00000800  //  NTFS Compressed file
#define UFSD_ENCRYPTED    0x00004000  //  NTFS Encrypted file
#define UFSD_INTEGRITY    0x00008000  //  REFS integrity file
#define UFSD_DEDUPLICATED 0x00010000  //  NTFS/REFS file is deduplicated
#define UFSD_ATTRIB_MASK  0x0001cfff

#endif //__FSATTRIBS_H__
