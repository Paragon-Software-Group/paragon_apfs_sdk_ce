// <copyright file="apfscompr.cpp" company="Paragon Software Group">
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
//     20-November-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 330721 $";
#endif

#include <ufsd.h>

#include "../h/uerrors.h"
#include "../h/assert.h"

#ifdef UFSD_ZLIB
#include "../zlib/uzlib.h"
#endif
#ifdef UFSD_LZFSE
#include "../lzfse/ulzfse.h"
#endif

#include "apfscompr.h"

namespace UFSD
{

namespace apfs
{

int UCompressFactory::CreateProvider(unsigned int Method, api::ICompress** pICompress)
{
  if (NULL == pICompress)
    return ERR_BADPARAMS;

#ifdef UFSD_ZLIB
  if (Method == I_COMPRESS_DEFLATE)
  {
    CHECK_PTR(*pICompress = new(m_Mm) CZlibCompressor(m_Mm));
    return ERR_NOERROR;
  }
#endif
#ifdef UFSD_LZFSE
  if (Method == I_COMPRESS_LZFSE)
  {
    CHECK_PTR(*pICompress = new(m_Mm) CLzfseCompression(m_Mm, m_Log));
    return ERR_NOERROR;
  }
#endif

#if !defined UFSD_ZLIB && !defined UFSD_LZFSE
   UNREFERENCED_PARAMETER(Method);
#endif

  *pICompress = NULL;
  return ERR_NOTIMPLEMENTED;
}

} // namespace apfs
} // namespace UFSD

#endif // UFSD_APFS
