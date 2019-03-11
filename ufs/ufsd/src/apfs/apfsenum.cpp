// <copyright file="apfsenum.cpp" company="Paragon Software Group">
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


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331632 $";
#endif

#include <ufsd.h>

#include "../h/assert.h"
#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixenum.h"
#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfsvolsb.h"
#include "apfssuper.h"
#include "apfsinode.h"
#include "apfshash.h"
#include "apfstable.h"
#include "apfsbplustree.h"
#include "apfsenum.h"

namespace UFSD
{

namespace apfs
{


/////////////////////////////////////////////////////////////////////////////
unsigned int CApfsEntryNumerator::GetNameHash(
  IN const unsigned char* Name,
  IN size_t               NameLen
  ) const
{
  unsigned int Hash = ~0u;
  int Status = NormalizeAndCalcHash(Name, NameLen, m_bCaseSensitive, &Hash);

  if (!UFSD_SUCCESS(Status))
  {
    ULOG_WARNING((GetLog(), "Failed to calc hash -> %x", Status));
    assert(0);
    return 0;
  }

  return Hash & 0x3FFFFF;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsEntryNumerator::Init(
  IN CUnixFileSystem*    Fs,
  IN size_t              Options,
  IN CUnixInode*         pInode,
  IN api::STRType        Type,
  IN const void*         Mask,
  IN size_t              Len
  )
{
  CHECK_CALL_SILENT(CUnixEnumerator::Init(Fs, Options, pInode, Type, Mask, Len));
  m_bVolumesDirEmulated = false;

  int Status = ERR_NOERROR;
  CApfsInode* pApfsInode = reinterpret_cast<CApfsInode*>(pInode);
  CApfsSuperBlock* pSuper = reinterpret_cast<CApfsSuperBlock*>(m_pSuper);
  CApfsTree* pRootTree = pApfsInode->GetTree();

  if (!pRootTree)
    return ERR_NOERROR;

  if (m_pInode)
    m_bCaseSensitive = pSuper->GetVolume(APFS_GET_TREE_ID(m_pInode->Id()))->IsCaseSensitive();

  if (!m_bMatchAll)
  {
    ULOG_DEBUG1((GetLog(), "Hash = %#x", m_Hash));
    CEntrySearchKey SearchKey(m_Mm, pApfsInode->GetInodeId(), m_bCaseSensitive, m_Hash );
    Status = pRootTree->GetDefaultEnum()->StartEnumByKey(&SearchKey, SEARCH_KEY_EQ);
  }

#ifdef UFSD_DRIVER_LINUX
  if (!UFSD_SUCCESS(Status))
    Status = ERR_NOFILEEXISTS;
#endif

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
void CApfsEntryNumerator::SetPosition(
  IN const UINT64& Position
  )
{
  if (!m_pInode->m_bHaveChanges)
    m_Position2 = Position;
  CUnixEnumerator::SetPosition(Position);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsEntryNumerator::FindEntry(
  OUT FileInfo&       Info,
  IN  CEntryTreeEnum* pTreeEnum
  )
{
  int Status = ERR_NOERROR;
  apfs_direntry_data* de;

  unsigned char TreeId = reinterpret_cast<CApfsInode*>(m_pInode)->GetTreeId();
  CApfsSuperBlock* pSuper = reinterpret_cast<CApfsSuperBlock*>(m_pSuper);

  if (m_bMatchAll && m_bRestarted)
  {
    m_pInode->m_bHaveChanges = false;
    m_bRestarted = false;
  }

  if ((FlagOn(m_Options, APFS_ENUM_ALL_VOLUMES) && TreeId == 0 && pSuper->GetMountedVolumesCount() > 1 && m_pInode->Id() == APFS_ROOT_INO && !m_bVolumesDirEmulated) &&
    (m_bMatchAll || (m_Len == APFS_VOLUMES_DIR_NAME_LEN && Memcmp2(m_Mask, APFS_VOLUMES_DIR_NAME, m_Len) == 0)))
  {
      EmulateVolumesDirEntry(Info);
      m_bVolumesDirEmulated = true;
      return ERR_FILEEXISTS;
  }

  CApfsTree* pRootTree = pSuper->GetObjectTree(TreeId);
  if (!pRootTree || !pSuper->GetVolume(TreeId)->CanDecrypt())
    return ERR_NOFILEEXISTS;

  while (UFSD_SUCCESS(Status))
  {
    apfs_direntry_key* entry = NULL;

    if (m_bMatchAll)
      Status = pTreeEnum->EnumNextEntry(&entry, &de);
    else
      Status = pRootTree->GetDefaultEnum()->EnumNextPtrByKey((void**)&entry, (void**)&de);

    if (Status == ERR_NOTFOUND || !entry)
      return ERR_NOFILEEXISTS;
    CHECK_STATUS(Status);

#ifdef BASE_BIGENDIAN
    // pointer to the hash bitfield
    unsigned int* x = (unsigned int*)Add2Ptr(&entry->id, sizeof(entry->id));
    SwapBytesInPlace(x);
#endif

    if (!m_bMatchAll)
    {
      if (entry->name_hash > m_Hash)
        return ERR_NOFILEEXISTS;
      assert(entry->name_hash == m_Hash);
      if (!IsNamesEqual(entry->name, entry->name_len - 1, (unsigned char*)m_Mask, m_Len, m_bCaseSensitive))
        continue;
    }

#ifndef UFSD_DRIVER_LINUX
    Info.NameType = api::StrUTF8;
    Info.NameLen = static_cast<unsigned short>(entry->name_len) - 1;
    Memcpy2(Info.Name, entry->name, entry->name_len - 1);

    switch (de->type)
    {
    case ApfsSubdir:
      Info.Attrib = UFSD_SUBDIR;
      break;
    case ApfsFile:
      Info.Attrib = UFSD_NORMAL;
      break;
    case ApfsSymlink:
      Info.Attrib = UFSD_LINK;
      break;
    case ApfsSocket:
    case ApfsCharDev:
    case ApfsBlockDev:
    case ApfsFifo:
      Info.Attrib = UFSD_SYSTEM;
      break;
    default:
      assert(!"Unknown entry type");
      break;
    }
#else
    size_t DstLen;
    verify( UFSD_SUCCESS( m_Strings->Convert(api::StrUTF8, entry->name, entry->name_len - 1,
                                             api::StrUTF16, Info.Name, MAX_FILE_NAME, &DstLen)));
    Info.NameLen = DstLen;
#endif
    Info.Id = APFS_MAKE_INODE_ID(TreeId, CPU2LE(de->id));
    Status = ERR_FILEEXISTS;

    ULOG_DEBUG1((GetLog(), "Found: '%s' (%x), r=%" PLL "x, hash=%#08x", entry->name, entry->name_len, Info.Id, entry->name_hash));

    m_Position = m_Position2++;
    BE_ONLY(SwapBytesInPlace(x));
  }

  return UFSD_SUCCESS(Status) ? ERR_NOFILEEXISTS : Status;
}


#ifdef UFSD_APFS_RO
/////////////////////////////////////////////////////////////////////////////
int CApfsEntryNumerator::SetNewEntry(
  IN UINT64         /*Id*/,
  IN const char*    /*Name*/,
  IN unsigned char  /*NameLen*/,
  IN unsigned char  /*FileType*/
  )
{
  return ERR_WPROTECT;
}
#endif


/////////////////////////////////////////////////////////////////////////////
void CApfsEntryNumerator::EmulateVolumesDirEntry(
  OUT FileInfo& Info
  ) const
{
#ifndef UFSD_DRIVER_LINUX
  Info.NameType = api::StrUTF8;
  Info.NameLen = APFS_VOLUMES_DIR_NAME_LEN;
  Memcpy2(Info.Name, APFS_VOLUMES_DIR_NAME, Info.NameLen);
#else
  size_t DstLen;
  verify(UFSD_SUCCESS(m_Strings->Convert(api::StrUTF8, APFS_VOLUMES_DIR_NAME, APFS_VOLUMES_DIR_NAME_LEN, api::StrUTF16, Info.Name, MAX_FILE_NAME, &DstLen)));
  Info.NameLen = static_cast<unsigned short>(DstLen);
#endif
  Info.Id = APFS_VOLUMES_DIR_ID;
  Info.Attrib = UFSD_SUBDIR | UFSD_RDONLY | UFSD_SYSTEM | UFSD_VOLID | UFSD_VSIZE;
  Info.CrTime = Info.ChangeTime = Info.ModiffTime = Info.ReffTime = m_pSuper->m_Time->Time();
  Info.HardLinks = 1;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsVolumeNumerator::FindEntry(
  OUT FileInfo&       Info,
  IN  CEntryTreeEnum* /*pTreeEnum*/
  )
{
  CApfsSuperBlock* pSuper = reinterpret_cast<CApfsSuperBlock*>(m_pSuper);
  unsigned char VolumesCount = pSuper->GetMountedVolumesCount();
  CApfsVolumeSb* CurVolSb;
  char* VolName = NULL;
  bool bFound = false;

  if (m_bMatchAll)
  {
    //Get next volume superblock from array and extract base info from it
    if (m_Position + 1 < VolumesCount)
    {
      CurVolSb = pSuper->GetVolume(static_cast<unsigned char>(++m_Position));
      VolName = CurVolSb->GetName();
      bFound = true;
    }
  }
  else
  {
    //Find volume superblock with specified name
    for (unsigned char i = 1; i < VolumesCount; i++)
    {
      CurVolSb = pSuper->GetVolume(i);
      VolName = CurVolSb->GetName();
      if (m_Strings->strlen(api::StrUTF8, VolName) == m_Len && Memcmp2(VolName, m_Mask, m_Len) == 0)
      {
        m_Position = i;
        bFound = true;
      }
    }
  }

  if (bFound)
  {
    assert(VolName != NULL);
#ifndef UFSD_DRIVER_LINUX
    Info.NameType = api::StrUTF8;
    Info.NameLen = static_cast<unsigned short>(m_Strings->strlen(api::StrUTF8, VolName));
    Memcpy2(Info.Name, VolName, Info.NameLen);
#else
    size_t DstLen;
    verify(UFSD_SUCCESS(m_Strings->Convert(api::StrUTF8, VolName, m_Strings->strlen(api::StrUTF8, VolName),
                                           api::StrUTF16, Info.Name, MAX_FILE_NAME, &DstLen)));
    Info.NameLen = DstLen;
#endif

    Info.Id = m_Position != 0 ? APFS_MAKE_INODE_ID(m_Position, APFS_VOLUMES_DIR_ID) : m_Position;
    Info.Attrib = UFSD_SUBDIR;

    return ERR_FILEEXISTS;
  }

  return ERR_NOFILEEXISTS;
}


} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
