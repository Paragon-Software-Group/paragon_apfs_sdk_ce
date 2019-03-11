// <copyright file="apfscompr.h" company="Paragon Software Group">
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
//     17-November-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __APFS_COMPR_H
#define __APFS_COMPR_H

#include <api/compress.hpp>

namespace UFSD
{

namespace apfs
{

class UCompressFactory : public UMemBased<UCompressFactory>, public api::ICompressFactory
{
  api::IBaseLog*      m_Log;
public:
  UCompressFactory(api::IBaseMemoryManager* Mm, api::IBaseLog* Log)
    : UMemBased<UCompressFactory>(Mm)
    , m_Log(Log)
  {}

  api::IBaseLog* GetLog() const { return m_Log; }

  virtual ~UCompressFactory() {}

  virtual int CreateProvider(unsigned int Method, api::ICompress** pICompress);

  virtual void Destroy() {}
};

} // namespace apfs

} // namespace UFSD

#endif // __APFS_COMPR_H
