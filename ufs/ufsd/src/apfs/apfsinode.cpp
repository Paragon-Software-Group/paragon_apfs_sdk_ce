// <copyright file="apfsinode.cpp" company="Paragon Software Group">
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

#include "../h/ucommon.h"
#include "../h/utrace.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixinode.h"
#include "../unixfs/unixblock.h"
#include "../unixfs/unixsuperblock.h"

#include "apfs_struct.h"
#include "apfsvolsb.h"
#include "apfscompr.h"
#include "apfssuper.h"
#include "apfsinode.h"
#include "apfstable.h"
#include "apfsbplustree.h"


namespace UFSD
{

namespace apfs
{


/////////////////////////////////////////////////////////////////////////////
CApfsInode::CApfsInode(api::IBaseMemoryManager* Mm)
  : CUnixInode(Mm)
  , m_pVol(NULL)
  , m_pInode(NULL)
  , m_pInodeSize(NULL)
  , m_SparseSize(0)
  , m_pDevice(NULL)
  , m_pFileName(NULL)
  , m_NameLen(0)
  , m_VolId(0)
  , m_pCmpAttr(NULL)
  , m_CompressedAttrLen(0)
  , m_bClonedData(false)
  , m_bClonedFlagsValid(false)
{
  m_EAList.init();
}


/////////////////////////////////////////////////////////////////////////////
CApfsInode::~CApfsInode()
{
  int Status = Dtor();

  if (!UFSD_SUCCESS(Status))
    ULOG_ERROR((GetLog(), Status, "Error on inode destruction"));

  while (!m_EAList.is_empty())
  {
    InodeXAttr* x = list_entry(m_EAList.next, InodeXAttr, m_Entry);
    x->m_Entry.remove();
    delete x;
  }

  Free2(m_pInode);
  Free2(m_pCmpAttr);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::Dtor()
{
  //ULOG_TRACE((GetLog(), "CApfsInode::~CApfsInode %" PLL "x%s", m_Id, m_bDelOnClose ? ", DelOnClose":""));
  assert(m_RefCount == 0);

  if (m_pInode != NULL)
  {
    CHECK_CALL(Flush());

    if (m_bDelOnClose)
    {
      m_bDelOnClose = false;
      if (GetLinksCount() == 0)
        CHECK_CALL(m_pSuper->UnlinkInode(this));
    }
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::Init(
  IN CUnixSuperBlock*   Super,
  IN UINT64             Id,
  bool                  fCreate
  )
{
  m_pSuper = Super;
  m_Strings = m_pSuper->m_Strings;
  m_Id = Id;
  m_TreeEntry.key = m_Id;
  m_VolId = APFS_GET_TREE_ID(m_Id);
  m_pVol = reinterpret_cast<CApfsSuperBlock*>(m_pSuper)->GetVolume(m_VolId);

  CApfsTree* pRoot = GetTree();

  if (APFS_GET_INODE_ID(Id) == APFS_VOLUMES_DIR_ID || !pRoot)
  {
    CHECK_PTR(m_pInode = reinterpret_cast<apfs_inode*>(Zalloc2(sizeof(apfs_inode))));
    ++m_RefCount;
    return ERR_NOERROR;
  }

//  ULOG_TRACE((GetLog(), "CApfsInode::Init r=%" PLL "x%s", Id, fCreate? ",new":""));

  if (fCreate)
  {
    m_RefCount++;
    return ERR_NOERROR;
  }

  CApfsTreeEnum *pEnum = pRoot->GetDefaultEnum();
  if (!pEnum)
    return ERR_BADPARAMS;

  UINT64 InodeId = GetInodeId();
  CSimpleSearchKey Key(m_Mm, InodeId, ApfsInode);
  CHECK_CALL_SILENT(pEnum->StartEnumByKey(&Key, SEARCH_KEY_GE | SEARCH_ALL_TYPES));

  apfs_key* k;
  void* Data;
  unsigned short DataSize;

  while (UFSD_SUCCESS(pEnum->EnumNextPtrByKey((void**)&k, &Data, NULL, &DataSize)))
  {
    UINT64 id = CPU2LE(k->id);
    char type = GET_TYPE(id);

    if (type > ApfsExtent || GET_ID(id) > InodeId)
      break;

    if (type == ApfsInode)
    {
      apfs_inode* pInode = reinterpret_cast<apfs_inode*>(Data);
      CHECK_PTR(m_pInode = reinterpret_cast<apfs_inode*>(Zalloc2(DataSize)));
      Memcpy2(m_pInode, pInode, DataSize);
      BE_ONLY(ConvertInode(m_pInode));

      void* pInodeField = Add2Ptr(&m_pInode->field[0], m_pInode->ai_number_of_fields * sizeof(apfs_inode_field));
      for (unsigned short i = 0; i < m_pInode->ai_number_of_fields; i++)
      {
        switch (m_pInode->field[i].if_ftype)
        {
        case INODE_FTYPE_NAME:
          m_pFileName = reinterpret_cast<unsigned char*>(pInodeField);
          m_NameLen = CPU2LE(m_pInode->field[i].if_size);
          assert(!m_Strings || m_NameLen == m_Strings->strlen(api::StrUTF8, m_pFileName)+1);
          break;
        case INODE_FTYPE_SIZE:
          m_pInodeSize = reinterpret_cast<apfs_data_size*>(pInodeField);
#ifdef BASE_BIGENDIAN
          SwapBytesInPlace(&m_pInodeSize->ds_blocks);
          SwapBytesInPlace(&m_pInodeSize->ds_bytes);
          SwapBytesInPlace(&m_pInodeSize->ds_written);
          SwapBytesInPlace(&m_pInodeSize->ds_read);
#endif
          m_SizeInBytes = m_pInodeSize->ds_bytes;
          break;
        case INODE_FTYPE_SPARSE_BYTES:
          m_SparseSize = *reinterpret_cast<UINT64*>(pInodeField);
          break;
        case INODE_FTYPE_05:
        case INODE_FTYPE_DOC_ID:
          break;
        case INODE_FTYPE_DEVICE:
          m_pDevice = reinterpret_cast<unsigned int*>(pInodeField);
          break;
        default:
          assert(!"Unknown inode field");
        }
        pInodeField = Add2Ptr(pInodeField, QuadAlign(CPU2LE(m_pInode->field[i].if_size)));
      }
    }
    else if (type == ApfsAttribute)
    {
      apfs_xattr_data* xData = reinterpret_cast<apfs_xattr_data*>(Data);
      apfs_xattr_key*  xKey = reinterpret_cast<apfs_xattr_key*>(k);
      assert(GET_ID(CPU2LE(xKey->id)) == InodeId);

      InodeXAttr* xAttr = new(m_Mm) InodeXAttr(m_Mm);
      CHECK_PTR(xAttr);
      xAttr->m_Entry.insert_before(&m_EAList);
      CHECK_CALL(xAttr->Init(xKey->name, xKey->name_len, xData->xd_type, xData->xd_data, xData->xd_len));
    }
    else if (type == ApfsExtent)
    {
      // detect start cluster to provide file continuity
      apfs_extent_data* pExtent = reinterpret_cast<apfs_extent_data*>(Data);

      if (m_StartExtent.Lcn == SPARSE_LCN && pExtent->ed_block != 0)
      {
        if (k->extent.file_offset == 0)
          m_StartExtent.Len = static_cast<size_t>(CPU2LE(pExtent->ed_size) >> m_pSuper->m_Log2OfCluster);
        m_StartExtent.Lcn = CPU2LE(pExtent->ed_block) - (CPU2LE(k->extent.file_offset) >> m_pSuper->m_Log2OfCluster);
        break;
      }
    }
  }

  if (m_pInode == NULL)
    return ERR_NOTFOUND;

  int Status = ERR_NOERROR;

  if (U_ISLNK(GetMode()))
  {
    // calc size of xattrs
    size_t size;
    Status = GetEa(XATTR_SYMLINK, XATTR_SYMLINK_LEN, NULL, 0, &size);
    if (Status == ERR_MORE_DATA)
      Status = ERR_NOERROR;
    else if (Status == ERR_NOFILEEXISTS)
    {
      ULOG_WARNING((GetLog(), "Symlink has no '%s' extended attribute", XATTR_SYMLINK));
      Status = ERR_NOERROR;
    }

    CHECK_STATUS(Status);

    m_SizeInBytes = static_cast<UINT64>(size) + 1;  // to prevent data loss on 32-bit BE platform
  }

  if (IsCompressed() && FlagOn(m_pInode->ai_fs_flags, FS_IFLAG_FILESIZE))
    m_SizeInBytes = m_pInode->ai_file_size;

  if (Status == ERR_NOTFOUND)
    Status = ERR_NOERROR;

  if (UFSD_SUCCESS(Status))
    m_RefCount++;

//  ULOG_TRACE((GetLog(), "CApfsInode::Init -> %x", Status));

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::InitCompression()
{
  if (m_pCmpAttr != NULL)
  {
    assert(m_SizeInBytes == m_pCmpAttr->data_size);   //init m_SizeInBytes on m_pCmpAttr init.
    return ERR_NOERROR;
  }

  size_t Size = 0;
  int Status = GetEa(XATTR_COMPRESSED, XATTR_COMPRESSED_LEN, NULL, 0, &Size);

  if (Size < sizeof(apfs_compressed_attr))
  {
    ULOG_ERROR((GetLog(), ERR_READFILE, "Bad size of compressed attribute %" PZZ "x", Size));
    return ERR_READFILE;
  }

  if (UFSD_SUCCESS(Status) || Status == ERR_MORE_DATA)
  {
    CHECK_PTR(m_pCmpAttr = reinterpret_cast<apfs_compressed_attr*>(Malloc2(Size)));
    CHECK_CALL(GetEa(XATTR_COMPRESSED, XATTR_COMPRESSED_LEN, m_pCmpAttr, Size, NULL));

#ifdef BASE_BIGENDIAN
    SwapBytesInPlace(&m_pCmpAttr->magic);
    SwapBytesInPlace(&m_pCmpAttr->data_size);
    SwapBytesInPlace(&m_pCmpAttr->type);
#endif

    m_SizeInBytes = m_pCmpAttr->data_size;
    m_CompressedAttrLen = Size;

    if (m_pCmpAttr->magic != APFS_COMPRESSED_ATTR_MAGIC)
    {
      ULOG_ERROR((GetLog(), ERR_READFILE, "Bad magic in compressed file: %x", m_pCmpAttr->magic));
      return ERR_READFILE;
    }
  }
  else if (Status == ERR_NOFILEEXISTS)
  {
    ULOG_WARNING((GetLog(), "Inode has a compressed flag, but '%s' extended attribute missed", XATTR_COMPRESSED));
    return ERR_READFILE;
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
UINT64 CApfsInode::GetSize(
    IN bool bFork
    ) const
{
  if (bFork)
  {
    InodeXAttr* xAttr = NULL;

    if (UFSD_SUCCESS(GetXAttr(XATTR_FORK, XATTR_FORK_LEN, &xAttr)))
      return xAttr->m_ValueSize;
    return 0;
  }

  if (IsCompressed() && m_SizeInBytes == 0)
  {
    int Status = const_cast<CApfsInode*>(this)->InitCompression();
    if (!UFSD_SUCCESS(Status))
    {
      ULOG_ERROR((GetLog(), Status, "Compression initialization failed"));
      return 0;
    }
  }

  return (U_ISLNK(m_pInode->ai_mode) && m_SizeInBytes > 0) ? m_SizeInBytes - 1 : m_SizeInBytes;
}


/////////////////////////////////////////////////////////////////////////////
UINT64 CApfsInode::GetAllocatedSize(
    IN bool bFork
    ) const
{
  if (bFork)
  {
    InodeXAttr* xAttr = NULL;

    if (UFSD_SUCCESS(GetXAttr(XATTR_FORK, XATTR_FORK_LEN, &xAttr)) && xAttr->m_Extent)
      return xAttr->m_AllocSize;
    return 0;
  }

  return m_pInodeSize ? m_pInodeSize->ds_blocks : 0;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetSparseSize()
{
  m_SparseSize = 0;

  UINT64 Id;
  CHECK_CALL(GetExtentId(&Id));

  CSimpleSearchKey Key(m_Mm, Id, ApfsExtent);
  CApfsTreeEnum* pEnum = GetTree()->GetDefaultEnum();

  CHECK_CALL(pEnum->StartEnumByKey(&Key, SEARCH_KEY_EQ));

  apfs_extent_key* k;
  apfs_extent_data* pExtent;
  int Status;

  while (UFSD_SUCCESS(Status = pEnum->EnumNextPtrByKey((void**)&k, (void**)&pExtent)))
  {
    if (pExtent->ed_block == 0)
      m_SparseSize += pExtent->ed_size;
  }

  return Status != ERR_NOTFOUND ? Status : ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetObjectInfo(
    OUT FileInfo*  Info,
    IN  bool       bReset
    ) const
{
  UINT64 InodeId = APFS_GET_INODE_ID(m_Id);

  if (InodeId == APFS_VOLUMES_DIR_ID)
    return ERR_NOERROR;

  CApfsVolumeSb* Vsb = reinterpret_cast<CApfsSuperBlock*>(m_pSuper)->GetVolume(m_VolId);

  if (Vsb == NULL)
  {
    ULOG_WARNING((GetLog(), "Unknown Volume Id (%x)", m_VolId));
    return ERR_BADPARAMS;
  }

  if (bReset)
    Memzero2(Info, sizeof(FileInfo));

  if ((InodeId == APFS_ROOT_INO && m_VolId != 0) || !GetTree())
    return GetSubVolumeInfo(m_Id, *Info);

//  ULOG_TRACE((GetLog(), "GetObjectInfo r=%" PLL "x", m_Id));

  Info->Id = GetExternalId();
  Info->Uid = m_pInode->ai_uid;
  Info->Gid = m_pInode->ai_gid;
  Info->Mode = GetMode();

  switch (Info->Mode & U_IFMT)
  {
  case U_IFREG:
    Info->Attrib = UFSD_NORMAL;
    break;
  case U_IFDIR:
    Info->Attrib = UFSD_SUBDIR;
    break;
  case U_IFLNK:
    Info->Attrib = UFSD_LINK;
    break;
  case U_IFSOCK:
    Info->Attrib = UFSD_SYSTEM;
  default:
    break;
  }

  if ((Info->Mode & U_IWUGO) == 0 || Vsb->IsReadOnly())
    SetFlag(Info->Attrib, UFSD_RDONLY);

  SetFlag(Info->Attrib, UFSD_UGM);

  Info->FSAttrib = m_pInode->ai_fs_flags;
  SetFlag(Info->Attrib, UFSD_FSSPEC);

  if (U_ISBLK(Info->Mode) || U_ISCHR(Info->Mode))
  {
    if (m_pDevice != NULL)
    {
      unsigned int major = APFS_DEVICE_SPEC_MAJOR(*m_pDevice);
      unsigned int minor = APFS_DEVICE_SPEC_MINOR(*m_pDevice);

      Info->Dev = encode_dev(minor, major);
    }
    else
      ULOG_WARNING((GetLog(), "Inode has a device flag but special field is missed or not initialized"));
  }

#ifdef UFSD_DRIVER_LINUX
  Info->Attrib = Info->Attrib | UFSD_VSIZE;
  if ( APFS_ROOT_INO == APFS_GET_INODE_ID(m_Id) )
      Info->Attrib = Info->Attrib|UFSD_SYSTEM|UFSD_HIDDEN|UFSD_SUBDIR|UFSD_VSIZE;
  // inode times stored in nano-seconds since epoch time
  Info->CrTime     = m_pSuper->TimeUnixToPosixNs(m_pInode->ai_crtime);
  Info->ReffTime   = m_pSuper->TimeUnixToPosixNs(m_pInode->ai_atime);
  Info->ModiffTime = m_pSuper->TimeUnixToPosixNs(m_pInode->ai_mtime);
  Info->ChangeTime = m_pSuper->TimeUnixToPosixNs(m_pInode->ai_ctime);
#else
  Info->CrTime     = m_pSuper->TimeUnixToUFSDNs(m_pInode->ai_crtime);
  Info->ReffTime   = m_pSuper->TimeUnixToUFSDNs(m_pInode->ai_atime);
  Info->ModiffTime = m_pSuper->TimeUnixToUFSDNs(m_pInode->ai_mtime);
  Info->ChangeTime = m_pSuper->TimeUnixToUFSDNs(m_pInode->ai_ctime);
#endif

  Info->ValidSize =
  Info->FileSize  = GetSize();
  Info->AllocSize = GetAllocatedSize();
  Info->HardLinks = static_cast<unsigned short>(m_pInode->ai_nlinks);

  if (U_ISDIR(Info->Mode))
  {
    Info->nSubDirs = m_pInode->ai_nchildren;
    SetFlag( Info->Attrib, UFSD_NSUBDIRS );
    DRIVER_ONLY(Info->HardLinks = 1); // driver expects 1 hardlink for the directory
  }

  if (HasEa())
    SetFlag(Info->Attrib, UFSD_EA);

  if (IsCompressed())
    SetFlag(Info->Attrib, UFSD_COMPRESSED);

  if (IsSparsed())
    SetFlag(Info->Attrib, UFSD_SPARSED);

  if (IsSparsed() || IsCompressed())
  {
    SetFlag( Info->Attrib, UFSD_TASIZE );
    Info->TotalAllocSize = GetAllocatedSize();
  }

  if (!Vsb->CanDecrypt())
    SetFlag(Info->Attrib, UFSD_ENCRYPTED);

//  ULOG_TRACE((GetLog(), "GetObjectInfo r=%" PLL "x: sz=%" PLL "x,%" PLL "x,%" PLL "x%s%s",
//    Info->Id, Info->FileSize, Info->AllocSize, Info->TotalAllocSize, IsCompressed()? ",cmpr":"", IsEncrypted()? ",encr":""));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
bool CApfsInode::IsClonedInternal(UINT64 ExtentId) const
{
  int Status;

  //Check extent status only for data (not for ea)
  if (ExtentId == m_pInode->ai_extent_id)
  {
    CSimpleSearchKey StatusKey(m_Mm, ExtentId, ApfsExtentStatus);
    unsigned int* Cloned = NULL;

    Status = GetTree()->GetData(&StatusKey, SEARCH_KEY_EQ, (void**)&Cloned);
    if (UFSD_SUCCESS(Status))
    {
      if (Cloned && *Cloned > 1)
        return true;
    }
    else if (Status != ERR_NOTFOUND)
      ULOG_ERROR((GetLog(), Status, "Error getting ExtentStatus while looking if file is cloned"));
  }

  //Enum all extents, look in extent tree
  bool bHasClonedExtents = false;
  Status = EnumerateVolumeExtents(ExtentId, false, &bHasClonedExtents);
  if (!UFSD_SUCCESS(Status))
    ULOG_ERROR((GetLog(), Status, "Error in enumerating VolumeExtentTree while looking if file is cloned:"));

  return bHasClonedExtents;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::EnumerateVolumeExtents(
    IN  UINT64  ExtentId,
    IN  bool    bDelete,
    OUT bool*   pHasClonedExtents
    ) const
{
  int Status;
  CSimpleSearchKey Key(m_Mm, ExtentId, ApfsExtent);
  apfs_extent_key* k;
  apfs_extent_data* d;
  bool bHasClonedExtents = false;

  CApfsTreeEnum *pEnum = new(m_Mm) CApfsTreeEnum(GetTree()->GetSuper());
  CHECK_PTR(pEnum);
  CHECK_CALL_EXIT(pEnum->Init(GetTree()));
  CHECK_CALL_EXIT(pEnum->StartEnumByKey(&Key, SEARCH_KEY_EQ));

  while (UFSD_SUCCESS(Status = pEnum->EnumNextPtrByKey((void**)&k, (void**)&d)))
  {
    if (d->ed_block != 0)
    {
      UINT64 RootTreeExtentSize  = d->ed_size >> m_pSuper->m_Log2OfCluster;
      UINT64 RootTreeExtentStart = d->ed_block;
      UINT64 RootTreeExtentEnd   = RootTreeExtentStart + RootTreeExtentSize;

      CApfsTreeEnum* pExtentEnum = GetExtentTree()->GetDefaultEnum();
      CSimpleSearchKey VolExtent(m_Mm, RootTreeExtentStart, ApfsVolumeExtent);
      CHECK_CALL_EXIT(pExtentEnum->StartEnumByKey(&VolExtent, SEARCH_KEY_LE));

      apfs_volume_extent_data* ved;
      apfs_volume_extent_key*  vek;

      while (UFSD_SUCCESS(Status = pExtentEnum->EnumNextPtr((void**)&vek, (void**)&ved)))
      {
        UINT64 VolumeExtentStart = GET_ID(vek->id);
        UINT64 VolumeExtentEnd = VolumeExtentStart + ved->ved_number_of_blocks;

        if (VolumeExtentEnd <= RootTreeExtentStart)
          continue;

        if (VolumeExtentStart >= RootTreeExtentEnd)
          break;

        if (ved->ved_extent_links > 1)
        {
          bHasClonedExtents = true;

          if (!bDelete)
            goto Exit;

#ifndef UFSD_APFS_RO
          CHECK_CALL_EXIT(SplitVolumeExtent(vek, ved, RootTreeExtentStart, RootTreeExtentEnd, VolumeExtentStart, VolumeExtentEnd));
#endif
        }
#ifndef UFSD_APFS_RO
        else if (bDelete && ved->ved_extent_links == 1)
          CHECK_CALL_EXIT(reinterpret_cast<CApfsSuperBlock*>(m_pSuper)->MarkAsFree(m_VolId, true, RootTreeExtentStart, static_cast<size_t>(RootTreeExtentSize)));
#endif
      }
    }

#ifndef UFSD_APFS_RO
    if (bDelete)
    {
      CExtentSearchKey RemKey(m_Mm, k);
      CHECK_CALL_EXIT(GetTree()->RemoveExtentLeaf(&RemKey));
    }
#endif
  }

Exit:
  delete pEnum;

  if (Status == ERR_NOTFOUND)
    Status = ERR_NOERROR;

  if (pHasClonedExtents && UFSD_SUCCESS(Status))
    *pHasClonedExtents = bHasClonedExtents;

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetSubVolumeInfo(
    IN  UINT64    Id,
    OUT FileInfo& Info
    ) const
{
  unsigned char VolId = APFS_GET_TREE_ID(Id);

  ULOG_TRACE((GetLog(), "GetSubVolumeInfo r=%x", VolId));

  if (VolId != 0)
    CHECK_CALL(GetSubVolumeInfo(0, Info));

  CApfsVolumeSb* Vsb = reinterpret_cast<CApfsSuperBlock*>(m_pSuper)->GetVolume(static_cast<unsigned char>(VolId));

  char* VolName   = Vsb->GetName();
#ifndef UFSD_DRIVER_LINUX
  Info.NameType   = api::StrUTF8;
  Info.NameLen    = static_cast<unsigned short>(m_Strings->strlen(api::StrUTF8, VolName));
  Memcpy2(Info.Name, VolName, Info.NameLen + 1);

  Info.CrTime     = m_pSuper->TimeUnixToUFSDNs(Vsb->GetTimeCreated());
  Info.ModiffTime =
  Info.ReffTime   =
  Info.ChangeTime = m_pSuper->TimeUnixToUFSDNs(Vsb->GetTimeModified());
  Info.HardLinks  = 0xFFFF;
#else
  size_t DstLen;
  verify(UFSD_SUCCESS(m_Strings->Convert(api::StrUTF8, VolName, m_Strings->strlen(api::StrUTF8, VolName),
                                         api::StrUTF16, Info.Name, MAX_FILE_NAME, &DstLen)));
  Info.NameLen    = DstLen;

  Info.CrTime     = m_pSuper->TimeUnixToPosixNs(Vsb->GetTimeCreated());
  Info.ModiffTime =
  Info.ReffTime   =
  Info.ChangeTime = m_pSuper->TimeUnixToPosixNs(Vsb->GetTimeModified());

  Info.HardLinks  = 1;
#endif

  Info.Id         = VolId == 0 ? Id : APFS_MAKE_INODE_ID(VolId, APFS_VOLUMES_DIR_ID);
  Info.Attrib     = UFSD_SUBDIR | UFSD_VSIZE | UFSD_NSUBDIRS;

  if (!Vsb->CanDecrypt())
    SetFlag(Info.Attrib, UFSD_ENCRYPTED);
  if (Vsb->IsReadOnly())
    SetFlag(Info.Attrib, UFSD_RDONLY);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadData(
  IN  UINT64   Offset,
  OUT size_t*  OutLen,
  OUT void*    pBuffer,
  IN  size_t   BufSize,
  IN  bool     bFork
  )
{
  if (U_ISLNK(GetMode()))
  {
    InodeXAttr* xData = NULL;
    CHECK_CALL(GetXAttr(XATTR_SYMLINK, XATTR_SYMLINK_LEN, &xData));
    CHECK_CALL(GetXAttrData(xData, 0, BufSize, pBuffer, OutLen));

    return ERR_NOERROR;
  }

  if (U_ISCHR(GetMode()) || U_ISBLK(GetMode()))
  {
    *(unsigned int*)pBuffer = *m_pDevice;

    if (OutLen)
      *OutLen = sizeof(unsigned int);

    return ERR_NOERROR;
  }

  // Compression flag check should be before the clone flag check due to compressed files could be cloned too
  if (!bFork && IsCompressed())
    return ReadCompressedData(Offset, OutLen, pBuffer, BufSize);

  size_t OutSize = 0;
  int Status = ERR_NOERROR;
  unsigned int BlockSize = m_pSuper->GetBlockSize();

  while (BufSize)
  {
    unsigned int BlockOffset = static_cast<unsigned>(mod_u64(Offset, BlockSize));
    size_t MaxLen = CEIL_UP(BlockOffset + BufSize, BlockSize);

    CUnixExtent Extent;
    CHECK_CALL(LoadBlocks(CEIL_DOWN64(Offset, BlockSize), MaxLen, &Extent, bFork, false));

    if (Extent.Len == 0)
      break;

    assert(Extent.Len <= MaxLen);
    UINT64 LastByte = static_cast<UINT64>(Extent.Len) << m_pSuper->m_Log2OfCluster;
    size_t Bytes = LastByte - BlockOffset > BufSize ? BufSize : static_cast<size_t>(LastByte - BlockOffset);

    if (Extent.Lcn != SPARSE_LCN)
      CHECK_CALL(m_pVol->ReadData((Extent.Lcn << m_pSuper->m_Log2OfCluster) + BlockOffset, pBuffer, Bytes, Extent.IsEncrypted, Extent.CryptoId));
    else
      Memzero2(pBuffer, Bytes);

    pBuffer = Add2Ptr(pBuffer, Bytes);
    BufSize -= Bytes;
    Offset += Bytes;
    OutSize += Bytes;
  }

  if (OutLen)
    *OutLen = OutSize;
  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadWriteData(
    IN UINT64   Offset,
    IN size_t*  OutLen,
    IN void*    pBuffer,
    IN size_t   BufSize,
    IN bool     bWrite,
    IN bool     bFork
    )
{
  ULOG_DEBUG1((GetLog(), "%sData r=%" PLL "x: [%" PLL "x, %" PLL "x), sz=%" PLL "x%s", bWrite? "Write":"Read",
    m_Id, Offset, Offset + BufSize, m_SizeInBytes, IsCompressed() ? ",cmpr" : IsCloned() ? ",cloned":""));

  if (pBuffer == NULL)
    return ERR_BADPARAMS;

  if (!bWrite)
    return ReadData(Offset, OutLen, pBuffer, BufSize, bFork);

#ifndef UFSD_APFS_RO
  return WriteData(Offset, OutLen, pBuffer, BufSize, bFork);
#else
  return ERR_WPROTECT;
#endif
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::GetExtent(
    IN  UINT64        Id,
    IN  UINT64        Offset,
    OUT CUnixExtent&  Extent
    ) const
{
  UINT64 Vcn = Offset >> m_pSuper->m_Log2OfCluster;

  if (m_LastExtent.Id == Id && m_LastExtent.CheckVcn(Vcn))
  {
    Extent = m_LastExtent;
    return ERR_NOERROR;
  }

  if (m_LastHole.Id == Id && m_LastHole.CheckVcn(Vcn))
  {
    Extent = m_LastHole;
    return ERR_NOERROR;
  }

  apfs_extent_key* ek;
  apfs_extent_data* ed;

  int Status = GetTree()->GetExtentPtr(Id, Offset, &ed, &ek);

  if (UFSD_SUCCESS(Status)
  && (((m_LastExtent.Lcn == SPARSE_LCN || m_LastExtent.Vcn > Vcn || ek->file_offset > m_LastExtent.Vcn << m_pSuper->m_Log2OfCluster)
    && (  m_LastHole.Lcn == SPARSE_LCN ||   m_LastHole.Vcn > Vcn || ek->file_offset >   m_LastHole.Vcn << m_pSuper->m_Log2OfCluster))
   || m_LastExtent.Id != Id))
  {
    assert(GET_ID(ek->id) == Id);
    UINT64 ExtentSizeInBlocks = CPU2LE(ed->ed_size) >> m_pSuper->m_Log2OfCluster;

    Extent.Id  = GET_ID(ek->id);
    Extent.Vcn = CPU2LE(ek->file_offset) >> m_pSuper->m_Log2OfCluster;
    Extent.Lcn = CPU2LE(ed->ed_block);
    Extent.Len = static_cast<size_t>(ExtentSizeInBlocks);
    Extent.CryptoId    = CPU2LE(ed->ed_crypto_id);
    Extent.IsAllocated = false;
    Extent.IsEncrypted = FlagOn(CPU2LE(ed->ed_flags), EXTENT_FLAG_ENCRYPTED);
    Extent.pData       = ed;

    return ERR_NOERROR;
  }

  if (m_LastExtent.Len && m_LastExtent.Id == Id)
  {
    Extent = m_LastExtent;
    Status = ERR_NOERROR;
  }

  if (m_LastHole.Len && m_LastHole.Id == Id && m_LastHole.Vcn > m_LastExtent.Vcn)
  {
    Extent = m_LastHole;
    Status = ERR_NOERROR;
  }

  assert(!UFSD_SUCCESS(Status) || Extent.Id == Id);
  return Status;
}


/////////////////////////////////////////////////////////////////////////////
bool CApfsInode::IsCloned() const
{
  if (!FlagOn(m_pInode->ai_inode_flags, FS_IFLAG_FULL_CLONED | FS_IFLAG_CLONED))
    return false;

  InodeXAttr* x;
  list_head* pos;

  if (!m_bClonedFlagsValid)
  {
    m_bClonedData = IsClonedInternal(m_pInode->ai_extent_id);
    list_for_each(pos, &m_EAList)
    {
      x = list_entry(pos, InodeXAttr, m_Entry);
      if (x->m_Extent)
        x->m_bCloned = IsClonedInternal(x->m_Extent);
    }
    m_bClonedFlagsValid = true;
  }

  if (m_bClonedData)
    return true;

  list_for_each(pos, &m_EAList)
  {
    x = list_entry(pos, InodeXAttr, m_Entry);
    if (x->m_bCloned)
      return true;
  }

  //no cloned data or ea
  ClearFlag(m_pInode->ai_inode_flags, FS_IFLAG_CLONED);
  ClearFlag(m_pInode->ai_inode_flags, FS_IFLAG_FULL_CLONED);

  return false;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::LoadBlocks(
    IN  UINT64        Vcn,
    IN  size_t        Len,
    OUT CUnixExtent*  pOutExtent,
    IN  bool          bFork,
    IN  bool          bAllocate,
    OUT UINT64*       LcnBase
    )
{
  pOutExtent->IsAllocated = false;

  if (!bFork && IsCompressed())
    pOutExtent->Lcn = SPARSE_LCN;
  else
  {
    UINT64 xId;

    CHECK_CALL(GetExtentId(&xId, bFork));
    if (xId == 0)
      pOutExtent->Lcn = SPARSE_LCN;
    else
      CHECK_CALL(FindExtent(xId, Vcn, Len, pOutExtent, LcnBase, bAllocate));
  }

  if (pOutExtent->Lcn == SPARSE_LCN && pOutExtent->Len == 0)
  {
    UINT64 OutLenTemp = CEIL_UP64(m_SizeInBytes, m_pSuper->m_BlockSize);
    pOutExtent->Len = OutLenTemp > Len ? Len : static_cast<size_t>(OutLenTemp);
  }

  if (pOutExtent->IsAllocated && Vcn == 0)
    m_StartExtent = *pOutExtent;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::FindExtent(
    IN  UINT64       Id,
    IN  UINT64       Vcn,
    IN  size_t       Len,
    OUT CUnixExtent* pOutExtent,
    OUT UINT64*      LcnBase,
    IN  bool         bAllocate
    )
{
#if __WORDSIZE < 64
  if (Vcn > MINUS_ONE_T)
  {
    ULOG_ERROR((GetLog(), ERR_NOTIMPLEMENTED, "CApfsInode::FindExtent: Vcn=0x%" PLL "x is big for 32 bit systems", Vcn));
    return ERR_NOTIMPLEMENTED;
  }
#endif
  pOutExtent->IsAllocated = false;

  if (LcnBase)
    *LcnBase = 0;

  pOutExtent->Lcn = pOutExtent->Len = 0;

  ULOG_TRACE((GetLog(), "FindExtent r=%" PLL "x: Vcn=%" PLL "x, Len=%" PZZ "x%s", Id, Vcn, Len, bAllocate ? ", alloc":""));

  CUnixExtent Extent;
  int Status = GetExtent(Id, Vcn << m_pSuper->m_Log2OfCluster, Extent);

  if (UFSD_SUCCESS(Status))
  {
    UINT64 ExtentEnd = Extent.Vcn + Extent.Len;
    UINT64 DataEnd = Vcn + Len;

    ULOG_DEBUG1((GetLog(), "Found %s: [%#" PLL "x; %#" PLL "x) -> %" PLL "x", Extent.Type(), Extent.Vcn, ExtentEnd, Extent.Lcn));

    assert(Extent.Vcn <= Vcn);
    assert(Extent.Id == Id);

    if ((DataEnd > Extent.Vcn && DataEnd <= ExtentEnd) || (Vcn >= Extent.Vcn && Vcn < ExtentEnd))
    {
      if (Extent.Lcn == 0)
      {
        if (!bAllocate)
        {
          pOutExtent->Len = Len > 0 ? MIN(Extent.Len, Len) : static_cast<size_t>(ExtentEnd - Vcn);
          pOutExtent->Lcn = SPARSE_LCN;
        }
#ifndef UFSD_APFS_RO
        else
          CHECK_CALL(SplitSparseExtent(Vcn, Len, Extent, pOutExtent));
#endif
      }
      else // not a sparse file
      {
        pOutExtent->Len = (DataEnd <= ExtentEnd && Len > 0) ? Len : static_cast<size_t>(ExtentEnd - Vcn);
        pOutExtent->Lcn = Extent.Lcn + Vcn - Extent.Vcn;
        pOutExtent->IsEncrypted = Extent.IsEncrypted;
        pOutExtent->CryptoId = Extent.CryptoId + Vcn - Extent.Vcn;
      }

      if (LcnBase)
        *LcnBase = pOutExtent->Lcn;
    }
    else
    {
      if (!bAllocate)
      {
        pOutExtent->Lcn = SPARSE_LCN;
        pOutExtent->Len = Len;
      }
#ifndef UFSD_APFS_RO
      else
        CHECK_CALL(AllocateExtent(Id, Vcn, Len, Extent, pOutExtent, LcnBase));
#endif
    }
  }
  else if (Status == ERR_NOTFOUND) // no extents
  {
    if (!bAllocate)
    {
      pOutExtent->Lcn = SPARSE_LCN;
      pOutExtent->Len = Len;
    }
#ifndef UFSD_APFS_RO
    else
      CHECK_CALL(CreateExtent(Id, Vcn, Len, pOutExtent, LcnBase));
#endif

    Status = ERR_NOERROR;
  }

  ULOG_TRACE((GetLog(), "FindExtent -> Lcn=%" PLL "x,%s Len=%" PZZ "x", pOutExtent->Lcn, pOutExtent->IsAllocated? "new":"", pOutExtent->Len));

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadCompressedData(
    IN  UINT64  Offset,
    OUT size_t* OutLen,
    IN  void*   pBuffer,
    IN  size_t  BufSize
    )
{
  CHECK_CALL(InitCompression());

  int Status;
  UCompressFactory CmpFactory(m_Mm, GetLog());
  api::ICompress* Compressor = NULL;

  switch (m_pCmpAttr->type)
  {
  case DecmpfsInlineZlibData:
  case ResourceForkZlibData:
    CHECK_CALL(CmpFactory.CreateProvider(I_COMPRESS_DEFLATE, &Compressor));
    break;

  case DecmpfsInlineLZData:
  case ResourceForkLZData:
    CHECK_CALL(CmpFactory.CreateProvider(I_COMPRESS_LZFSE, &Compressor));
    break;

  case DecmpfsVersion:
    Memzero2(pBuffer, BufSize);
    *OutLen = BufSize;
    return ERR_NOERROR;

  default:
    break;
  }

  switch (m_pCmpAttr->type)
  {
  case DecmpfsInlineZlibData:
  case DecmpfsInlineLZData:
    Status = ReadInlineCompressedData(Compressor, Offset, OutLen, pBuffer, BufSize);
    break;

  case ResourceForkZlibData:
  case ResourceForkLZData:
    Status = ReadResourceCompressedData(Compressor, Offset, OutLen, pBuffer, BufSize);
    break;

  default:
    ULOG_ERROR((GetLog(), ERR_NOTIMPLEMENTED, "Unknown compression type %d", m_pCmpAttr->type));
    Status = ERR_NOTIMPLEMENTED;
  }

  if (Compressor)
    Compressor->Destroy();

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadInlineCompressedData(
    IN  api::ICompress* Compressor,
    IN  UINT64          Offset,
    OUT size_t*         OutLen,
    OUT void*           pBuffer,
    IN  size_t          Size
    ) const
{
  if (m_SizeInBytes > APFS_UNCOMPRESS_BUFFER_SIZE)
    return ERR_BADPARAMS;

  unsigned char* AttrData = Add2Ptr(m_pCmpAttr, sizeof(apfs_compressed_attr));
  size_t AttrLen = m_CompressedAttrLen - sizeof(apfs_compressed_attr);
  assert(m_CompressedAttrLen >= sizeof(apfs_compressed_attr));

  unsigned char* CmpData;
  size_t CmpLen = AttrLen;

  if (m_pCmpAttr->type == DecmpfsInlineLZData)
  {
    size_t CmpBufSize = AttrLen + sizeof(APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC) +
      MAX(sizeof(apfs_lzvn_compressed_block_header), sizeof(apfs_lzvn_uncompressed_block_header));

    CHECK_PTR(CmpData = reinterpret_cast<unsigned char*>(Malloc2(CmpBufSize)));

    if (AttrLen > m_SizeInBytes)
    {
      unsigned char* OutPtr = CmpData + sizeof(apfs_lzvn_uncompressed_block_header) - 1;
      Memcpy2(OutPtr, AttrData, AttrLen);
      assert(*OutPtr == APFS_LZFSE_UNCOMPRESSED_DATA);

      apfs_lzvn_uncompressed_block_header* Header = reinterpret_cast<apfs_lzvn_uncompressed_block_header*>(CmpData);
      Header->magic = APFS_LZFSE_UNCOMPRESSED_BLOCK_MAGIC;
      Header->n_raw_bytes = static_cast<unsigned int>(m_SizeInBytes);

      OutPtr += AttrLen;
      *(unsigned int*)OutPtr = APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC;

      CmpLen += sizeof(apfs_lzvn_uncompressed_block_header) + sizeof(APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC) - 1;
    }
    else
    {
      apfs_lzvn_compressed_block_header* Header = reinterpret_cast<apfs_lzvn_compressed_block_header*>(CmpData);

      Header->magic = APFS_LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC;
      Header->n_payload_bytes = static_cast<unsigned int>(AttrLen);
      Header->n_raw_bytes = static_cast<unsigned int>(m_SizeInBytes);

      unsigned char* OutPtr = CmpData + sizeof(apfs_lzvn_compressed_block_header);
      Memcpy2(OutPtr, AttrData, AttrLen);
      OutPtr += AttrLen;
      *(unsigned int*)OutPtr = APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC;

      CmpLen += sizeof(apfs_lzvn_compressed_block_header) + sizeof(APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC);
    }
  }
  else
    CmpData = AttrData;

  size_t DecmpDataSize;
  size_t BytesToCopy;
  int Status;
  unsigned char* DecmpBuf = reinterpret_cast<unsigned char*>(Malloc2(m_SizeInBytes));

  CHECK_PTR_EXIT(DecmpBuf);
  CHECK_CALL_EXIT(Compressor->Decompress(CmpData, CmpLen, DecmpBuf, static_cast<size_t>(m_SizeInBytes), &DecmpDataSize));

  BytesToCopy = (Offset + Size > DecmpDataSize) ? DecmpDataSize - Offset : Size;
  Memcpy2(pBuffer, Add2Ptr(DecmpBuf, Offset), BytesToCopy);
  *OutLen = BytesToCopy;

Exit:
  Free2(DecmpBuf);
  if (m_pCmpAttr->type == DecmpfsInlineLZData)
    Free2(CmpData);

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadResourceCompressedData(
    IN  api::ICompress* Compressor,
    IN  UINT64          off,
    OUT size_t*         OutLen,
    OUT void*           pBuffer,
    IN  size_t          Size
    )
{
  unsigned int Entries = 0;
  size_t CmpBufSize = 0;
  apfs_compressed_block* Blocks = NULL;
  int Status = ERR_NOERROR;

  InodeXAttr* xData = NULL;
  CHECK_CALL(GetXAttr(XATTR_FORK, XATTR_FORK_LEN, &xData));

  if (m_pCmpAttr->type == ResourceForkZlibData)
    CHECK_CALL(ReadZlibBlockInfo(xData, &Blocks, &Entries, &CmpBufSize));
  else if (m_pCmpAttr->type == ResourceForkLZData)
    CHECK_CALL(ReadLZBlockInfo(xData, &Blocks, &Entries, &CmpBufSize));
  else
  {
    assert(!"Unknown compression type");
    return ERR_NOTIMPLEMENTED;
  }

  assert(0 != CmpBufSize);

  UINT64 FirstBlock = CEIL_DOWN64(off, APFS_UNCOMPRESS_BUFFER_SIZE);
  UINT64 WantedBlocks = CEIL_UP64(off + Size, APFS_UNCOMPRESS_BUFFER_SIZE) - FirstBlock;
  UINT64 CurBlock = FirstBlock;
  size_t BlockOffset = static_cast<size_t>(mod_u64(off, APFS_UNCOMPRESS_BUFFER_SIZE));
  size_t offset = 0;

  void* DecmpBuf = Malloc2(APFS_UNCOMPRESS_BUFFER_SIZE);
  void* CmpBuf = Malloc2(CmpBufSize);
  CHECK_PTR_EXIT(DecmpBuf);
  CHECK_PTR_EXIT(CmpBuf);

  while ((CurBlock < FirstBlock + WantedBlocks) &&
         (CurBlock <= Entries))
  {
    size_t CmpBlockOffset, CmpBlockSize = 0;
    size_t Bytes;

    if (m_pCmpAttr->type == ResourceForkZlibData)
    {
      CmpBlockOffset = Blocks[CurBlock].offset;
      CmpBlockSize = Blocks[CurBlock].size;

      CHECK_CALL_EXIT(GetXAttrData(xData, CmpBlockOffset, CmpBlockSize, CmpBuf, &Bytes));
      assert(Bytes == Blocks[CurBlock].size);
    }
    else if (m_pCmpAttr->type == ResourceForkLZData)
    {
      CmpBlockOffset = Blocks[CurBlock].offset;
      CmpBlockSize = Blocks[CurBlock].size;

      size_t DecmpBlockSize = (CurBlock < Entries - 1) ? APFS_UNCOMPRESS_BUFFER_SIZE : mod_u64(m_SizeInBytes, APFS_UNCOMPRESS_BUFFER_SIZE);

      if (0 == DecmpBlockSize)
        DecmpBlockSize = APFS_UNCOMPRESS_BUFFER_SIZE;

      void* OutPtr;

      if (CmpBlockSize > DecmpBlockSize)
      {
        OutPtr = Add2Ptr(CmpBuf, sizeof(apfs_lzvn_uncompressed_block_header) - 1);
        CHECK_CALL_EXIT(GetXAttrData(xData, CmpBlockOffset, CmpBlockSize, OutPtr, &Bytes));
        assert(*(char*)OutPtr == APFS_LZFSE_UNCOMPRESSED_DATA);

        apfs_lzvn_uncompressed_block_header* Header = reinterpret_cast<apfs_lzvn_uncompressed_block_header*>(CmpBuf);
        Header->magic = APFS_LZFSE_UNCOMPRESSED_BLOCK_MAGIC;
        Header->n_raw_bytes = static_cast<unsigned int>(DecmpBlockSize);

        OutPtr = Add2Ptr(OutPtr, CmpBlockSize);

        CmpBlockSize += sizeof(apfs_lzvn_uncompressed_block_header) + sizeof(APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC) - 1;
      }
      else
      {
        apfs_lzvn_compressed_block_header* Header = reinterpret_cast<apfs_lzvn_compressed_block_header*>(CmpBuf);
        Header->magic = APFS_LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC;
        Header->n_payload_bytes = static_cast<unsigned int>(CmpBlockSize);
        Header->n_raw_bytes = static_cast<unsigned int>(DecmpBlockSize);

        OutPtr = Add2Ptr(CmpBuf, sizeof(apfs_lzvn_compressed_block_header));
        CHECK_CALL_EXIT(GetXAttrData(xData, CmpBlockOffset, CmpBlockSize, OutPtr, &Bytes));
        OutPtr = Add2Ptr(OutPtr, CmpBlockSize);

        CmpBlockSize += sizeof(apfs_lzvn_compressed_block_header) + sizeof(APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC);
      }

      *(unsigned int*)OutPtr = APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC;
    }

    size_t BytesToCopy = (Size - offset > APFS_UNCOMPRESS_BUFFER_SIZE - BlockOffset) ?
      APFS_UNCOMPRESS_BUFFER_SIZE - BlockOffset :
      Size - offset;

    CHECK_CALL_EXIT(Compressor->Decompress(CmpBuf, CmpBlockSize, DecmpBuf, APFS_UNCOMPRESS_BUFFER_SIZE, &Bytes));
    Memcpy2(Add2Ptr(pBuffer, offset), Add2Ptr(DecmpBuf, BlockOffset), BytesToCopy);

    BlockOffset = 0;
    ++CurBlock;
    offset += BytesToCopy;
  }

  if (OutLen)
    *OutLen = offset;
Exit:
  if (DecmpBuf)
    Free2(DecmpBuf);
  if (CmpBuf)
    Free2(CmpBuf);
  if (Blocks)
    Free2(Blocks);

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadZlibBlockInfo(
    IN  InodeXAttr*             xData,
    OUT apfs_compressed_block** Blocks,
    OUT unsigned int*           Entries,
    OUT size_t*                 CmpBufSize
    )
{
  size_t ResourceHeaderSize = sizeof(apfs_zlib_compressed_resource_fork);
  size_t ReadBytes = 0;
  apfs_zlib_compressed_resource_fork ResourceHeader;

  CHECK_CALL(GetXAttrData(xData, 0, ResourceHeaderSize, &ResourceHeader, &ReadBytes));

  if (ResourceHeader.data_table_num_entries == 0)
    return ERR_READFILE;

  assert(ReadBytes == ResourceHeaderSize);

  SwapBytesInPlace(&ResourceHeader.header.header_size);
  SwapBytesInPlace(&ResourceHeader.header.footer_offset);
  SwapBytesInPlace(&ResourceHeader.header.data_size);
  SwapBytesInPlace(&ResourceHeader.header.footer_size);
  SwapBytesInPlace(&ResourceHeader.data_table_size);

  assert(ResourceHeader.header.header_size == 0x100);
  assert(ResourceHeader.header.footer_size == 0x32);
  assert(ResourceHeader.header.data_size == ResourceHeader.header.footer_offset - ResourceHeader.header.header_size);

  *Entries = CPU2LE(ResourceHeader.data_table_num_entries);

  size_t BlockTableOffset = ResourceHeaderSize;
  size_t BlockTableSize = sizeof(apfs_compressed_block) * (*Entries);

  if (BlockTableSize > xData->m_ValueSize)
  {
    ULOG_ERROR((GetLog(), ERR_READFILE, "Amount of entries %x in compressed file is too large. DataSize = %" PZZ "x", *Entries, xData->m_ValueSize));
    return ERR_READFILE;
  }

  CHECK_PTR(*Blocks = reinterpret_cast<apfs_compressed_block*>(Malloc2(BlockTableSize)));
  int Status = GetXAttrData(xData, BlockTableOffset, BlockTableSize, *Blocks, &ReadBytes);
  if (!UFSD_SUCCESS(Status))
  {
    Free2(*Blocks);
    *Blocks = NULL;
    *Entries = 0;
    return Status;
  }

  size_t MaxSize = 0;

  for (size_t i = 0; i < *Entries; ++i)
  {
#ifdef BASE_BIGENDIAN
    SwapBytesInPlace(&(*Blocks)[i].offset);
    SwapBytesInPlace(&(*Blocks)[i].size);
#endif
    (*Blocks)[i].offset += static_cast<unsigned int>(BlockTableOffset - sizeof(int));
    if (MaxSize < (*Blocks)[i].size)
      MaxSize = (*Blocks)[i].size;
  }

  *CmpBufSize = MaxSize;

  assert(ReadBytes == BlockTableSize);

  return ERR_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
int CApfsInode::ReadLZBlockInfo(
    IN  InodeXAttr*             xData,
    OUT apfs_compressed_block** Blocks,
    OUT unsigned int*           Entries,
    OUT size_t*                 CmpBufSize
    )
{
  unsigned int CmpBlock;
  size_t Bytes;

  CHECK_CALL(GetXAttrData(xData, 0, sizeof(CmpBlock), &CmpBlock, &Bytes));
  BE_ONLY(SwapBytesInPlace(&CmpBlock));

  if (CmpBlock < sizeof(int) * 2 || CmpBlock >= xData->m_ValueSize)
  {
    ULOG_ERROR((GetLog(), ERR_READFILE, "Incorrect block %x in compressed file. DataSize = %" PZZ "x", CmpBlock, xData->m_ValueSize));
    return ERR_READFILE;
  }

  unsigned int EntryCount = CmpBlock / sizeof(int) - 1;
  size_t HeaderSize = CmpBlock;

  unsigned int* Header = reinterpret_cast<unsigned int*>(Malloc2(HeaderSize));
  CHECK_PTR(Header);

  int Status;
  apfs_compressed_block* Table;
  size_t MaxBlockSize = 0;

  CHECK_CALL_EXIT(GetXAttrData(xData, 0, HeaderSize, Header, &Bytes));
  assert(Bytes == HeaderSize);

  Table = reinterpret_cast<apfs_compressed_block*>(Malloc2(sizeof(apfs_compressed_block) * EntryCount));
  CHECK_PTR_EXIT(Table);

  for (size_t i = 0; i < EntryCount; ++i)
  {
    unsigned int Cur = CPU2LE(Header[i]);
    unsigned int Next = CPU2LE(Header[i + 1]);

    if (0 == Cur || 0 == Next || Next <= Cur)
    {
      Status = ERR_BADPARAMS;
      goto Exit;
    }

    Table[i].offset = Cur;
    Table[i].size =  Next - Cur;

    if (MaxBlockSize < Table[i].size)
      MaxBlockSize = Table[i].size;
  }

  *CmpBufSize = MaxBlockSize + sizeof(APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC) +
    MAX(sizeof(apfs_lzvn_compressed_block_header), sizeof(apfs_lzvn_uncompressed_block_header));

  *Blocks  = Table;
  *Entries = EntryCount;

  Status = ERR_NOERROR;

Exit:
  Free2(Header);
  return Status;
}


#ifdef UFSD_APFS_RO
/////////////////////////////////////////////////////////////////////////////
int CApfsInode::SetObjectInfo(const CUnixFileSystem* /*pFS*/, const FileInfo* /*fi*/, size_t /*Flags*/)
{
  return ERR_WPROTECT;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::Flush()
{
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsInode::Truncate(UINT64 /*Vcn*/)
{
  return ERR_WPROTECT;
}


#endif  // UFSD_APFS_RO

} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
