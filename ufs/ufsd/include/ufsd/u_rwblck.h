// <copyright file="u_rwblck.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File rwblck.h
 *
 *  Here is the definition of CRWBlock class
 *  for low level block read/write operations.
 ****************************************************/


// Possible options for Map
#define UFSD_RW_MAP_NO_READ       0x01
//#define UFSD_RW_MAP_FIXUP         0x02

struct buffer_head;

namespace UFSD {

struct BcbBuf{
  buffer_head* Bcb;
  void* Buf;
};

#ifdef UFSD_DRIVER_LINUX

#define RWB_DRIVER_OBJECT     0xD0D0D0

class IDriverRWBlock : public api::IDeviceRWBlock
{
public:
  virtual int Map( IN const UINT64&  Offset,
                   IN size_t         Bytes,
                   IN size_t         Flags,      // - one of the UFSD_RW_MAP_XXX
                   OUT buffer_head**  Bcb,
                   OUT void**        Mem) = 0;

  virtual void UnMap(
      IN buffer_head*  bh,
      IN bool   Forget = false) = 0;

  virtual int SetDirty(
      IN buffer_head*  bh,
      IN size_t  Wait = 0
      ) = 0;

  virtual void LockBuffer(
      IN buffer_head*  bh
      ) = 0;

  virtual void UnLockBuffer(
      IN buffer_head*  bh
      ) = 0;
};

#endif  // UFSD_DRIVER_LINUX

} // namespace UFSD

