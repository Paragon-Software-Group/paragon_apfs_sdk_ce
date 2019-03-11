// <copyright file="unixfs.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
// Revision History:
//
//     07-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////

#include "../h/versions.h"

#if defined UFSD_BTRFS || defined UFSD_EXTFS2 || defined UFSD_XFS || defined UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331766 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"

#include "unixfs.h"
#include "unixdir.h"

namespace UFSD
{

/////////////////////////////////////////////////////////////////////////////
CUnixFileSystem::CUnixFileSystem(
  IN api::IBaseMemoryManager* mm,
  IN api::IBaseStringManager* ss,
  IN api::ITime*              tt,
  IN api::IBaseLog*           log
  )
  : CFileSystem(mm, ss, tt, log)
  , m_umask(022)
  , m_pSuper(NULL)
  , m_NfsId(0)
{
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFileSystem::Dtor()
{
  int Status = ERR_NOERROR;

  if (m_pSuper)
  {
    Status = Flush();

    CloseAll();
#ifndef UFSD_DRIVER_LINUX
    Free2(m_Rws);
#endif
    delete m_pSuper;
  }

  ULOG_TRACE(( GetVcbLog( this ), "~CUnixFileSystem -> %x", Status));

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFileSystem::Init(
  IN  api::IDeviceRWBlock** rw,
  IN  unsigned int          RwBlockCount,
  IN  size_t                Options,
  OUT size_t*               Flags,
  IN const PreInitParams*   //Params
  )
{
  if (!rw)
    return ERR_BADPARAMS;

  m_Options = Options;
#ifndef UFSD_DRIVER_LINUX
  m_Rw = *rw;
  m_RwBlocksCount = RwBlockCount;

  if (m_Rws != rw)
  {
    Free2(m_Rws);
    size_t  bufsize = sizeof(api::IDeviceRWBlock*) * RwBlockCount;
    CHECK_PTR(m_Rws = reinterpret_cast<api::IDeviceRWBlock**>(Malloc2(bufsize)));
    Memcpy2(m_Rws, rw, bufsize);
  }
#else
  m_Rw = (IDriverRWBlock*)*rw;
#endif

  if (!m_Rw)
    return ERR_BADPARAMS;

  if (Flags)
    *Flags = 0;

  CHECK_CALL_SILENT(InitFS(Flags));

  UINT64 rwSize = m_Rw->GetNumberOfBytes();
  UINT64 fsSize = GetBlockSize() * GetBlocksCount();

  if (rwSize < fsSize)
  {
    ULOG_WARNING((GetVcbLog( this ), "CUnixFs::Init Rw block size is less than filesystem (%" PLL "u < %" PLL "u)", rwSize, fsSize));
    if (!FlagOn(m_Options, UFSD_OPTIONS_IGNORE_RWBLOCK_SIZE))
      return ERR_FSUNKNOWN;
  }

  DRIVER_ONLY(SetupBlocks());

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFileSystem::ReInit(
  IN  size_t  Options,
  OUT size_t* Flags,
  IN const PreInitParams*
  )
{
  ULOG_MESSAGE(( GetVcbLog( this ), "CUnixFs::ReInit %s", IsReadOnly() ? "ro" : "rw"));

  if (Flags)
    *Flags = 0;

  if (!IsReadOnly())
    CHECK_CALL(Flush());

#ifndef UFSD_DRIVER_LINUX
  return Init(m_Rws, m_RwBlocksCount, Options, Flags);
#else
  return Init((api::IDeviceRWBlock**)(&m_Rw), 1, Options, Flags);
#endif
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFileSystem::GetVolumeInfo(
    OUT UINT64*       FreeClusters,
    OUT UINT64*       TotalClusters,
    OUT size_t*       BytesPerCluster,
    OUT void*         VolumeSerial,
    IN  size_t        BytesPerVolumeSerial,
    OUT size_t*       RealBytesPerSerial,
    IN  api::STRType  VolumeNameType,
    OUT void*         VolumeNameBuffer,
    IN  size_t        VolumeNameBufferLength,
    OUT VolumeState*  VolState,
    OUT size_t*       BytesPerSector
    )
{
  //  ULOG_TRACE((m_Log, "CUnixFileSystem::GetVolumeInfo"));

  if ( FreeClusters )
    * FreeClusters = GetFreeBlocksCount();
  if ( TotalClusters )
    * TotalClusters = GetBlocksCount();
  if ( BytesPerCluster )
    * BytesPerCluster = GetBlockSize();
  if ( VolState )
    * VolState = IsFsDirty() ? Dirty : Clean;
  if ( BytesPerSector )
    * BytesPerSector = m_Rw->GetSectorSize();

  size_t SerialBytes;
  const unsigned char* VolSerial = GetVolumeSerial( &SerialBytes );

  if ( VolumeSerial )
  {
    if ( BytesPerVolumeSerial < SerialBytes )
      return ERR_INSUFFICIENT_BUFFER;
    Memcpy2( VolumeSerial, VolSerial, SerialBytes );
  }

  if ( RealBytesPerSerial )
    * RealBytesPerSerial = SerialBytes;

  if ( VolumeNameBuffer && (VolumeNameBufferLength > 0) )
  {
    size_t len;
    const char* name = GetVolumeName( &len );

    if ( len >= VolumeNameBufferLength )
      len = VolumeNameBufferLength - 1;

    UFSD_SUCCESS( m_Strings->Convert( api::StrUTF8, name, len, VolumeNameType, VolumeNameBuffer, VolumeNameBufferLength, NULL ) );

    unsigned CharSize = api::IBaseStringManager::GetCharSize( VolumeNameType );
    if ( CharSize == 1 )
      * (static_cast<char*>(VolumeNameBuffer) + len) = 0;
    else if ( CharSize == 2 )
      * (static_cast<unsigned short*>(VolumeNameBuffer) + len) = 0;
  }

#ifdef UFSD_DRIVER_LINUX
  ULOG_INFO( (GetVcbLog( this ), "===== VolumeInfo =====") );
  ULOG_INFO( (GetVcbLog( this ), "Total blocks: %#" PLL "x", GetBlocksCount()) );
  ULOG_INFO( (GetVcbLog( this ), "Free blocks : %#" PLL "x", GetFreeBlocksCount()) );
  ULOG_INFO( (GetVcbLog( this ), "Block size  : %#x", GetBlockSize()) );
  ULOG_INFO( (GetVcbLog( this ), "Sector size : %#x", m_Rw->GetSectorSize()) );
  ULOG_INFO( (GetVcbLog( this ), "======================") );
#endif

  return ERR_NOERROR;
}


#if (defined UFSD_APFS && defined UFSD_APFS_RO) && !defined UFSD_BTRFS && !defined UFSD_EXTFS2 && !defined UFSD_XFS
/////////////////////////////////////////////////////////////////////////////
int CUnixFileSystem::Flush(
  IN bool
  )
{
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CUnixFileSystem::SetVolumeInfo(
  IN const void*    ,
  IN size_t         ,
  IN api::STRType   ,
  IN const void*    ,
  IN VolumeState
)
{
  return ERR_NOTIMPLEMENTED;
}

int CUnixFileSystem::IoControl(
    IN  size_t      ,
    IN  const void* ,
    IN  size_t      ,
    OUT void*       ,
    IN  size_t      ,
    OUT size_t*
    )
{
  return ERR_NOTIMPLEMENTED;
}

int CUnixFileSystem::OnGetRetrievalPointers() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetVolumeBitmap() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetVolumeLcn2Lbo() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetBytesPerVolume() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetVolumeLbo2Lcn() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetSizes() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetFsType() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetFsVersion() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetDirty() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnSetDirty() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnClearDirty() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetVolumeInfo() { return ERR_NOTIMPLEMENTED; }
int CUnixFileSystem::OnGetAttributes() { return ERR_NOTIMPLEMENTED; }
#endif  // UFSD_APFS_RO

}

#endif
