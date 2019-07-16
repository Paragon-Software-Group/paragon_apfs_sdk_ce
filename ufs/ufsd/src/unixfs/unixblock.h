// <copyright file="unixblock.h" company="Paragon Software Group">
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
//     08-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __UFSD_UNIX_BLOCK_H
#define __UFSD_UNIX_BLOCK_H

#include "unixsuperblock.h"

namespace UFSD
{

//=================================================================
class CUnixBlock : public UMemBased<CUnixBlock>
{
  void*             m_pBuffer;        //Block data

  bool              m_bDirty;         //block needs to flush on disk
  unsigned char     m_VolIndex;       //Number of encrypted volume. m_VolIndex = BLOCK_BELONGS_TO_CONTAINER if block is not encrypted
  unsigned int      m_ReffCounter;    //number of usages of block

protected:
  CUnixSuperBlock*  m_pSuper;
  bool              m_bCalcCrc;

public:
  struct list_head  m_Entry;          //Entry for m_BlocksList
  struct list_head  m_RankEntry;      //Entry for m_BlocksRankList
  avl_link64        m_TreeEntry;      //Entry of tree

  CUnixBlock(api::IBaseMemoryManager* mm);

  //read block number Block from disk if fCreate = false or fill block buffer with zeroes if fCreate = false
  int Init(
    IN CUnixSuperBlock* Super,
    IN UINT64           Block,
    IN bool             fCreate   = false,
    IN unsigned int     BlockSize = 0,
    IN unsigned char    VolIndex  = BLOCK_BELONGS_TO_CONTAINER,
    IN bool             bCalcCrc  = false
  );

  int Dtor();

  UINT64 Id() const { return m_TreeEntry.key; }
  void Destroy() const { delete this; }

  int Release();
  virtual int Flush();

  void* GetBuffer() const { return m_pBuffer; }

  bool IsDirty() const { return m_bDirty; }
  virtual void SetDirty(bool bDirty = true) { m_bDirty = bDirty; }

  unsigned int GetReffCounter() const { return m_ReffCounter; }
  void IncReffCount(unsigned short N = 1) { m_ReffCounter += N; }

  virtual ~CUnixBlock() { Dtor(); }

  api::IBaseLog* GetLog() const { return m_pSuper->GetLog(); }
};


//=================================================================
//Smart pointer to CUnixBlock
class CUnixSmartBlockPtr
{
  CUnixBlock* m_pBlock;

public:
  CUnixSmartBlockPtr()
    : m_pBlock(NULL) {}

  ~CUnixSmartBlockPtr()
  {
    if (m_pBlock)
      m_pBlock->Release();
  }

  int Init(
    IN CUnixSuperBlock* pSuper,
    IN UINT64           Block,
    IN bool             fCreate   = false,
    IN unsigned int     BlockSize = 0,
    IN unsigned char    VolId     = BLOCK_BELONGS_TO_CONTAINER,
    IN bool             bCalcCrc  = false
  );
  bool IsInited() const { return m_pBlock != NULL; }
  int Close();

  void* GetBuffer() const { return m_pBlock == NULL ? NULL : m_pBlock->GetBuffer(); }
  UINT64 GetBlockNum() const { return m_pBlock == NULL ? 0 : m_pBlock->Id(); }
  void SetDirty(bool bDirty = true) const;
  bool IsDirty() const;
  int Flush() const;

  //get log object
  api::IBaseLog* GetLog() const
  {
    return m_pBlock ? m_pBlock->GetLog() : NULL;
  }
};

};

#endif   // __UFSD_UNIX_BLOCK_H
