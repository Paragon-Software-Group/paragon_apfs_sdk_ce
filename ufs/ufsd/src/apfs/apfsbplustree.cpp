// <copyright file="apfsbplustree.cpp" company="Paragon Software Group">
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
//     17-October-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////

#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331828 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/assert.h"
#include "../h/uswap.h"
#include "../h/uavl.h"
#include "../h/uerrors.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfssuper.h"
#include "apfs.h"  // m_Sb
#include "apfstable.h"
#include "apfsbplustree.h"
#ifdef BASE_BIGENDIAN
#include "apfsbe.h"
#endif

namespace UFSD
{

namespace apfs
{

/////////////////////////////////////////////////////////////////////////////
CApfsTreeNode::CApfsTreeNode(CApfsTree* pTree, CApfsTree* pLocationTree, bool bEnumerator)
  : UMemBased<CApfsTreeNode>(pTree->m_Mm)
  , m_Parent(NULL)
  , m_pChild(NULL)
  , m_pLocationTree(pLocationTree)
  , m_pTree(pTree)
  , m_Pos(0)
  , m_bEnumerator(bEnumerator)
{
  m_pTable = new(m_Mm) CApfsTable(m_pTree);
  assert(m_pTable);
}


/////////////////////////////////////////////////////////////////////////////
CApfsTreeNode::~CApfsTreeNode()
{
  delete m_pTable;
  delete m_pChild;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeNode::Init(CApfsTreeNode *Parent, unsigned int Pos)
{
  CHECK_PTR(m_pTable);
  if (!Parent || Pos >= Parent->Count())
    return ERR_BADPARAMS;

  m_Parent = Parent;
  m_Pos = Pos;
  UINT64 *ObjectId = NULL;
  CHECK_CALL(m_Parent->GetItem(Pos, NULL, reinterpret_cast<void**>(&ObjectId)));

  if (!IsLocationTree())
  {
    //Search block number for underlying table in location tree
    apfs_location_table_data CurLocation;
    Memzero2( &CurLocation, sizeof( apfs_location_table_data ) );
    CHECK_CALL(m_pLocationTree->GetActualLocation(CPU2LE(*ObjectId), NULL, &CurLocation));

    if ( CurLocation.ltd_block == 0 )
      return ERR_NOTFOUND;

    //Init table with found block
    CHECK_CALL(m_pTable->Init(CurLocation.ltd_block, Parent->m_pTable->GetVolumeIndex(), Parent->m_pTable->MayBeEncrypted()));
  }
  else
  {
    UINT64 BlockNumber;
    if (m_Parent->m_pTable->GetContentType() == APFS_CONTENT_HISTORY)
    {
      unsigned int TreeBlockSize = 0;
      CHECK_CALL(m_pTable->GetSuper()->GetMetaLocationBlock(*ObjectId, APFS_TYPE_NODE_BLOCK, &BlockNumber, &TreeBlockSize));    //using superblock map to find block number
      if (TreeBlockSize != m_pTable->GetSuper()->GetBlockSize())
      {
        ULOG_ERROR((GetLog(), ERR_NOTIMPLEMENTED, "Block sizes for tree(%x) and for superblock (%x) are different", TreeBlockSize, m_pTable->GetSuper()->GetBlockSize()));
        return ERR_NOTIMPLEMENTED;
      }
    }
    else
      BlockNumber = CPU2LE(*ObjectId);
    CHECK_CALL(m_pTable->Init(BlockNumber, Parent->m_pTable->GetVolumeIndex(), Parent->m_pTable->MayBeEncrypted()));
  }

  //Store table buffer only for nodes of location tree and for root node (because they are not copied on tree enumerator creation)
  //For other nodes unload table buffer and load it from cache or from disk if necessary
  if (NeedUnloadBuffer())
    CHECK_CALL(m_pTable->UnloadBuffer());

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeNode::ReInit(CApfsTreeNode* Parent, unsigned int Pos)
{
  if (m_pTable->IsBufferLoaded())
    CHECK_CALL(m_pTable->UnloadBuffer());
  return Init(Parent, Pos);
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeNode::InitAsRoot(UINT64 RootBlock, unsigned char VolIndex, bool bMayBeEncrypted)
{
  m_Parent = NULL;
  m_Pos = INDEX_NOT_FOUND;

  CHECK_PTR(m_pTable);
  CHECK_CALL( m_pTable->Init(RootBlock, VolIndex, bMayBeEncrypted) );

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeNode::LoadChild(unsigned int ChildIndex)
{
//  ULOG_DEBUG1( (GetLog(), "LoadChild %x", ChildIndex) );

  if (m_pChild)
    CHECK_CALL(m_pChild->ReInit(this, ChildIndex));
  else
  {
    CHECK_PTR(m_pChild = new(m_Mm) CApfsTreeNode(m_pTree, m_pLocationTree, m_bEnumerator));
    CHECK_CALL(m_pChild->Init(this, ChildIndex));
  }

//  ULOG_DEBUG1( (GetLog(), "LoadChild -> block=%" PLL "x, level=%x", m_pChild->m_pTable->GetBlockNumber(), m_pChild->Depth()) );

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
CApfsTreeInternal::CApfsTreeInternal(CApfsSuperBlock *sb)
  : UMemBased<CApfsTreeInternal>(sb->m_Mm)
  , m_KeyFactory(NULL)
  , m_pLocationTree(NULL)
  , m_pRootNode(NULL)
  , m_pCurNode(NULL)
  , m_PosInCurNode(0)
  , m_pSuper(sb)
  , m_bCurNodeValid(false)
  , m_TempKey1(NULL)
  , m_TempKey2(NULL)
{
}


/////////////////////////////////////////////////////////////////////////////
CApfsTreeInternal::~CApfsTreeInternal()
{
  delete m_TempKey1;
  delete m_TempKey2;
  delete m_KeyFactory;
  delete m_pRootNode;
  m_pRootNode = NULL;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeInternal::Init(UINT64 BlockNumber, unsigned char VolIndex, CApfsTree* pLocationTree, CApfsTree* pTree, bool bMayBeEncrypted)
{
  if (m_KeyFactory == NULL )
    CHECK_PTR(m_KeyFactory = new(m_Mm) CKeyFactory( m_Mm ));

  m_pLocationTree = pLocationTree;

  if (m_pRootNode == NULL)
    CHECK_PTR(m_pRootNode = new(m_Mm) CApfsTreeNode(pTree, m_pLocationTree, IsEnumerator()));
  else
  {
    delete m_pRootNode->m_pChild;
    m_pRootNode->m_pChild = NULL;
  }
  CHECK_CALL(m_pRootNode->InitAsRoot(BlockNumber, VolIndex, bMayBeEncrypted));
  m_pCurNode = m_pRootNode;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
UINT64
CApfsTreeInternal::Count() const
{
  if (m_pRootNode->Depth() == 0)
    return m_pRootNode->Count();

  apfs_table_footer* pFooter = m_pRootNode->m_pTable->GetFooter();
  if (pFooter)
    return pFooter->tf_count;

  assert(0);           //unexpected situation
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeInternal::GetData(
  IN const CCommonSearchKey*  SearchKey,
  IN unsigned short           SearchRegime,
  OUT void**                  Data,
  OUT void**                  KeyRecord
  )
{
  CHECK_CALL_SILENT(FindPosition(SearchKey, SearchRegime));

  if (IS_INDEX_NOT_FOUND(m_PosInCurNode))
    return ERR_NOTFOUND;
  CHECK_CALL(m_pCurNode->GetItem(m_PosInCurNode, KeyRecord, Data));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeInternal::GetCurNode(
  IN const CCommonSearchKey* pSearchKey,
  IN unsigned short SearchRegime
  )
{
  //Search leaf from root
  int Status = ERR_NOERROR;
  m_pCurNode = m_pRootNode;

  while (m_pCurNode->Depth() > 0)
  {
//    ULOG_DEBUG1((GetLog(), "GetCurNode %" PLL "x: block=%" PLL "x, level=%x", pSearchKey->GetId(),
//                   m_pCurNode->m_pTable->GetBlockNumber(), m_pCurNode->Depth()));

    CHECK_CALL_EXIT(m_pCurNode->FindDataIndex(pSearchKey, SearchRegime, &m_PosInCurNode));

    if (IS_INDEX_NOT_FOUND(m_PosInCurNode))
    {
      Status = ERR_NOTFOUND;
      goto Exit;
    }
    CHECK_CALL_EXIT(m_pCurNode->LoadChild(m_PosInCurNode));
    m_pCurNode = m_pCurNode->m_pChild;
  }

  assert(m_pCurNode->Depth() == 0);

Exit:
  if (m_pCurNode->m_pChild)
  {
    delete m_pCurNode->m_pChild;
    m_pCurNode->m_pChild = NULL;
  }

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeInternal::FindFirstLeaf()
{
  //Search from root
  m_pCurNode = m_pRootNode;
  if (m_pRootNode->Depth() == 0)
    return ERR_NOERROR;

  //Delete current branch of nodes
  if (m_pRootNode->m_pChild != NULL)
  {
    delete m_pRootNode->m_pChild;
    m_pRootNode->m_pChild = NULL;
  }

  //Find first leaf
  while (m_pCurNode->Depth() != 0)
  {
    CHECK_CALL(m_pCurNode->LoadChild(0));
    m_pCurNode = m_pCurNode->m_pChild;
    assert(m_pCurNode->Count() > 0);
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeInternal::FindNextLeaf(CApfsTreeNode** NextLeaf) const
{
  if (NextLeaf == NULL)
    return ERR_BADPARAMS;

  CApfsTreeNode* TempNode = m_pCurNode;
  while (TempNode->m_Parent != NULL)
  {
    if (TempNode->m_Pos < TempNode->m_Parent->Count() - 1)
    {
      //If we are not approaching last leaf in node, load next leaf in this node
      CApfsTreeNode* Parent = TempNode->m_Parent;
      CHECK_CALL(Parent->LoadChild(TempNode->m_Pos + 1));
      TempNode = Parent->m_pChild;
      break;
    }

    TempNode = TempNode->m_Parent;    //Go to parent node to load another tree branch
  }

  if (TempNode->m_Parent == NULL)
    return ERR_NOTFOUND;

  //load 1st leaf in this branch
  while (TempNode->Depth() > 0)
  {
    CHECK_CALL(TempNode->LoadChild(0));
    TempNode = TempNode->m_pChild;
    assert(TempNode->Count() > 0);
  }
  *NextLeaf = TempNode;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeInternal::FindPosition(const CCommonSearchKey* pSearchKey, unsigned short SearchRegime)
{
//  ULOG_DEBUG1( (GetLog(), "FindPosition %" PLL "x...", pSearchKey->GetId()) );

  if (FlagOn(SearchRegime, SEARCH_KEY_GE))
    return ERR_NOTIMPLEMENTED;               //not needed yet

#ifndef NDEBUG
  if (m_bCurNodeValid && m_pCurNode->m_Parent)
    assert(m_pCurNode->m_Pos < m_pCurNode->m_Parent->Count());
#endif

  //Try to find item in current node
  if (m_pCurNode->Depth() == 0 && (m_bCurNodeValid || m_pRootNode->Depth() == 0))
    CHECK_CALL(m_pCurNode->FindDataIndex(pSearchKey, SearchRegime, &m_PosInCurNode));
  else
    m_PosInCurNode = INDEX_NOT_FOUND;

  if (IS_INDEX_NOT_FOUND(m_PosInCurNode) || (m_PosInCurNode + 1 == m_pCurNode->Count() && NeedSearchFromRoot(pSearchKey, SearchRegime)))
  {
    //Item in current node not found or founded item is last item in the list
    if (m_pRootNode->Depth() == 0)
      return ERR_NOERROR;

//    ULOG_DEBUG1( (GetLog(), "FindPosition %" PLL "x: New search from root", pSearchKey->GetId()) );

    //Search m_pCurNode from tree root
    CHECK_CALL_SILENT(GetCurNode(pSearchKey, SearchRegime));

    //Search item in leaf
    CHECK_CALL(m_pCurNode->FindDataIndex(pSearchKey, SearchRegime, &m_PosInCurNode));
    m_bCurNodeValid = true;
  }

//  ULOG_DEBUG1( (GetLog(), "FindPosition %" PLL "x --> %x", pSearchKey->GetId(), m_PosInCurNode) );

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
bool
CApfsTreeInternal::NeedSearchFromRoot(const CCommonSearchKey* pSearchKey, unsigned short SearchRegime) const
{
  CApfsTreeNode* p = m_pCurNode;
  unsigned int FoundIndex = 0;

  while (p->m_Parent != NULL)
  {
    if (p->m_Pos + 1 < p->m_Parent->Count())
    {
      if (p->m_Parent->FindDataIndex(pSearchKey, SearchRegime, &FoundIndex) != ERR_NOERROR)
        return true;
      return FoundIndex != p->m_Pos;
    }

    p = p->m_Parent;
  }
  return false;
}


/////////////////////////////////////////////////////////////////////////////
CApfsTree::CApfsTree(CApfsSuperBlock* sb)
  : CApfsTreeInternal(sb)
  , m_pRootDesc(NULL)
  , m_pDefaultEnum(NULL)
{
  m_EnumsList.init();
}


/////////////////////////////////////////////////////////////////////////////
CApfsTree::~CApfsTree()
{
  //if (GetRootTable() && GetRootTable()->GetFooter())
  //  ULOG_DEBUG1((GetLog(), "CApfsTree::Destroy: Level=0x%x, ItemsCount=0x%" PLL "x, MetaDataBlocksCount=%" PLL "x", GetLevel(), GetRootTable()->GetFooter()->tf_count, GetRootTable()->GetFooter()->tf_number_of_blocks));

  Free2(m_pRootDesc);
  delete m_pDefaultEnum;
  assert(m_EnumsList.is_empty());              //All enums for this tree is destroyed
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTree::Init(UINT64 BlockNumber, unsigned char VolIndex, bool bHasRootDesc, CApfsTree* pLocationTree, bool bMayBeEncrypted)
{
  if (bHasRootDesc)
  {
    //Read block number of root table in tree root descriptor
    if (m_pRootDesc == NULL)
      CHECK_PTR(m_pRootDesc = reinterpret_cast<apfs_btreed*>(Malloc2(sizeof(apfs_btreed))));
    CHECK_CALL(m_pSuper->ReadBytes(BlockNumber * m_pSuper->GetBlockSize(), m_pRootDesc, sizeof(apfs_btreed)));
    BE_ONLY(ConvertBTD(m_pRootDesc));

    if (m_pRootDesc->header.block_type != APFS_TYPE_BTREE)
    {
      UFSDTracek((m_pSuper->m_pFs->m_Sb, "Wrong tree block header (incorrect type)"));
      return ERR_NOFSINTEGRITY;
    }
    BlockNumber = m_pRootDesc->btree_root;
  }

  CHECK_CALL(CApfsTreeInternal::Init(BlockNumber, VolIndex, pLocationTree, this, bMayBeEncrypted));

  if (m_pDefaultEnum == NULL)
    CHECK_PTR(m_pDefaultEnum = new(m_Mm)CApfsTreeEnum(m_pSuper));
  CHECK_CALL(m_pDefaultEnum->Init(this));
  //ULOG_DEBUG1((GetLog(), "CApfsTree::Init: Level=0x%x, ItemsCount=0x%" PLL "x, MetaDataBlocksCount=%" PLL "x", GetLevel(), GetRootTable()->GetFooter()->tf_count, GetRootTable()->GetFooter()->tf_number_of_blocks));
  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTree::GetActualLocation(UINT64 Id, apfs_location_table_key* Key, apfs_location_table_data* Data) const
{
//  ULOG_DEBUG1( (GetLog(), "GetActualLocation %" PLL "x...", Id) );

  if (!IsLocationTree())
    return ERR_NOTIMPLEMENTED;           //this function is actual only for location tree

  apfs_location_table_data *pCurLocationData = NULL;
  apfs_location_table_key *pCurLocationKey = NULL;
  int Status = ERR_NOERROR;

  //Search location with max checkpoint (last location)
  //TODO: For full checkpoint support we should find max checkpoint which low or equal than volume superblock checkpoint id
  CSimpleSearchKey SearchKey(m_Mm, Id);
  CHECK_CALL(m_pDefaultEnum->StartEnumByKey(&SearchKey, SEARCH_KEY_EQ));

  while (Status == ERR_NOERROR)
  {
    Status = m_pDefaultEnum->EnumNextPtrByKey((void**)&pCurLocationKey, (void**)&pCurLocationData);
    if (Status == ERR_NOERROR)
    {
//      if ( pCurLocationData != NULL )
//        ULOG_DEBUG1( (GetLog(), "GetActualLocation %" PLL "x --> %" PLL "x", Id, pCurLocationData->ltd_block) );

      if (Data)
      {
        Memcpy2(Data, pCurLocationData, sizeof(apfs_location_table_data));
#ifdef BASE_BIGENDIAN
        SwapBytesInPlace(&Data->ltd_block);
        SwapBytesInPlace(&Data->ltd_length);
        SwapBytesInPlace(&Data->ltd_flags);
#endif
      }
      if (Key)
      {
        Memcpy2(Key, pCurLocationKey, sizeof(apfs_location_table_key));
#ifdef BASE_BIGENDIAN
        SwapBytesInPlace(&Key->ltk_id);
        SwapBytesInPlace(&Key->ltk_checkpoint);
#endif
      }
    }
    else if (Status != ERR_NOTFOUND)
      return Status;
  }

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTree::GetItem(UINT64 index, void** pKey, void** pData, unsigned short* KeyLen, unsigned short *DataLen)
{
  CHECK_CALL(FindFirstLeaf());
  if (index >= m_pCurNode->Count())                    //index not in first leaf
    return ERR_NOTIMPLEMENTED;                         //We will implement it later if required
  return m_pCurNode->GetItem(static_cast<unsigned int>(index), pKey, pData, KeyLen, DataLen);
}


/////////////////////////////////////////////////////////////////////////////
CApfsTreeEnum::CApfsTreeEnum(CApfsSuperBlock* pSuper)
  : CApfsTreeInternal(pSuper)
  , m_EnumRegime(SEARCH_KEY_LE)
  , m_EnumType(ApfsDefaultKey)
  , m_bCaseSensitive(true)
  , m_bValid(true)
  , m_bEnumStarted(false)
  , m_bNeedNextOnRestoreEnum(false)
  , m_bTreeEmpty(false)
{
  m_Entry.init();
  Memzero2( &m_EnumKey, sizeof( apfs_key ) );
  Memzero2( &m_CurKey, sizeof( apfs_key ) );
}


/////////////////////////////////////////////////////////////////////////////
CApfsTreeEnum::~CApfsTreeEnum()
{
  m_Entry.remove();
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeEnum::StartEnum()
{
  CHECK_CALL_SILENT(InitEnumerator());
  m_PosInCurNode = 0;
  return FindFirstLeaf();
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeEnum::InitEnumerator()
{
  if (!m_bValid || !m_bEnumStarted)
  {
    CHECK_CALL(m_pRootNode->m_pTable->ReInit());
    m_pCurNode = m_pRootNode;
  }

  if (m_pRootNode->Count() == 0)
    return ERR_NOTFOUND;

  m_bValid = true;
  m_bEnumStarted = true;

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeEnum::EnumNextPtr(void** pKey, void** pData, unsigned short* KeyLen, unsigned short *DataLen)
{
  if (!m_bValid)
    CHECK_CALL_SILENT(RestoreEnumerator());
  if (m_PosInCurNode == m_pCurNode->Count())
  {
    //We are approaching end of the leaf. We should search in next leaf or say that "not found"
    if (GetLevel() > 0)
    {
      CHECK_CALL_SILENT(FindNextLeaf(&m_pCurNode));
      m_PosInCurNode = 0;
    }
    else
      return ERR_NOTFOUND;
  }

  return m_pCurNode->GetItem(m_PosInCurNode++, pKey, pData, KeyLen, DataLen);
}


/////////////////////////////////////////////////////////////////////////////
int CApfsTreeEnum::EnumNextPtrByKey(void** Key, void** Data, unsigned short* KeyLen, unsigned short* DataLen)
{
  apfs_key* pFoundBuffer = NULL;

  CHECK_CALL( m_KeyFactory->CreateKey( m_EnumType, &m_EnumKey, &m_TempKey1 ) );
  CHECK_CALL( m_KeyFactory->CreateKey( m_EnumType, NULL, &m_TempKey2 ) );

  CCommonSearchKey* pEnumKey = m_TempKey1;
  CCommonSearchKey* pFoundKey = m_TempKey2;

  pEnumKey->m_bCaseSensitive = m_bCaseSensitive;
  pFoundKey->m_bCaseSensitive = m_bCaseSensitive;

  while ( true )
  {
    CHECK_CALL_SILENT( EnumNextPtr( (void**)&pFoundBuffer, Data, KeyLen, DataLen ) );
    pFoundKey->SetKey( pFoundBuffer );

    if ( (pFoundKey->GetType() == pEnumKey->GetType() || FlagOn( m_EnumRegime, SEARCH_ALL_TYPES )) &&
      ((FlagOn( m_EnumRegime, SEARCH_KEY_EQ ) && *pFoundKey == *pEnumKey)
        || (FlagOn( m_EnumRegime, SEARCH_KEY_LE ) && *pFoundKey <= *pEnumKey)
        || (FlagOn( m_EnumRegime, SEARCH_KEY_GE ) && *pEnumKey <= *pFoundKey)) )
    {
      if ( Key != NULL )
        *Key = pFoundBuffer;
      break;
    }
    if ( *pFoundKey > *pEnumKey &&
      (!FlagOn( m_EnumRegime, SEARCH_KEY_GE ) || (pFoundKey->GetType() != pEnumKey->GetType() && !FlagOn( m_EnumRegime, SEARCH_ALL_TYPES ))) )
      return ERR_NOTFOUND;
  }

  return ERR_NOERROR;
}

/////////////////////////////////////////////////////////////////////////////
int
CApfsTreeEnum::RestoreEnumerator()
{
  int Status = ERR_NOTFOUND;

  if (!m_bTreeEmpty)
  {
    CHECK_CALL(m_pRootNode->m_pTable->ReInit());
    assert(m_pRootNode->m_pChild == NULL);
    m_pCurNode = m_pRootNode;

    CCommonSearchKey* Key = NULL;
    CHECK_CALL(m_KeyFactory->CreateKey( m_EnumType, &m_CurKey, &Key ));

    Status = GetData(Key, SEARCH_KEY_LOW, NULL);
    delete Key;
  }

  if (Status == ERR_NOTFOUND)
  {
    CHECK_CALL(FindFirstLeaf());
    m_PosInCurNode = 0;
    m_bNeedNextOnRestoreEnum = false;
  }
  else
    CHECK_STATUS(Status);
  m_bValid = true;

  if (m_bNeedNextOnRestoreEnum)
    CHECK_CALL_SILENT(EnumNextPtr(NULL));

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CEntryTreeEnum::SetPosition(UINT64 Pos)
{
  if (Pos == m_Position)
    return ERR_NOERROR;

  ULOG_DEBUG1((GetLog(), "CEntryTreeEnum::SetPosition %" PLL "x -> %" PLL "x", m_Position, Pos));

  if (Pos < m_Position)
  {
    CHECK_CALL(StartEnum());
    m_Position = 0;
  }

  apfs_direntry_data* dummy;

  while (m_Position < Pos)
    CHECK_CALL(EnumNextEntry(NULL, &dummy));

  return ERR_NOERROR;
}


} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
