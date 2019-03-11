// <copyright file="apfstable.cpp" company="Paragon Software Group">
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
//     11-October-2017 - created.
//
/////////////////////////////////////////////////////////////////////////////


#include "../h/versions.h"

#ifdef UFSD_APFS

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331853 $";
#endif

#include <ufsd.h>

#include "../h/utrace.h"
#include "../h/ucommon.h"
#include "../h/uswap.h"
#include "../h/assert.h"
#include "../h/uerrors.h"
#include "../h/uavl.h"
#include "../h/bitfunc.h"
#include "../h/ubitmap.h"

#include "../unixfs/unixblock.h"

#include "apfs_struct.h"
#include "apfssuper.h"
#include "apfstable.h"
#include "apfsbplustree.h"


namespace UFSD
{

namespace apfs
{

/////////////////////////////////////////////////////////////////////////////
int
StrCompare(const unsigned char* str1, unsigned int len1, const unsigned char* str2, unsigned int len2)
{
  unsigned short len = static_cast<unsigned short>(MIN(len1, len2));
  assert(len != 0);

  for (unsigned short i = 0; i < len; i++)
  {
    if (str1[i] > str2[i])
      return i + 1;
    if (str1[i] < str2[i])
      return -i - 1;
  }
  return len1 - len2;
}


/////////////////////////////////////////////////////////////////////////////
CApfsTable::CApfsTable(CApfsTreeInternal *pTree)
    : UMemBased<CApfsTable>(pTree->m_Mm)
    , m_pTree(pTree)
    , m_pSuper(pTree->m_pSuper)
    , m_pTable(NULL)
    , m_pIndexArea(NULL)
    , m_pDataArea(NULL)
    , m_pFooter(NULL)
    , m_BlockNum(0)
    , m_Id(0)
    , m_CheckPointId(0)
    , m_Count(0)
    , m_Level(0)
    , m_ContentType(0)
    , m_bLenFixed(false)
    , m_bHasFooter(false)
    , m_bOrdered(false)
    , m_VolIndex(BLOCK_BELONGS_TO_CONTAINER)
{
}


/////////////////////////////////////////////////////////////////////////////
int CApfsTable::Init(
    IN UINT64         BlockNumber,
    IN unsigned char  VolumeIndex,
    IN bool           bMayBeEncrypted
    )
{
  CHECK_CALL(m_TableBlock.Init(m_pSuper, BlockNumber, false, 0, bMayBeEncrypted ? VolumeIndex : BLOCK_BELONGS_TO_CONTAINER, true));
  m_pTable = reinterpret_cast<apfs_table*>(m_TableBlock.GetBuffer());
  m_VolIndex = VolumeIndex;
  m_BlockNum = BlockNumber;
  return ReInit();
}


/////////////////////////////////////////////////////////////////////////////
int CApfsTable::ReInit()
{
  if (CPU2LE(m_pTable->table_header.flags) > APFS_TABLE_MASK)
    return ERR_NOTIMPLEMENTED;

  m_bLenFixed = ((CPU2LE(m_pTable->table_header.flags) & APFS_TABLE_FIXED_ENTRY_SIZE) != 0);
  m_bHasFooter = ((CPU2LE(m_pTable->table_header.flags) & APFS_TABLE_HAS_FOOTER) != 0);

  m_Id = CPU2LE(m_pTable->header.id);
  m_CheckPointId = CPU2LE(m_pTable->header.checkpoint_id);
  m_Count = CPU2LE(m_pTable->table_header.count);
  m_Level = CPU2LE(m_pTable->table_header.level);
  m_ContentType = CPU2LE(m_pTable->header.content_type);

  if (CPU2LE(m_pTable->header.block_type) != (m_bHasFooter ? APFS_TYPE_ROOT_NODE_BLOCK : APFS_TYPE_NODE_BLOCK))
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Wrong block type: BlockNum=0x%" PLL "x, Id=0x%" PLL "x, Checkpoint=0x%" PLL "x, BlockType=0x%x",
      m_BlockNum, m_pTable->header.id, m_pTable->header.checkpoint_id, m_pTable->header.block_type));
    return ERR_NOFSINTEGRITY;
  }

  if ((!FlagOn(m_pTable->table_header.flags, APFS_TABLE_LEAF_NODE) && m_Level == 0) || (FlagOn(m_pTable->table_header.flags, APFS_TABLE_LEAF_NODE) && m_Level != 0))
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Wrong table header flags: BlockNum=0x%" PLL "x, Id=0x%" PLL "x, Checkpoint=0x%" PLL "x, Level=0x%x, TableHeaderFlags=0x%x",
      m_BlockNum, m_pTable->header.id, m_pTable->header.checkpoint_id, m_Level, m_pTable->table_header.flags));
    return ERR_NOFSINTEGRITY;
  }

  if (FlagOn(m_pTable->header.flags, APFS_BLOCK_FLAG_POSITION) && m_Id != m_BlockNum)
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Id=0x%" PLL "x and BlockNum=0x%" PLL "x are different for BlockHeaderFlag=0x%x", m_Id, m_BlockNum, m_pTable->header.flags));
    return ERR_NOFSINTEGRITY;
  }

  //Checksum already checked on cache block reading

  if (m_pTable->header.checkpoint_id > m_pSuper->GetCheckPointId())
  {
    ULOG_ERROR((GetLog(), ERR_NOFSINTEGRITY, "Wrong checkpoint id: BlockNum=0x%" PLL "x, Id=0x%" PLL "x, Checkpoint=0x%" PLL "x", m_BlockNum, m_pTable->header.id, m_pTable->header.checkpoint_id));
    return ERR_NOFSINTEGRITY;
  }

  assert(m_pTable->table_header.unknown_0x08 == 0);

  InitTablePointers();

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsTable::LoadBuffer()
{
  CHECK_CALL(m_TableBlock.Init(m_pSuper, m_BlockNum, false, 0, MayBeEncrypted() ? m_VolIndex : BLOCK_BELONGS_TO_CONTAINER, true));
  m_pTable = reinterpret_cast<apfs_table*>(m_TableBlock.GetBuffer());
  InitTablePointers();
  assert(m_Count == m_pTable->table_header.count);

  return ERR_NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
int CApfsTable::UnloadBuffer()
{
  assert(m_TableBlock.IsInited());
  m_pTable = NULL;

  return m_TableBlock.Close();
}


/////////////////////////////////////////////////////////////////////////////
void CApfsTable::GetItem(
    IN  unsigned int    Index,
    OUT void**          Key,
    OUT void**          Data,
    OUT unsigned short* KeyLen,
    OUT unsigned short* DataLen
    ) const
{
  unsigned short key_offset = 0, key_len = 0, data_offset = 0, data_len = 0;
  GetOffsetsAndSizes(Index, &key_offset, &key_len, &data_offset, &data_len);

  //Fill output parameters using key_offset, key_len, data_offset, data_len
  if (Key)
    *Key = Add2Ptr(GetKeyArea(), key_offset);
  if (Data)
    *Data = data_offset != EMPTY_OFFSET ? GetDataItem(data_offset) : NULL;
  if (KeyLen)
    *KeyLen = key_len;
  if (DataLen)
    *DataLen = data_len;

#ifndef NDEBUG
  if (data_offset != EMPTY_OFFSET)
    assert(data_offset < m_pSuper->GetBlockSize() && data_len <= data_offset);
  assert(data_len < m_pSuper->GetBlockSize());
  assert(key_len <= m_pTable->table_header.key_area_size);
  assert(sizeof(apfs_block_header) + sizeof(apfs_table_header) + m_pTable->table_header.index_area_size + m_pTable->table_header.key_area_size + m_pTable->table_header.free_area_size + (m_bHasFooter ? sizeof(apfs_table_footer) : 0) <= m_pSuper->GetBlockSize());
#endif
}


/////////////////////////////////////////////////////////////////////////////
int CApfsTable::FindDataIndex(
    IN const CCommonSearchKey*  pSearchKey,
    IN unsigned short           SearchRegime
    )
{
  assert(!FlagOn(SearchRegime, SEARCH_KEY_GE));
  //Return INDEX_NOT_FOUND for empty leaves
  if (m_Count == 0)
    return FlagOn(SearchRegime, SEARCH_KEY_LOW) && m_bHasFooter ? FIRST_INDEX_ON_LOW_SEARCH : INDEX_NOT_FOUND;

  C_ASSERT(offsetof(apfs_table_fixed_item, key_offset) == 0);

  void* keys = GetKeyArea();

  //If SearchKey not in this leaf return INDEX_NOT_FOUND (co,pare with first key)
  unsigned short KeyOffset = CPU2LE( *(reinterpret_cast<unsigned short*>(m_pIndexArea)) );

  CHECK_CALL(m_pTree->GetKeyFactory()->CreateKey( pSearchKey->m_Type, (apfs_key*)Add2Ptr( keys, KeyOffset ), &m_pTree->m_TempKey1 ));
  CHECK_CALL(m_pTree->GetKeyFactory()->CreateKey( pSearchKey->m_Type, NULL, &m_pTree->m_TempKey2 ));

  CCommonSearchKey* pCurKey = m_pTree->m_TempKey1;
  CCommonSearchKey* pTempKey2 = m_pTree->m_TempKey2;

  pCurKey->m_bCaseSensitive = pSearchKey->m_bCaseSensitive;
  pTempKey2->m_bCaseSensitive = pSearchKey->m_bCaseSensitive;

  CCommonSearchKey* pFirstKey = pCurKey;

  if (*pFirstKey > *pSearchKey)
    return FIRST_INDEX_IS_GREATER;

  //Check situation if only one item in the leaf
  if (m_Count == 1)
  {
    if (((FlagOn(SearchRegime, SEARCH_KEY_LOW) || FlagOn(SearchRegime, SEARCH_KEY_LE) || m_Level != 0) && *pSearchKey > *pFirstKey) ||
      ((FlagOn(SearchRegime, SEARCH_KEY_LE) || FlagOn(SearchRegime, SEARCH_KEY_EQ)) && *pFirstKey == *pSearchKey))
      return 0;
    if (FlagOn(SearchRegime, SEARCH_KEY_LOW) && *pFirstKey == *pSearchKey)
      return FIRST_INDEX_ON_LOW_SEARCH;
    return INDEX_NOT_FOUND;
  }

  //Binary search cycle
  unsigned int first = 0, last = m_Count - 1;
  while (first <= last)
  {
    const unsigned int mid = first + ((last - first) >> 1);
    unsigned short NextKeyOffset;

    //Find KeyOffset, NextKeyOffset for different item length types
    if (m_bLenFixed)
    {
      apfs_table_fixed_item* offsets = reinterpret_cast<apfs_table_fixed_item*>(m_pIndexArea);
      KeyOffset = CPU2LE(offsets[mid].key_offset);
      NextKeyOffset = GetNextKeyOffset(offsets, mid);
    }
    else
    {
      apfs_table_var_item* offsets = reinterpret_cast<apfs_table_var_item*>(m_pIndexArea);
      KeyOffset = CPU2LE(offsets[mid].key_offset);
      NextKeyOffset = GetNextKeyOffset(offsets, mid);
    }

    //Check if we find key
    pCurKey->SetKey(reinterpret_cast<apfs_key*>(Add2Ptr(keys, KeyOffset)));

    CCommonSearchKey* pNextKey = NULL;
    if (NextKeyOffset != NEXT_KEY_NOT_FOUND)
    {
      pNextKey = pTempKey2;
      pNextKey->SetKey(reinterpret_cast<apfs_key*>(Add2Ptr(keys, NextKeyOffset)));
    }

    bool bCurKeyLtSearchKey = *pSearchKey > *pCurKey;

    if (FlagOn(SearchRegime, SEARCH_KEY_LOW))    //search low
    {
      //Check if CurKey < SearchKey && NextKey >= SearchKey
      if ( bCurKeyLtSearchKey && (pNextKey == NULL || *pSearchKey <= *pNextKey) )
        return mid;
      if ( mid == 0 && *pSearchKey == *pCurKey )     //SEARCH_KEY_LOW, but 1st item in the tree is equal
        return FIRST_INDEX_ON_LOW_SEARCH;
    }
    else
    {
      bool bKeysAreEqual = (pCurKey->GetType() == pSearchKey->GetType() && *pSearchKey == *pCurKey);

      if ( bKeysAreEqual )     //SEARCH_KEY_EQ or SEARCH_KEY_LE
        return mid;

      if (FlagOn(SearchRegime, SEARCH_KEY_LE) || m_Level != 0)    //search low or equal
      {
        //Check if CurKey <= SearchKey && NextKey > SearchKey
        if ( (bCurKeyLtSearchKey || bKeysAreEqual)
          && (m_Level > 0 || FlagOn(SearchRegime, SEARCH_ALL_TYPES) || pCurKey->GetType() == pSearchKey->GetType())
          && (pNextKey == NULL || *pNextKey > *pSearchKey) )
          return mid;
      }
    }

    if (first == last)
      break;

    if ( bCurKeyLtSearchKey )
      first = mid + 1;
    else
      last = mid;
  }

  return INDEX_NOT_FOUND;
}


/////////////////////////////////////////////////////////////////////////////
void CApfsTable::GetOffsetsAndSizes(
    IN unsigned int     Index,
    IN unsigned short*  KeyOffset,
    IN unsigned short*  KeySize,
    IN unsigned short*  DataOffset,
    IN unsigned short*  DataSize
    ) const
{
  assert(Index < m_Count);

  //Calculate key_offset, key_len, data_offset, data_len by index for different item length types
  if (m_bLenFixed)      //types 4-7
  {
    apfs_table_fixed_item* CurIndex = reinterpret_cast<apfs_table_fixed_item*>(Add2Ptr(m_pIndexArea, Index * sizeof(apfs_table_fixed_item)));
    *KeyOffset = CPU2LE(CurIndex->key_offset);
    *DataOffset = CPU2LE(CurIndex->data_offset);
    GetFixedItemSize(KeySize, DataSize);

#ifndef NDEBUG
    if (m_bHasFooter)
    {
      apfs_table_footer* footer = GetFooter();
      if (m_Level == 0)
        assert(CPU2LE(footer->tf_data_size) == *DataSize);
      assert( CPU2LE(footer->tf_key_size) == *KeySize);
    }
#endif
  }
  else                   //types 0-3
  {
    apfs_table_var_item* CurIndex = reinterpret_cast<apfs_table_var_item*>(Add2Ptr(m_pIndexArea, Index * sizeof(apfs_table_var_item)));
    *KeyOffset = CPU2LE(CurIndex->key_offset);
    *KeySize = CPU2LE(CurIndex->key_size);
    *DataOffset = CPU2LE(CurIndex->data_offset);
    *DataSize = CPU2LE(CurIndex->data_size);
  }

  if (*DataOffset == EMPTY_OFFSET)
    assert(m_pTable->header.content_type == APFS_CONTENT_HISTORY);
  assert(*KeyOffset != EMPTY_OFFSET);
}


/////////////////////////////////////////////////////////////////////////////
void CApfsTable::GetFixedItemSize(
    IN unsigned short* KeySize,
    IN unsigned short* DataSize
    ) const
{
  //key_len and data_len depend on content type
  switch (m_ContentType)
  {
  case APFS_CONTENT_LOCATION:
    *KeySize  = sizeof(apfs_location_table_key);
    *DataSize = (m_Level == 0) ? sizeof(apfs_location_table_data) : sizeof(UINT64);
    break;
  case APFS_CONTENT_HISTORY:
    *KeySize  = sizeof(apfs_history_table_key);
    *DataSize = (m_Level == 0) ? sizeof(apfs_history_table_data) : sizeof(UINT64);
    break;
  case APFS_CONTENT_SNAPSHOTS_MAP:
    *KeySize  = sizeof(apfs_snapshot_map_key);
    *DataSize = (m_Level == 0) ? sizeof(apfs_snapshot_map_data) : sizeof(UINT64);
    break;
  case APFS_CONTENT_ENCRYPTION:
    *KeySize  = sizeof(UINT64);
    *DataSize = sizeof(UINT64);
    break;
  default:
    assert(!"unexpected type");
  }
}


} // namespace apfs

} // namespace UFSD

#endif // UFSD_APFS
