// <copyright file="apfsinode.h" company="Paragon Software Group">
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

#ifndef __UFSD_APFSINODE_H
#define __UFSD_APFSINODE_H

#include <api/compress.hpp>

namespace UFSD
{

namespace apfs
{

//XAttr for inode cache
struct InodeXAttr : UMemBased<InodeXAttr>
{
  UINT64          m_Extent;       // for external xattr
  size_t          m_ValueSize;
  UINT64          m_AllocSize;
  list_head       m_Entry;
  unsigned short  m_Type;
  unsigned short  m_NameLen;
  unsigned char*  m_pNameVal;   // Name and Value (for internal xattr)
  bool            m_bCloned;

  InodeXAttr(IN api::IBaseMemoryManager* Mm);
  ~InodeXAttr() { Free2(m_pNameVal); }

  int Init(
      IN unsigned char* Name,
      IN unsigned short NameLen,
      IN unsigned short Type,
      IN char*          Value,
      IN unsigned short ValueSize
      );

  int ReInit(
      IN unsigned short Type,
      IN char*          Value,
      IN unsigned short ValueSize
      )
  {
    return Init(NULL, 0, Type, Value, ValueSize);
  }

  void Dump(api::IBaseLog* Log) const
  {
#ifndef UFSD_TRACE
    UNREFERENCED_PARAMETER(Log);
#endif
    ULOG_DEBUG1((Log, "%.*s (%x) = %.*s (%" PZZ "x); Extent = %" PLL "x, Type = %x",
      m_NameLen, m_pNameVal, m_NameLen, (unsigned int)m_ValueSize, Add2Ptr(m_pNameVal, m_NameLen), m_ValueSize, m_Extent, m_Type));
  }
};



class CApfsInode : public CUnixInode
{
  friend class CApfsFileSystem;
  friend class CApfsDir;

  CApfsVolumeSb*        m_pVol;
  apfs_inode*           m_pInode;             //inode disk struct
  apfs_data_size*       m_pInodeSize;         //Size of inode disk struct for this inode
  UINT64                m_SparseSize;         //Sparsed area size
  unsigned int*         m_pDevice;            //Pointer to INODE_FTYPE_DEVICE field
  unsigned char*        m_pFileName;          //Pointer to file name in inode disk struct
  unsigned short        m_NameLen;            //Len of file name

  unsigned char         m_VolId;              //Volume index which this inode belongs to

  apfs_compressed_attr* m_pCmpAttr;           //Extended attribute for reading compressed files
  size_t                m_CompressedAttrLen;  //Len of compressed EA

  list_head             m_EAList;             //List of all extended attributes

  CUnixExtent           m_StartExtent;
  CUnixExtent           m_LastExtent;
  CUnixExtent           m_LastHole;

  mutable bool          m_bClonedData;         //data is cloned
  mutable bool          m_bClonedFlagsValid;   //m_bClonedData and xAttr->m_bCloned flags for all ea are valid

public:
  CApfsInode(api::IBaseMemoryManager* Mm);
  virtual ~CApfsInode();
  int Dtor();

  int Init(
      IN CUnixSuperBlock* Super,
      IN UINT64           Id,
      bool                fCreate = false
      );

  int InitCompression();

  CApfsVolumeSb* GetVolume() const { return m_pVol; }

  unsigned short GetDataSize() const { return sizeof(apfs_inode) + m_pInode->ai_fields_data_size; }

  virtual int ReadWriteData(
      IN  UINT64    Offset,
      OUT size_t*   OutLen,
      IN OUT void*  pBuffer,
      IN  size_t    Size,
      IN  bool      bWrite,
      IN  bool      bFork = false
      );

  virtual int LoadBlocks(
      IN  UINT64    Vcn,
      IN  size_t    Len,
      OUT UINT64*   Lcn,
      OUT size_t*   OutLen,
      IN  bool      bFork       = false,
      IN  bool      Allocate    = false,
      OUT bool*     IsAllocated = NULL,
      OUT UINT64*   VcnBase     = NULL
      )
  {
    CUnixExtent Extent;
    CHECK_CALL(LoadBlocks(Vcn, Len, &Extent, bFork, Allocate, VcnBase));

    *Lcn = Extent.Lcn;
    *OutLen = Extent.Len;

    if (IsAllocated)
      *IsAllocated = Extent.IsAllocated;
    return ERR_NOERROR;
  }

  virtual int LoadBlocks(
      IN  UINT64        Vcn,
      IN  size_t        Len,
      OUT CUnixExtent*  pOutExtent,
      IN  bool          bFork     = false,
      IN  bool          Allocate  = false,
      OUT UINT64*       VcnBase   = NULL
      );

  virtual int GetObjectInfo(
      OUT FileInfo*      Info,
      IN  bool           bResetObjectInfo = true
      ) const;

  virtual int SetObjectInfo(
      IN const CUnixFileSystem* pFS,
      IN const FileInfo*        Info,
      IN size_t                 Flags
      );

  virtual unsigned short GetMode() const { return static_cast<unsigned short>(m_pInode->ai_mode); }
  virtual bool IsInlineData() const { return false; }
  virtual UINT64 GetFlags() const { return m_pInode->ai_fs_flags; }
  virtual void SetFlags(UINT64 Flags) { SetFlag(m_pInode->ai_fs_flags, Flags); }
  virtual void SetUid(unsigned int uid) { m_pInode->ai_uid = uid; }
  virtual void SetGuid(unsigned int gid) { m_pInode->ai_gid = gid; }
  virtual unsigned int GetLinksCount() const { return m_pInode->ai_nlinks; }
  virtual void IncLinksCount(unsigned int N = 1) { m_pInode->ai_nlinks += N; }
  virtual void DecLinksCount(unsigned int N = 1)
  {
    assert(m_pInode->ai_nlinks >= N);
    m_pInode->ai_nlinks -= N;
  }

  bool IsEncrypted() const { return GetTree() == NULL; }
  virtual bool IsCompressed() const { return FlagOn(m_pInode->ai_fs_flags, FS_UFLAG_COMPRESSED); }
  virtual bool IsCloned() const;
  virtual bool IsSparsed() const
  {
#ifdef UFSD_DRIVER_LINUX
    return U_ISREG(m_pInode->ai_mode);
#else
    return m_SparseSize != 0;
#endif
  }

  UINT64 GetParentId() const { return m_pInode->ai_parent_id; }

  apfs_inode* GetInode() const { return m_pInode; }

  virtual UINT64 GetAllocatedBlocks(
      IN bool bFork = false
      ) const
  {
    return GetAllocatedSize(bFork) >> m_pSuper->m_Log2OfCluster;
  }

  virtual UINT64 GetSize(
      IN bool bFork = false
      ) const;

  virtual UINT64 GetAllocatedSize(
      IN bool bFork = false
      ) const;

  virtual int Flush();

  virtual int Truncate(
      IN UINT64 Vcn
      );

  //=================================================================
  // Working with extended attributes
  //=================================================================

  // Copy a list of extended attribute names into the buffer
  // provided, or compute the buffer size required
  virtual int ListEa(
    OUT void*     Buffer,
    IN  size_t    BytesPerBuffer,
    OUT size_t*   Bytes
  );

  // Reads extended attribute
  virtual int GetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    OUT void*       Value,
    IN  size_t      BytesPerValue,
    OUT size_t*     Len
  );

  // Creates/Writes extended attribute
  virtual int SetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    IN  const void* Value,
    IN  size_t      BytesPerValue,
    IN  size_t      Flags
    )
  {
#ifndef UFSD_APFS_RO
    return SetEaInternal(Name, NameLen, Value, BytesPerValue, Flags, XATTR_DATA_TYPE_BINARY);
#else
    UNREFERENCED_PARAMETER(Name);
    UNREFERENCED_PARAMETER(NameLen);
    UNREFERENCED_PARAMETER(Value);
    UNREFERENCED_PARAMETER(BytesPerValue);
    UNREFERENCED_PARAMETER(Flags);
    return ERR_WPROTECT;
#endif
  }

  //Get inode id on volume
  UINT64 GetInodeId() const { return APFS_GET_INODE_ID(m_Id); }

  //Get id for external use (actual for subvolumes root dirs)
  UINT64 GetExternalId() const
  {
    return IsSubVolumeRootInode() ? APFS_MAKE_INODE_ID(m_VolId, APFS_VOLUMES_DIR_ID) : m_Id;
  }

  //Get tree id (index of subvolume where inode placed)
  unsigned char GetTreeId() const { return m_VolId; }

  CApfsTree* GetTree() const
  {
    return m_pVol ? m_pVol->GetObjectTree() : NULL;
  }

private:
  int ReadData(
      IN  UINT64    Offset,
      OUT size_t*   OutLen,
      OUT void*     pBuffer,
      IN  size_t    Size,
      IN  bool      bFork = false
      );

  bool IsClonedInternal(UINT64 ExtentId) const;

  int EnumerateVolumeExtents(
      IN  UINT64 ExtentId,
      IN  bool   bDelete,
      OUT bool*  pHasClonedExtents = NULL
      ) const;

  int GetSparseSize();

  int GetExtent(
      IN  UINT64          Id,
      IN  UINT64          Offset,
      OUT CUnixExtent&    Extent
      ) const;

  int FindExtent(
      IN  UINT64          Id,
      IN  UINT64          Vcn,
      IN  size_t          Len,
      OUT CUnixExtent*    pOutExtent,
      OUT UINT64*         LcnBase,
      IN  bool            bAllocate
      );

  int GetExtentId(
      OUT UINT64*  Id,
      IN  bool     bFork = false
      ) const
  {
    if (bFork)
    {
      InodeXAttr* xAttr;
      CHECK_CALL(GetXAttr(XATTR_FORK, XATTR_FORK_LEN, &xAttr));
      *Id = xAttr->m_Extent;
    }
    else
      *Id = m_pInode->ai_extent_id;

    return ERR_NOERROR;
  }

  bool HasEa() const;

  bool IsSubVolumeRootInode() const
  {
    return (GetInodeId() == APFS_ROOT_INO && m_VolId != 0);
  }

  UINT64 GetVolumeExtentId(
      IN UINT64 RootTreeExtentId
      ) const
  {
    return (RootTreeExtentId == m_pInode->ai_extent_id) ? GetInodeId() : RootTreeExtentId;
  }

  int GetSubVolumeInfo(
      IN  UINT64    Id,
      OUT FileInfo& Info
      ) const;

  CApfsTree* GetExtentTree() const
  {
    return m_pVol ? m_pVol->GetExtentTree() : NULL;
  }

  int GetXAttr(
      IN  const char*     Name,
      IN  unsigned short  NameLen,
      OUT InodeXAttr**    xAttr
      ) const;

  int GetXAttrData(
      IN  InodeXAttr* xAttr,
      IN  size_t      Off,
      IN  size_t      ReqLen,
      OUT void*       pBuffer,
      OUT size_t*     OutLen
      );

  //Help function for SetEa and GetEa
  int ReadWriteEaToExtent(
      IN UINT64           Id,
      IN OUT const void*  Value,
      IN size_t           BytesPerValue,
      IN bool             bWrite
      );

  int ReadCompressedData(
      IN  UINT64  Offset,
      OUT size_t* OutLen,
      IN  void*   pBuffer,
      IN  size_t  BufSize
      );

  int ReadInlineCompressedData(
      IN  api::ICompress* Compressor,
      IN  UINT64          Offset,
      OUT size_t*         OutLen,
      OUT void*           pBuffer,
      IN  size_t          Size
      ) const;

  int ReadResourceCompressedData(
      IN  api::ICompress* Compressor,
      IN  UINT64          off,
      OUT size_t*         OutLen,
      OUT void*           pBuffer,
      IN  size_t          Size
      );

  int ReadZlibBlockInfo(
      IN  InodeXAttr*             xData,
      OUT apfs_compressed_block** Blocks,
      OUT unsigned int*           Entries,
      OUT size_t*                 CmpBufSize
      );

  int ReadLZBlockInfo(
      IN  InodeXAttr*             xData,
      OUT apfs_compressed_block** Blocks,
      OUT unsigned int*           Entries,
      OUT size_t*                 CmpBufSize
      );

#ifndef UFSD_APFS_RO
  int WriteData(
      IN  UINT64    Offset,
      OUT size_t*   OutLen,
      IN  void*     pBuffer,
      IN  size_t    BufSize,
      IN  bool      bFork = false
      );

  virtual int SetForkSize(
      IN size_t Size
      );

  virtual int SetSizeValue(
      IN UINT64 Size
      );

  int SetInodeName(
      IN unsigned char* Name,
      IN unsigned short NameLen
      );

#ifdef UFSD_APFS_SIEVE
  int AddExtentToFileEnd(
      IN UINT64 Lcn
      );
#endif

  int SetEaInternal(
    IN  const char*    Name,
    IN  size_t         NameLen,
    IN  const void*    Value,
    IN  size_t         BytesPerValue,
    IN  size_t         Flags,
    IN  unsigned short xdType
  );

  virtual int DeCloneExtents();
  virtual int DecompressExtents();

  int DeCloneExtentsInternal(
      IN UINT64 ExtentId
      );

  int SetSparsed();

  int TraceExtents(
      IN UINT64 Id
      ) const;

  int AddExtent(
      IN UINT64 Id,
      IN UINT64 Vcn,
      IN UINT64 Lcn,
      IN size_t Len
      );

  int FlushExtent(
      IN CUnixExtent& Extent
      ) const;

  int RemoveExtent(
      IN UINT64 Id
      );

  int UpdateExtent(
      IN const CUnixExtent& Extent
      );

  int RemoveExtent(
      IN const CUnixExtent& Extent
      );

  int SplitSparseExtent(
      IN  UINT64        Vcn,
      IN  size_t        Len,
      IN  CUnixExtent&  Extent,
      OUT CUnixExtent*  pOutExtent
      );

  int AllocateExtent(
      IN  UINT64        Id,
      IN  UINT64        Vcn,
      IN  size_t        Len,
      IN  CUnixExtent&  Extent,
      OUT CUnixExtent*  pOutExtent,
      OUT UINT64*       LcnBase
      );

  int CreateExtent(
      IN  UINT64        Id,
      IN  UINT64        Vcn,
      IN  size_t        Len,
      OUT CUnixExtent*  pOutExtent,
      OUT UINT64*       LcnBase
      );

  int AddInodeField(
      IN  const apfs_inode_field& f,
      OUT void**                  data = NULL
      );

  int CreateInodeSizeField();

  int CreateSparseSizeField(
      OUT void** Field
      );

  int DeleteInodeField(
      IN unsigned int type
      );

  int CreateDeviceField(
      IN unsigned int DevNum
      );

  int SplitVolumeExtent(
      IN apfs_volume_extent_key*  vek,
      IN apfs_volume_extent_data* ved,
      IN UINT64   RootTreeExtentStart,
      IN UINT64   RootTreeExtentEnd,
      IN UINT64   VolumeExtentStart,
      IN UINT64   VolumeExtentEnd
      ) const;

  //Help function for truncate
  int TruncateInternal(
      IN UINT64 Id,
      IN UINT64 Vcn
      );

public:
  int Unlink();
#else
  virtual int SetSizeValue(UINT64 /*Size*/) { return ERR_WPROTECT; }
#endif
};

} // namespace apfs

} // namespace UFSD

#endif
