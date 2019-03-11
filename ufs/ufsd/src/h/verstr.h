// <copyright file="verstr.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
#ifndef UFSD_NEW_LINE
  #define UFSD_NEW_LINE "\n"
#endif

#if defined UFSD_NTFS
  #ifdef UFSD_NTFS_64BIT_CLUSTER
    #define UFSD_NTFS_64_BIT_STR ", 64c"
  #else
    #define UFSD_NTFS_64_BIT_STR
  #endif

  #if defined UFSD_NTFS_JNL
    #define UFSD_NTFS_STR "NTFSJ support included" UFSD_NTFS_64_BIT_STR UFSD_NEW_LINE
  #else
    #define UFSD_NTFS_STR "NTFS support included" UFSD_NTFS_64_BIT_STR UFSD_NEW_LINE
  #endif

  #ifdef UFSD_PERSIST_UGM_NTFS
    #define UFSD_NTFS_UGM_STR "NTFS $UGM included" UFSD_NEW_LINE
  #else
    #define UFSD_NTFS_UGM_STR
  #endif
#else
  #define UFSD_NTFS_STR
  #define UFSD_NTFS_UGM_STR
#endif

#if defined UFSD_NTFS_FORMAT
  #define UFSD_NTFS_FORMAT_STR "Format NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_FORMAT_STR
#endif

#if defined UFSD_NTFS_CHECK
  #define UFSD_NTFS_CHECK_STR "Check NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_CHECK_STR
#endif

#if defined UFSD_NTFS_EXTEND
  #define UFSD_NTFS_EXTEND_STR "Extend NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_EXTEND_STR
#endif

#if defined UFSD_NTFS_SHRINK
  #define UFSD_NTFS_SHRINK_STR "Shrink NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_SHRINK_STR
#endif

#if defined UFSD_NTFS_DEFRAG
  #define UFSD_NTFS_DEFRAG_STR  "Defrag NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_DEFRAG_STR
#endif

#if defined UFSD_NTFS_DEFRAG2
  #define UFSD_NTFS_DEFRAG2_STR "Safe Defrag NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_DEFRAG2_STR
#endif

#ifdef UFSD_NTFS_DEFRAG_MFT
  #define UFSD_NTFS_DEFRAG_MFT_STR  "Defrag MFT included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_DEFRAG_MFT_STR
#endif

#ifdef UFSD_NTFS_COMPACT_MFT
  #define UFSD_NTFS_COMPACT_MFT_STR "Compact MFT included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_COMPACT_MFT_STR
#endif

#ifdef UFSD_NTFS_WIPE
  #define UFSD_NTFS_WIPE_STR  "Wipe NTFS included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_WIPE_STR
#endif

#ifdef UFSD_NTFS_REMOVE_SHORT_NAMES
  #define UFSD_NTFS_REMOVE_SHORT_NAMES_STR  "Remove NTFS's short name included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_REMOVE_SHORT_NAMES_STR
#endif

#ifdef UFSD_EXT2
  #define UFSD_EXT2_STR "Ext2 support included" UFSD_NEW_LINE
#else
  #define UFSD_EXT2_STR
#endif

#ifdef UFSD_EXT
  #define UFSD_EXT_STR "Extfs support included" UFSD_NEW_LINE
#else
  #define UFSD_EXT_STR
#endif

#if defined UFSD_EXT_FORMAT | defined UFSD_EXT2_FORMAT
  #define UFSD_EXT_FORMAT_STR "Format Extfs included" UFSD_NEW_LINE
#else
  #define UFSD_EXT_FORMAT_STR
#endif

#ifdef UFSD_EXT_CHECK
  #define UFSD_EXT_CHECK_STR  "Check Extfs included" UFSD_NEW_LINE
#else
  #define UFSD_EXT_CHECK_STR
#endif

#ifdef UFSD_EXT_SHRINK
  #define UFSD_EXT_SHRINK_STR "Shrink Extfs included" UFSD_NEW_LINE
#else
  #define UFSD_EXT_SHRINK_STR
#endif

#ifdef UFSD_EXT_EXTEND
  #define UFSD_EXT_EXTEND_STR "Extend Extfs included" UFSD_NEW_LINE
#else
  #define UFSD_EXT_EXTEND_STR
#endif

#ifdef UFSD_ISO
  #define UFSD_ISO_STR  "Iso-9660 support included" UFSD_NEW_LINE
#else
  #define UFSD_ISO_STR
#endif

#ifdef UFSD_REG
  #define UFSD_REG_STR  "Windows2K+ registry support included" UFSD_NEW_LINE
#else
  #define UFSD_REG_STR
#endif

#ifdef UFSD_FAT
  #define UFSD_FAT_STR  "FAT support included" UFSD_NEW_LINE
#else
  #define UFSD_FAT_STR
#endif

#ifdef UFSD_BTRFS
  #define UFSD_BTRFS_STR  "BTRFS support included" UFSD_NEW_LINE
#else
  #define UFSD_BTRFS_STR
#endif

#ifdef UFSD_APFS
  #define UFSD_APFS_STR  "APFS support included" UFSD_NEW_LINE
#else
  #define UFSD_APFS_STR
#endif

#ifdef UFSD_FAT_EXTEND
  #define UFSD_FAT_EXTEND_STR "Extend FAT included" UFSD_NEW_LINE
#else
  #define UFSD_FAT_EXTEND_STR
#endif

#ifdef UFSD_FAT_FORMAT
  #define UFSD_FAT_FORMAT_STR "Format FAT included" UFSD_NEW_LINE
#else
  #define UFSD_FAT_FORMAT_STR
#endif

#ifdef UFSD_FAT_DEFRAG
  #define UFSD_FAT_DEFRAG_STR "Defrag FAT included" UFSD_NEW_LINE
#else
  #define UFSD_FAT_DEFRAG_STR
#endif

#ifdef UFSD_EXFAT
  #define UFSD_EXFAT_STR "exFAT support included" UFSD_NEW_LINE
#else
  #define UFSD_EXFAT_STR
#endif

#ifdef UFSD_EXFAT_FORMAT
  #define UFSD_EXFAT_FORMAT_STR "Format exFAT/TexFAT included" UFSD_NEW_LINE
#else
  #define UFSD_EXFAT_FORMAT_STR
#endif

#ifdef UFSD_EXFAT_DEFRAG
  #define UFSD_EXFAT_DEFRAG_STR "Defrag exFAT included" UFSD_NEW_LINE
#else
  #define UFSD_EXFAT_DEFRAG_STR
#endif

#ifdef UFSD_EXFAT_CHECK
  #define UFSD_EXFAT_CHECK_STR  "Check exFAT/TexFAT included" UFSD_NEW_LINE
#else
  #define UFSD_EXFAT_CHECK_STR
#endif

#ifdef BASE_BIGENDIAN
  #define BASE_BIGENDIAN_STR  "Big endian platform" UFSD_NEW_LINE
#else
  #define BASE_BIGENDIAN_STR
#endif

#ifdef UFSD_ZIP
  #define UFSD_ZIP_STR  "Zip archive file support included" UFSD_NEW_LINE
#else
  #define UFSD_ZIP_STR
#endif

#ifdef UFSD_HFS

  #if defined UFSD_FAKE_HFS_JOURNAL
    #define UFSD_HFS_STR "Hfs+ support included" UFSD_NEW_LINE
  #else
    #define UFSD_HFS_STR "Hfs+J support included" UFSD_NEW_LINE
  #endif

#else
  #define UFSD_HFS_STR
#endif

#ifdef UFSD_HFS_FORMAT
  #define UFSD_HFS_FORMAT_STR "Format Hfs+ included" UFSD_NEW_LINE
#else
  #define UFSD_HFS_FORMAT_STR
#endif

#ifdef UFSD_HFS_DEFRAG
  #define UFSD_HFS_DEFRAG_STR "Defrag Hfs+ included" UFSD_NEW_LINE
#else
  #define UFSD_HFS_DEFRAG_STR
#endif

#ifdef UFSD_HFS_SHRINK
  #define UFSD_HFS_SHRINK_STR "Shrink Hfs+ included" UFSD_NEW_LINE
#else
  #define UFSD_HFS_SHRINK_STR
#endif

#ifdef UFSD_HFS_EXTEND
  #define UFSD_HFS_EXTEND_STR "Extend Hfs+ included" UFSD_NEW_LINE
#else
  #define UFSD_HFS_EXTEND_STR
#endif

#ifdef UFSD_HFS_CHECK
  #define UFSD_HFS_CHECK_STR "Check Hfs+ included" UFSD_NEW_LINE
#else
  #define UFSD_HFS_CHECK_STR
#endif

#ifdef UFSD_REFS
  #define UFSD_REFS_STR "ReFs 1.x support included" UFSD_NEW_LINE
#else
  #define UFSD_REFS_STR
#endif

#ifdef UFSD_REFS_FORMAT
  #define UFSD_REFS_FORMAT_STR "Format ReFs 1.x included" UFSD_NEW_LINE
#else
  #define UFSD_REFS_FORMAT_STR
#endif

#ifdef UFSD_REFS_CHECK
  #define UFSD_REFS_CHECK_STR "Check ReFs 1.x included" UFSD_NEW_LINE
#else
  #define UFSD_REFS_CHECK_STR
#endif

#ifdef UFSD_REFS3
  #define UFSD_REFS3_STR "ReFs 3.x support included" UFSD_NEW_LINE
#else
  #define UFSD_REFS3_STR
#endif

#ifdef UFSD_REFS3_FORMAT
  #define UFSD_REFS3_FORMAT_STR "Format ReFs 3.x included" UFSD_NEW_LINE
#else
  #define UFSD_REFS3_FORMAT_STR
#endif

#ifdef UFSD_REFS3_CHECK
  #define UFSD_REFS3_CHECK_STR "Check ReFs 3.x included" UFSD_NEW_LINE
#else
  #define UFSD_REFS3_CHECK_STR
#endif



#if defined UFSD_NO_CRT_USE || !defined BASE_USE_BUILTIN_MEMCPY
  #define UFSD_MEM_STR  "no crt"  UFSD_NEW_LINE
#elif defined BASE_USE_BUILTIN_MEMMOVE
  #define UFSD_MEM_STR  "builtin memcpy and memmove"  UFSD_NEW_LINE
#else
  #define UFSD_MEM_STR  "builtin memcpy"  UFSD_NEW_LINE
#endif

#ifdef UFSD_NTFS_GET_VOLUME_META_BITMAP
  #define UFSD_NTFS_VOLUME_META_BITMAP_STR  "Ntfs's fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_NTFS_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_HFS_GET_VOLUME_META_BITMAP
  #define UFSD_HFS_VOLUME_META_BITMAP_STR  "Hfs's fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_HFS_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_EXFAT_GET_VOLUME_META_BITMAP
  #define UFSD_EXFAT_VOLUME_META_BITMAP_STR  "Exfat's fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_EXFAT_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_EXTFS_GET_VOLUME_META_BITMAP
  #define UFSD_EXTFS_VOLUME_META_BITMAP_STR  "Ext's fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_EXTFS_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_REFS_GET_VOLUME_META_BITMAP
  #define UFSD_REFS_VOLUME_META_BITMAP_STR  "Refs(1)'s fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_REFS_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_REFS3_GET_VOLUME_META_BITMAP
  #define UFSD_REFS3_VOLUME_META_BITMAP_STR  "Refs(3)'s fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_REFS3_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_FAT_GET_VOLUME_META_BITMAP
  #define UFSD_FAT_VOLUME_META_BITMAP_STR  "Fat's fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_FAT_VOLUME_META_BITMAP_STR
#endif

#ifdef UFSD_APFS_GET_VOLUME_META_BITMAP
  #define UFSD_APFS_VOLUME_META_BITMAP_STR  "Apfs's fsdump included" UFSD_NEW_LINE
#else
  #define UFSD_APFS_VOLUME_META_BITMAP_STR
#endif



#define UFSD_FEATURES_STR \
    UFSD_NTFS_STR \
    UFSD_NTFS_UGM_STR \
    UFSD_NTFS_FORMAT_STR \
    UFSD_NTFS_CHECK_STR \
    UFSD_NTFS_EXTEND_STR \
    UFSD_NTFS_SHRINK_STR \
    UFSD_NTFS_DEFRAG_STR \
    UFSD_NTFS_DEFRAG2_STR \
    UFSD_NTFS_DEFRAG_MFT_STR \
    UFSD_NTFS_COMPACT_MFT_STR \
    UFSD_NTFS_WIPE_STR \
    UFSD_NTFS_REMOVE_SHORT_NAMES_STR \
    UFSD_NTFS_VOLUME_META_BITMAP_STR  \
    UFSD_EXT2_STR \
    UFSD_EXT_STR \
    UFSD_EXT_FORMAT_STR \
    UFSD_EXT_CHECK_STR \
    UFSD_EXT_SHRINK_STR \
    UFSD_EXT_EXTEND_STR \
    UFSD_EXTFS_VOLUME_META_BITMAP_STR \
    UFSD_ISO_STR  \
    UFSD_REG_STR  \
    UFSD_FAT_STR  \
    UFSD_FAT_EXTEND_STR \
    UFSD_FAT_FORMAT_STR \
    UFSD_FAT_DEFRAG_STR \
    UFSD_FAT_VOLUME_META_BITMAP_STR \
    UFSD_BTRFS_STR \
    UFSD_APFS_STR \
    UFSD_APFS_VOLUME_META_BITMAP_STR  \
    UFSD_EXFAT_STR \
    UFSD_EXFAT_FORMAT_STR \
    UFSD_EXFAT_DEFRAG_STR \
    UFSD_EXFAT_CHECK_STR  \
    UFSD_EXFAT_VOLUME_META_BITMAP_STR \
    UFSD_ZIP_STR \
    UFSD_HFS_STR \
    UFSD_HFS_FORMAT_STR \
    UFSD_HFS_DEFRAG_STR \
    UFSD_HFS_SHRINK_STR \
    UFSD_HFS_EXTEND_STR \
    UFSD_HFS_CHECK_STR  \
    UFSD_HFS_VOLUME_META_BITMAP_STR \
    UFSD_REFS_STR       \
    UFSD_REFS_FORMAT_STR  \
    UFSD_REFS_CHECK_STR  \
    UFSD_REFS_VOLUME_META_BITMAP_STR  \
    UFSD_REFS3_STR      \
    UFSD_REFS3_FORMAT_STR  \
    UFSD_REFS3_CHECK_STR  \
    UFSD_REFS3_VOLUME_META_BITMAP_STR  \
    BASE_BIGENDIAN_STR
