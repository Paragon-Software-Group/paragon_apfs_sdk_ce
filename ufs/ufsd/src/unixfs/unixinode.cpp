// <copyright file="unixinode.cpp" company="Paragon Software Group">
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
//     30-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#if defined UFSD_BTRFS | defined UFSD_EXTFS2 | defined UFSD_XFS | defined UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331714 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"

#include "unixsuperblock.h"
#include "unixinode.h"

namespace UFSD
{

CUnixInode::CUnixInode(IN api::IBaseMemoryManager* Mm)
  : UMemBased<CUnixInode>(Mm)
  , m_Id(0)
  , m_NfsId(0)
  , m_bDirty(false)
  , m_bDelOnClose(false)
  , m_RefCount(0)
  , m_bSizeValid(true)
  , m_SizeInBytes(0)
  , m_Strings(NULL)
  , m_pSuper(NULL)
  , m_bHaveChanges(false)
{}


/////////////////////////////////////////////////////////////////////////////
api::IBaseLog* CUnixInode::GetLog() const
{
  return m_pSuper->GetLog();
}


/////////////////////////////////////////////////////////////////////////////
UINT64 CUnixInode::GetAllocatedSize(bool bFork) const
{
  return GetAllocatedBlocks(bFork) << m_pSuper->m_Log2OfCluster;
}


/////////////////////////////////////////////////////////////////////////////
void CUnixInode::Release()
{
  --m_RefCount;

  if (m_RefCount == 0)
  {
    m_pSuper->m_InodeCache.remove(&m_TreeEntry);

    delete this;
  }
}


/////////////////////////////////////////////////////////////////////////////
int
CUnixInode::LoadBlocks(
  IN  UINT64        Vcn,
  IN  size_t        Len,
  OUT CUnixExtent*  pOutExtent,
  IN  bool          bFork,
  IN  bool          Allocate,
  OUT UINT64*       VcnBase
)
{
  return LoadBlocks(Vcn, Len, &pOutExtent->Lcn, &pOutExtent->Len, bFork, Allocate, &pOutExtent->IsAllocated, VcnBase);
}

} // namespace UFSD

#endif
