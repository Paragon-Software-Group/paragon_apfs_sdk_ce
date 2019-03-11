// <copyright file="apfsenum.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
// Revision History :
//
//     05-October-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __UFSD_APFSENUM_H
#define __UFSD_APFSENUM_H

namespace UFSD
{

namespace apfs
{

#define APFS_ENUM_ALL_VOLUMES       0x10000000    // Enumerate all subvolumes as dir in mount point

struct CApfsEntryNumerator : CUnixEnumerator
{
  bool            m_bVolumesDirEmulated;

  CApfsEntryNumerator(api::IBaseMemoryManager* Mm)
    : CUnixEnumerator(Mm)
    , m_bVolumesDirEmulated(false)
  {}

  virtual int Init(
    IN CUnixFileSystem*   Fs,
    IN size_t             Options,
    IN CUnixInode*        pInode = NULL,
    IN api::STRType       Type   = api::StrUTF8,
    IN const void*        Mask   = NULL,
    IN size_t             Len    = 0
  );

  virtual void SetPosition(
    IN const UINT64& Position
  );

  virtual unsigned int GetNameHash(
    IN const unsigned char* Name,
    IN size_t               NameLen
  ) const;

  virtual int FindEntry(
    OUT FileInfo&       Info,
    IN  CEntryTreeEnum* pTreeEnum
  );

  //Emulate FileInfo for subvolumes mount point directory (APFS_VOLUMES_DIR_NAME)
  void EmulateVolumesDirEntry(
    OUT FileInfo&     Info
  ) const;

  virtual int SetNewEntry(
    IN UINT64         Id,
    IN const char*    Name,
    IN unsigned char  NameLen,
    IN unsigned char  FileType
    );
};


//Enumerator for volumes dir (emulated directory where all volumes are shown as directories)
struct CApfsVolumeNumerator : CApfsEntryNumerator
{
  CApfsVolumeNumerator(api::IBaseMemoryManager* Mm)
    : CApfsEntryNumerator(Mm)
  {}

  virtual ~CApfsVolumeNumerator() {}

  virtual int Init(
    IN CUnixFileSystem*   Fs,
    IN size_t             Options,
    IN CUnixInode*        pInode = NULL,
    IN api::STRType       Type   = api::StrUTF8,
    IN const void*        Mask   = NULL,
    IN size_t             Len    = 0
    )
  {
    return CUnixEnumerator::Init(Fs, Options, pInode, Type, Mask, Len);
  }

  virtual int FindEntry(
    OUT FileInfo&       Info,
    IN  CEntryTreeEnum* pTreeEnum
  );

  virtual int SetNewEntry(
    IN UINT64         /*Id*/,
    IN const char*    /*Name*/,
    IN unsigned char  /*NameLen*/,
    IN unsigned char  /*FileType*/
    )
  {
    return ERR_NOTIMPLEMENTED;
  }
};

} // namespace apfs

} // namespace UFSD

#endif
