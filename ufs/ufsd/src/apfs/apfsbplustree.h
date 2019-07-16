// <copyright file="apfsbplustree.h" company="Paragon Software Group">
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

#ifndef __UFSD_APFS_BPLUS_TREE_H
#define __UFSD_APFS_BPLUS_TREE_H

#include "apfstable.h"

namespace UFSD
{

namespace apfs
{

#define APFS_MAX_KEY_LEN              (MAX_FILENAME + sizeof(apfs_direntry_key))

//Node of Tree class
class CApfsTreeNode : public UMemBased<CApfsTreeNode>
{
public:
  CApfsTreeNode(CApfsTree* pTree, CApfsTree* pLocationTree, bool bEnumerator);
  ~CApfsTreeNode();

  CApfsTreeNode*    m_Parent;          //Pointer to parent node (NULL for root)
  CApfsTreeNode*    m_pChild;          //Pointer to children in active branch (NULL for leaves)
  CApfsTree*        m_pLocationTree;   //Location tree used for searching table block by object id. NULL if this tree is location tree
  CApfsTree*        m_pTree;           //Link to tree
  CApfsTable*       m_pTable;          //Pointer to table object for this node
  unsigned int      m_Pos;             //Position in parent node
  bool              m_bEnumerator;     //Tree node belongs to enumerator

  //Initialize node
  //Pos - position in parent node
  int Init(CApfsTreeNode *Parent, unsigned int Pos);

  //Reinitialize node
  int ReInit(CApfsTreeNode* Parent, unsigned int Pos);

  //Initialize root node
  int InitAsRoot(UINT64 BlockNumber, unsigned char VolIndex, bool bMayBeEncrypted);

  int Flush() const { return m_pTable->Flush(); }

  //Load child with specified ChildIndex
  int LoadChild(unsigned int ChildIndex);

  ////////////////////////////////////////////////////////////////////////////////
  // Recall function from CApfsTable class. Load and unload table if required
  ////////////////////////////////////////////////////////////////////////////////

  //Find last index that satisfies condition key[index] <= SearchKey
  int FindDataIndex(
    IN  const CCommonSearchKey* SearchKey,
    IN  unsigned short          SearchRegime,
    OUT unsigned int*           FoundIndex
    ) const
  {
    if (!m_pTable->IsBufferLoaded())
      CHECK_CALL(m_pTable->LoadBuffer());

    *FoundIndex = m_pTable->FindDataIndex(SearchKey, SearchRegime);

//    ULOG_DEBUG1( (GetLog(), "FindDataIndex %" PLL "x, block=%" PLL "x, level=%x -> %x", SearchKey->GetId(), m_pTable->GetBlockNumber(), m_pTable->GetLevel(), *FoundIndex) );

    if (NeedUnloadBuffer())
      CHECK_CALL(m_pTable->UnloadBuffer());

    return ERR_NOERROR;
  }

  //Get item by index
  int GetItem(unsigned int index, void** Key, void** Data, unsigned short* KeyLen = NULL, unsigned short *DataLen = NULL) const
  {
    if (!m_pTable->IsBufferLoaded())
      CHECK_CALL(m_pTable->LoadBuffer());

    m_pTable->GetItem(index, Key, Data, KeyLen, DataLen);

    if (NeedUnloadBuffer())
      CHECK_CALL(m_pTable->UnloadBuffer());

    return ERR_NOERROR;
  }

  //Get number of elements in this node (table)
  unsigned int Count() const { return m_pTable->GetRecordsCount(); }

  //Get Depth of this node (table)
  unsigned short Depth() const { return m_pTable->GetLevel(); }

  //Get Id of this node (table)
  UINT64 Id() const { return m_pTable->GetId(); }

  ////////////////////////////////////////////////////////////////////////////////
  // End of functions recall from CApfsTable class
  ////////////////////////////////////////////////////////////////////////////////

private:
  //Checks if this tree is this node belongs to location tree
  bool IsLocationTree() const { return m_pLocationTree == NULL; }    //Location tree hasn't pointers to location tree (m_pLocationTree = NULL)

  //Checks if this node is root
  bool IsRootNode() const { return m_Parent == NULL; }               //Root node hasn't parents (m_Parent = NULL)

  //Is block need to unload (move disk block to cache)
  bool NeedUnloadBuffer() const { return !IsLocationTree() && !IsRootNode() && m_bEnumerator && Depth() != 0; }

  //Get log object for trace
  api::IBaseLog* GetLog() const { return m_pTable->GetLog(); }

#ifndef UFSD_APFS_RO
  //Create child node
  int Create(CApfsTreeNode* Parent, unsigned int Pos, CApfsTreeNode** ppChild) const;

  //Help function for AddItem, UpdateItemData
  int AddItemInternal(unsigned int index, void* Key, void* Data, unsigned short DataLen, unsigned short KeyLen);

  //Split this node on 2 nodes (this and NewNode).
  int SplitNode(unsigned int index, unsigned short ItemSize, CApfsTreeNode** NewNode);

  //Update key with specified index (when adding/removing 0th item in the child)
  int UpdateItemKey(unsigned int index, void* NewKey, unsigned short NewKeyLen);

  //Add table info to location tree
  int AddToLocationTree(const CApfsTable* pTable) const;

  //Add this node to the tree
  int AddToTree() const;

public:
  //Add item to table
  int AddItem(unsigned int index, void* Key, void* Data, unsigned short DataLen, unsigned short KeyLen);

  //Remove item from table
  int RemoveItem(unsigned int index) const;

  //Update data for item
  int UpdateItemData(unsigned int index, void* NewData, unsigned short NewDataLen);

  void Trace() const
  {
    bool bNeedLoadBuffer = !m_pTable->IsBufferLoaded();
    if (bNeedLoadBuffer)
      m_pTable->LoadBuffer();
    m_pTable->Trace();
    if (bNeedLoadBuffer)
      m_pTable->UnloadBuffer();
  }
#endif
};


//Internal class, common for CApfsTree and CApfsTreeEnum
class CApfsTreeInternal: public UMemBased<CApfsTreeInternal>
{
protected:
  CKeyFactory*           m_KeyFactory;
  CApfsTree*             m_pLocationTree;   //Location tree used for searching table block by object id. NULL if this tree is location tree

  CApfsTreeNode*         m_pRootNode;       //Root node of tree
  CApfsTreeNode*         m_pCurNode;        //current node
  unsigned int           m_PosInCurNode;    //Position in m_pCurNode (for enumeration)

public:
  CApfsSuperBlock*       m_pSuper;          //Pointer to superblock
  bool                   m_bCurNodeValid;   //m_pCurNode is Valid

  CCommonSearchKey*      m_TempKey1;
  CCommonSearchKey*      m_TempKey2;

  CApfsTreeInternal(CApfsSuperBlock* sb);
  virtual ~CApfsTreeInternal();

  CKeyFactory* GetKeyFactory() const { return m_KeyFactory; }

  int Flush() const
  {
    CApfsTreeNode* p = m_pRootNode;
    while (p != NULL)
    {
      CHECK_CALL(p->Flush());
      p = p->m_pChild;
    }

    return ERR_NOERROR;
  }

  //Get tree parameters
  UINT64 Count() const;
  unsigned short GetLevel() const { return m_pRootNode->Depth(); }

  UINT64 GetCurBlock() const { return m_pCurNode->m_pTable->GetBlockNumber(); }

  //Get root table in tree
  CApfsTable* GetRootTable() const { return m_pRootNode ? m_pRootNode->m_pTable : NULL; }

  //Get root node in tree
  CApfsTreeNode* GetRootNode() const { return m_pRootNode; }

  //Find key and data by SearchKey. Found KeyRecord should be <= SearchKey
  //use this function if you are sure that there are no duplicating keys in the tree. Otherwise use StartEnumByKey and EnumNextPtrByKey pair
  int GetData(
    IN const CCommonSearchKey*  SearchKey,
    IN unsigned short           SearchRegime,
    OUT void**                  Data,
    OUT void**                  KeyRecord = NULL
  );

  int GetExtentPtr(
    IN  UINT64              Id,
    IN  UINT64              Offset,
    OUT apfs_extent_data**  pExtent,
    OUT apfs_extent_key**   pDiskKey
    )
  {
    if (Offset != 0 && m_pCurNode->Depth() == 0 && m_PosInCurNode < m_pCurNode->Count() - 1)
    {
      CHECK_CALL(m_pCurNode->GetItem(m_PosInCurNode + 1, (void**)pDiskKey, (void**)pExtent));

      if ((*pDiskKey)->id == Id &&
          (*pDiskKey)->file_offset <= Offset &&
          (*pDiskKey)->file_offset + (*pExtent)->ed_size > Offset)
      {
        ++m_PosInCurNode;
        return ERR_NOERROR;
      }
    }

    const CExtentSearchKey Key( m_Mm, Id, Offset );

    CHECK_CALL_SILENT(GetData(&Key, SEARCH_KEY_LE, (void**)pExtent, (void**)pDiskKey));
    if ((*pDiskKey)->id != Key.m_DiskKey->id)
    {
      //ULOG_ERROR((GetLog(), ERR_NOTFOUND, "GetExtentPtr: Found key with Id=0x%" PLL "x, expected Id=0x%" PLL "x", (*pDiskKey)->Id, SearchKey.Id));
      //TRACE_ONLY(TraceCurNode());
      return ERR_NOTFOUND;
    }
    return ERR_NOERROR;
  }

  CApfsSuperBlock* GetSuper()const { return m_pSuper; }

  //Get object for trace
  api::IBaseLog* GetLog() const { return m_pSuper->GetLog(); }

  //Init internal tree class
  int Init(UINT64 BlockNumber, unsigned char VolIndex, CApfsTree* pLocationTree, CApfsTree* pTree, bool bMayBeEncrypted);

  //Help functions for enumeration
  int FindFirstLeaf();
  int FindNextLeaf(CApfsTreeNode** NextLeaf) const;

  //Checks if this tree is this tree is location tree
  bool IsLocationTree() const { return m_pLocationTree == NULL; }

  //Find m_pCurNode and m_PosInCurNode for pSearchKey and SearchRegime
  int FindPosition(const CCommonSearchKey* pSearchKey, unsigned short SearchRegime);

private:

  //Get leaf node which suits SearchKey. Store it in m_pCurNode pointer
  int GetCurNode(IN const CCommonSearchKey* pSearchKey, IN unsigned short SearchRegime);

  virtual bool IsEnumerator() const = 0;

  //Help function for GetData. Returns true if we need start search data from tree root
  bool NeedSearchFromRoot(const CCommonSearchKey* pSearchKey, unsigned short SearchRegime) const;

#ifndef UFSD_APFS_RO
#ifdef UFSD_TRACE
  //Trace root node
  void TraceRootNode() const
  {
    m_pRootNode->Trace();
  }

  //Trace active branch from root to leaf
  void TraceBranch() const
  {
    for (CApfsTreeNode *p = m_pRootNode; p != NULL; p = p->m_pChild)
      p->Trace();
  }

  //Trace current node
  void TraceCurNode() const
  {
    ULOG_DEBUG1((GetLog(), "Trace current table"));
    m_pCurNode->Trace();
  }
#endif
public:
  //Update table when buffer modified outside
  //(Only for data modification because on Key modification with 0th index we should recalculate parent key)
  int UpdateCurBlock() const;
#endif
};


//forward declaration
class CApfsTreeEnum;

//Tree class
class CApfsTree : public CApfsTreeInternal
{
  apfs_btreed*           m_pRootDesc;       //Descriptor of tree root. Presented not in all trees
  CApfsTreeEnum*         m_pDefaultEnum;    //default enumerator for tree

public:

  list_head              m_EnumsList;       //List of enumerators

  CApfsTree(CApfsSuperBlock* sb);
  ~CApfsTree();

  //Init tree
  //BlockNumber - number of tree root or tree root descriptor
  //VolIndex - index of volume which tree belongs to
  //bHasRootDesc - if tree has root descriptor
  //pLocationTree - pointer to location tree (for ds object tree)
  int Init(UINT64 BlockNumber, unsigned char VolIndex, bool bHasRootDesc, CApfsTree* pLocationTree = NULL, bool bMayBeEncrypted = false);

  //Get default enumerator for tree
  CApfsTreeEnum* GetDefaultEnum() const { return m_pDefaultEnum; }

  //Get location tree for this tree (NULL if not exisits)
  CApfsTree* GetLocationTree() const { return m_pLocationTree; }

  //Get item with specified index
  int GetItem(UINT64 index, void** Key, void** Data, unsigned short* KeyLen = NULL, unsigned short *DataLen = NULL);

  //Get actual location for location tree
  int GetActualLocation(UINT64 Id, apfs_location_table_key* Key, apfs_location_table_data* Data) const;

#ifndef UFSD_APFS_RO
  int InvalidateEnumerators();

  //Check if key exists in the three
  int IsKeyExists(UINT64 Id, unsigned char KeyType, bool* IsExists);
  int IsKeyExists(UINT64 Id, bool* IsExists) { return IsKeyExists(Id, 0, IsExists); }

  //Add leaf item to tree.
  int AddLeafItem(CCommonSearchKey* Key, void* Data, unsigned short DataLen);
  int AddEntryLeaf(CEntrySearchKey* Key, apfs_direntry_data* Data) { return AddLeafItem(Key, Data, sizeof(apfs_direntry_data)); }
  int AddExtentLeaf(CExtentSearchKey* Key, apfs_extent_data* Data) { return AddLeafItem(Key, Data, sizeof(apfs_extent_data)); }
  int AddXAttrLeaf(CXAttrSearchKey* Key, apfs_xattr_data* Data, unsigned short DataLen) { return AddLeafItem(Key, Data, DataLen); }
  int AddLocationLeaf(CLocationSearchKey* Key, apfs_location_table_data* Data) { return AddLeafItem(Key, Data, sizeof(apfs_location_table_data)); }
  int AddExtentStatusLeaf(UINT64 ObjectID, unsigned int Data = 1)
  {
    bool bExists;
    CHECK_CALL(IsKeyExists(ObjectID, ApfsExtentStatus, &bExists));

    if (bExists)
      return ERR_NOERROR;

    CSimpleSearchKey Key(m_Mm, ObjectID, ApfsExtentStatus);
    return AddLeafItem(&Key, &Data, sizeof(unsigned int));
  }

  //Remove leaf item to tree
  int RemoveLeafItem(CCommonSearchKey* Key);

  int RemoveEntryLeaf(CEntrySearchKey* Key) { return RemoveLeafItem(Key); }
  int RemoveExtentLeaf(CExtentSearchKey* Key) { return RemoveLeafItem(Key); }
  int RemoveXAttrLeaf(CXAttrSearchKey* Key) { return RemoveLeafItem(Key); }
  int RemoveLocationLeaf(CLocationSearchKey* Key) { return RemoveLeafItem(Key); }
  int RemoveExtentStatusLeaf(UINT64 ObjectID)
  {
    bool bExists;
    CHECK_CALL(IsKeyExists(ObjectID, ApfsExtentStatus, &bExists));

    if (bExists)
    {
      CSimpleSearchKey Key(m_Mm, ObjectID, ApfsExtentStatus);
      CHECK_CALL(RemoveLeafItem(&Key));
    }
    return ERR_NOERROR;
  }

  //Update leaf item data
  //NewData shouldn't cross with table buffer
  int UpdateLeafItemData(CCommonSearchKey* Key, void* NewData, unsigned short NewDataLen);

  int UpdateEntryLeafData(CEntrySearchKey* Key, apfs_direntry_data* NewData) { return UpdateLeafItemData(Key, NewData, sizeof(apfs_direntry_data)); }
  int UpdateExtentLeafData(CExtentSearchKey* Key, apfs_extent_data* NewData) { return UpdateLeafItemData(Key, NewData, sizeof(apfs_extent_data)); }
  int UpdateXAttrLeafData(CXAttrSearchKey* Key, apfs_xattr_data* NewData, unsigned short NewDataLen) { return UpdateLeafItemData(Key, NewData, NewDataLen); }
  int UpdateLocationLeafData(CLocationSearchKey* Key, apfs_location_table_data* NewData) { return UpdateLeafItemData(Key, NewData, sizeof(apfs_location_table_data)); }

  //Update leaf item key
  //NewKey shouldn't cross with table buffer
  int UpdateLeafItemKey(CCommonSearchKey* Key, CCommonSearchKey* NewKey);

  int UpdateEntryLeafKey(CEntrySearchKey* Key, CEntrySearchKey* NewKey) { return UpdateLeafItemKey(Key, NewKey); }
  int UpdateXAttrLeafKey(CXAttrSearchKey* Key, CXAttrSearchKey* NewKey) { return UpdateLeafItemKey(Key, NewKey); }
#endif

private:
  virtual bool IsEnumerator() const { return false; }
};


//Tree enumerator
class CApfsTreeEnum : public CApfsTreeInternal
{
  apfs_key               m_EnumKey;                                  //Key for enumeration (set in StartEnumByKey function)
  unsigned int           m_EnumRegime;                               //See SEARCH_KEY_XX definition
  ApfsSearchKeyType      m_EnumType;                                 //Search key type (for casting from CCommonSearchKey)
  bool                   m_bCaseSensitive;                           //Is enumeration case sensitive

  bool                   m_bValid;                                   //Is enumerator valid
  bool                   m_bEnumStarted;                             //Is enumeration started
  bool                   m_bNeedNextOnRestoreEnum;                   //Internal flag for RestoreEnum
  bool                   m_bTreeEmpty;                               //Tree is empty on set SetInvalid function. We don't need remember the key
  apfs_key               m_CurKey;                                  //Current key to resume enumeration

public:

  list_head              m_Entry;        //for enums list in the tree

  CApfsTreeEnum(CApfsSuperBlock* pSuper);
  virtual ~CApfsTreeEnum();

  //Init tree
  int Init(CApfsTree* pTree)
  {
    if (m_Entry.is_empty())
      m_Entry.insert_before(&pTree->m_EnumsList);
    return CApfsTreeInternal::Init(pTree->GetRootTable()->GetBlockNumber(), pTree->GetRootTable()->GetVolumeIndex(), pTree->GetLocationTree(), pTree, pTree->GetRootTable()->MayBeEncrypted());
  }

#ifdef UFSD_APFS_CHECK
  UINT64 GetCurTableId() const { return m_pCurNode->m_pTable->GetId(); }
  CApfsTable* GetCurTable() const { return m_pCurNode->m_pTable; }
#endif

  //Functions for enumeration
  //Start enumeration by key and regime
  int StartEnumByKey(CCommonSearchKey* Key, unsigned int EnumRegime, bool bCaseSensitive = true)
  {
    Memcpy2(&m_EnumKey, Key->GetKey(), Key->GetBufferLen());
    m_EnumType = Key->m_Type;
    m_bCaseSensitive = bCaseSensitive;

    CHECK_CALL_SILENT(InitEnumerator());

    m_EnumRegime = EnumRegime;
    int Status = GetData(Key, SEARCH_KEY_LOW | SEARCH_ALL_TYPES, NULL);
    if (Status == ERR_NOTFOUND && (m_PosInCurNode == FIRST_INDEX_ON_LOW_SEARCH || (m_PosInCurNode == INDEX_NOT_FOUND && FlagOn(EnumRegime, SEARCH_KEY_GE))))
    {
      m_PosInCurNode = 0;
      return FindFirstLeaf();
    }
    return Status;
  }

  //Start enumeration of all items
  virtual int StartEnum();

  //Enumerate next pointer
  int EnumNextPtr(void** Key, void** Data = NULL, unsigned short* KeyLen = NULL, unsigned short *DataLen = NULL);

  //Enumerate next pointer which satisfies for EnumKey and EnumRegime
  //search_key - type of enumeration key. Operators >, <=, == and function GetType should be defined (for example CSimpleSearchKey, CExtentSearchKey, CEntrySearchKey and etc, see file apfstable.h)
  int EnumNextPtrByKey(void** Key, void** Data = NULL, unsigned short* KeyLen = NULL, unsigned short* DataLen = NULL);

  //Enumerate next item (and copy it to pKey and pData buffers)
  int EnumNext(void* pKey, void* pData)
  {
    void *pTempKey = NULL, *pTempData = NULL;
    unsigned short KeyLen, DataLen;
    CHECK_CALL_SILENT(EnumNextPtr(&pTempKey, &pTempData, &KeyLen, &DataLen));

    if (pKey)
      Memcpy2(pKey, pTempKey, KeyLen);
    if (pData)
      Memcpy2(pData, pTempData, DataLen);

    return ERR_NOERROR;
  }

#ifndef UFSD_APFS_RO
  int SetInvalid();
#endif

private:
  virtual bool IsEnumerator() const { return true; }

  int RestoreEnumerator();
  int InitEnumerator();
};


class CEntryTreeEnum : public CApfsTreeEnum
{
  UINT64      m_Position;
  UINT64      m_Id;
public:
  CEntryTreeEnum(CApfsSuperBlock* pSuper, UINT64 Id)
    : CApfsTreeEnum(pSuper)
    , m_Position(0)
    , m_Id(Id)
  {}
  virtual ~CEntryTreeEnum() {}

  virtual int StartEnum()
  {
    CSimpleSearchKey Key(m_Mm, m_Id, ApfsDirEntry);
    return StartEnumByKey(&Key, SEARCH_KEY_EQ);
  }

  int EnumNextEntry(apfs_direntry_key** Key, apfs_direntry_data** Data)
  {
    CHECK_CALL_SILENT(EnumNextPtrByKey((void**)Key, (void**)Data));
    ++m_Position;
    return ERR_NOERROR;
  }

  int SetPosition(UINT64 Pos);
};

} // namespace apfs

} // namespace UFSD

#endif
