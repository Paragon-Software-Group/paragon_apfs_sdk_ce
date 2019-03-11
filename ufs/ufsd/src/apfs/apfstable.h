// <copyright file="apfstable.h" company="Paragon Software Group">
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


#ifndef __UFSD_APFS_TABLE_H
#define __UFSD_APFS_TABLE_H

#include "../unixfs/unixblock.h"
#include "apfshash.h"
#ifdef BASE_BIGENDIAN
#include "apfsbe.h"
#endif

namespace UFSD
{

namespace apfs
{

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef SubFromPtr
#define SubFromPtr(P,I)   ((unsigned char*)(P) - (I))
#endif

//FindDataIndex return values (last 16 indexes reserved for special values)
#define INDEX_NOT_FOUND               0xFFFFFFFF
#define FIRST_INDEX_ON_LOW_SEARCH     0xFFFFFFFE
#define FIRST_INDEX_IS_GREATER        0xFFFFFFFD
#define IS_INDEX_NOT_FOUND(index)     ((index) == INDEX_NOT_FOUND || (index) == FIRST_INDEX_ON_LOW_SEARCH || (index) == FIRST_INDEX_IS_GREATER)

//macros for key id and type
#define ID_MASK                       ((1ULL << TYPE_SHIFT) - 1)
#define GET_ID(id)                    ((id) & ID_MASK)
#define GET_TYPE(id)                  ((id) >> TYPE_SHIFT)
#define IS_ID_GREATER(id1, id2)   ( (GET_ID(id1) > GET_ID(id2)) || ((GET_ID(id1) == GET_ID(id2)) && (GET_TYPE(id1) > GET_TYPE(id2))) )
#define IS_ID_LE(id1, id2)        ( (GET_ID(id1) < GET_ID(id2)) || ((GET_ID(id1) == GET_ID(id2) && GET_TYPE(id1) <= GET_TYPE(id2))) )

#define MAKE_KEY(id, type)        ((id)) | (static_cast<UINT64>((type)) << TYPE_SHIFT)

#define NEXT_KEY_NOT_FOUND              0xFFFE

//Definitions for key searching
#define SEARCH_KEY_LE    0x1
#define SEARCH_KEY_EQ    0x2
#define SEARCH_KEY_LOW   0x4
#define SEARCH_KEY_GE    0x8
#define SEARCH_ALL_TYPES 0x10

//Returns 0 if strings are equal, > 0 if str1 > str2, < 0 if str1 < str2
int StrCompare(const unsigned char* str1, unsigned int len1, const unsigned char* str2, unsigned int len2);

//Types of search keys
enum ApfsSearchKeyType    // TODO: use enum NodeRecordType from apfs_struct.h
{
  ApfsDefaultKey,
  ApfsSimpleKey,
  ApfsEntryKey,
  ApfsExtentKey,
  ApfsXattrKey,
  ApfsLocationKey,
  ApfsHardlinkKey,
  ApfsHistoryKey
};

//Interface for search keys
struct CCommonSearchKey : UMemBased<CCommonSearchKey>
{
  apfs_key*             m_DiskKey;
  bool                  m_bCaseSensitive;
  ApfsSearchKeyType     m_Type;

  CCommonSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_key*                Key  = NULL,
    IN ApfsSearchKeyType        Type = ApfsDefaultKey,
    IN bool                bCaseSens = true
    )
    : UMemBased<CCommonSearchKey>(Mm)
    , m_DiskKey(Key)
    , m_bCaseSensitive(bCaseSens)
    , m_Type(Type)
  {
  }

  virtual ~CCommonSearchKey()
  {
  }

  virtual UINT64 GetId() const { return GET_ID(m_DiskKey->id); }
  virtual unsigned char GetType() const { return GET_TYPE(m_DiskKey->id); }
  virtual void* GetKey() const { return m_DiskKey; }
  virtual void SetKey( apfs_key* pKeyBuf ) { m_DiskKey = pKeyBuf; }

  virtual unsigned short GetBufferLen() const = 0;

  virtual bool operator==(const CCommonSearchKey& Key) const = 0;
  virtual bool operator<=(const CCommonSearchKey& Key) const = 0;
  virtual bool operator> (const CCommonSearchKey& Key) const = 0;
};

//Key for searching directory entries
struct CEntrySearchKey : CCommonSearchKey
{
  apfs_direntry_key       m_Key;

  CEntrySearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_direntry_key*       Key,
    IN bool                     bCaseSens = true
    )
    : CCommonSearchKey(Mm, reinterpret_cast<apfs_key*>(Key), ApfsEntryKey, bCaseSens)
  {
    m_Key.id = m_Key.name_hash = m_Key.name_len = m_Key.name[0] = 0;
  }

  CEntrySearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN UINT64                   Id,
    IN bool                     bCaseSens,
    IN unsigned int             Hash = 0
    )
    : CCommonSearchKey(Mm, reinterpret_cast<apfs_key*>(&m_Key), ApfsEntryKey, bCaseSens)
  {
    m_Key.id        = MAKE_KEY(Id, ApfsDirEntry);
    m_Key.name_hash = Hash;
    m_Key.name[0]   = '\0';
    m_Key.name_len  = 0;
  }

  int Init(const unsigned char* Name, unsigned int NameLen) const
  {
    int Status = ERR_NOERROR;

    if (m_DiskKey->direntry.name_hash == 0)
    {
      unsigned int Hash = ~0u;
      Status = NormalizeAndCalcHash(Name, NameLen, m_bCaseSensitive, &Hash);
      m_DiskKey->direntry.name_hash = Hash & 0x3FFFFF;
    }

    if (UFSD_SUCCESS(Status))
    {
      Memcpy2( m_DiskKey->direntry.name, Name, NameLen);
      m_DiskKey->direntry.name[NameLen] = 0;
      m_DiskKey->direntry.name_len = NameLen + 1;
    }

    return Status;
  }

  unsigned short GetBufferLen() const
  {
    return static_cast<unsigned short>(sizeof(apfs_direntry_key) - sizeof( m_DiskKey->direntry.name) + m_DiskKey->direntry.name_len);
  }

  //if len1 = 0 or len2 = 0 then don't compare names
  virtual bool operator==(const CCommonSearchKey& Key) const
  {
    apfs_direntry_key* k1 = &m_DiskKey->direntry;
    apfs_direntry_key* k2 = &Key.m_DiskKey->direntry;

    if (k1->id != CPU2LE(k2->id) || k1->name_hash != k2->name_hash)
      return false;

    if ( k1->name_len == 0 || k2->name_len == 0 )
      return true;

    return k1->name_len == k2->name_len &&
           IsNamesEqual(k1->name, k1->name_len - 1, k2->name, k2->name_len - 1, m_bCaseSensitive);
  }

  virtual bool operator<=(const CCommonSearchKey& Key) const
  {
    apfs_direntry_key* k1 = &m_DiskKey->direntry;
    apfs_direntry_key* k2 = &Key.m_DiskKey->direntry;

    if (k1->name_len == 0 || k2->name_len == 0)
      return IS_ID_GREATER(CPU2LE(k2->id), k1->id) || (k1->id == CPU2LE(k2->id) && k1->name_hash <= static_cast<unsigned int>(k2->name_hash));

    return IS_ID_GREATER(CPU2LE(k2->id), k1->id) || (k1->id == CPU2LE(k2->id) && (k1->name_hash < k2->name_hash ||
      (k1->name_hash == k2->name_hash && StrCompare(k1->name, k1->name_len, k2->name, k2->name_len) <= 0)));
  }

  virtual bool operator>(const CCommonSearchKey& Key) const
  {
    apfs_direntry_key* k1 = &m_DiskKey->direntry;
    apfs_direntry_key* k2 = &Key.m_DiskKey->direntry;

    if (k1->name_len == 0 || k2->name_len == 0)
      return IS_ID_GREATER(k1->id, CPU2LE(k2->id)) || (k1->id == CPU2LE(k2->id) && k1->name_hash > static_cast<unsigned int>(k2->name_hash));

    return IS_ID_GREATER(k1->id, CPU2LE(k2->id)) || (k1->id == CPU2LE(k2->id) && (k1->name_hash > k2->name_hash ||
      (k1->name_hash == k2->name_hash && StrCompare(k1->name, k1->name_len, k2->name, k2->name_len) > 0)));
  }
};


//Key for searching extents
struct CExtentSearchKey : CCommonSearchKey
{
  apfs_extent_key     m_Key;

  CExtentSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_extent_key*         Key = NULL
    )
    : CCommonSearchKey( Mm, reinterpret_cast<apfs_key*>(Key), ApfsExtentKey )
  {
    m_Key.id = m_Key.file_offset = 0;
  }

  CExtentSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN UINT64 Id,
    IN UINT64 Offset = 0
    )
    : CCommonSearchKey(Mm, reinterpret_cast<apfs_key*>(&m_Key), ApfsExtentKey)
  {
    m_Key.id = MAKE_KEY(Id, ApfsExtent);
    m_Key.file_offset = Offset;
  }

  unsigned short GetBufferLen() const { return sizeof(apfs_extent_key); }

  virtual bool operator==(const CCommonSearchKey& Key) const
  {
    apfs_extent_key* k1 = &m_DiskKey->extent;
    apfs_extent_key* k2 = &Key.m_DiskKey->extent;

    return k1->id == CPU2LE(k2->id) && k1->file_offset == CPU2LE(k2->file_offset);
  }

  virtual bool operator<=(const CCommonSearchKey& Key) const
  {
    apfs_extent_key* k1 = &m_DiskKey->extent;
    apfs_extent_key* k2 = &Key.m_DiskKey->extent;

    return IS_ID_GREATER(CPU2LE(k2->id), k1->id) ||
           (k1->id == CPU2LE(k2->id) && k1->file_offset <= CPU2LE(k2->file_offset));
  }

  virtual bool operator>(const CCommonSearchKey& Key) const
  {
    apfs_extent_key* k1 = &m_DiskKey->extent;
    apfs_extent_key* k2 = &Key.m_DiskKey->extent;

    return IS_ID_GREATER(k1->id, CPU2LE(k2->id)) ||
          (k1->id == CPU2LE(k2->id) && k1->file_offset > CPU2LE(k2->file_offset));
  }
};


struct CXAttrSearchKey : CCommonSearchKey
{
  CXAttrSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_xattr_key*          Key
    )
    : CCommonSearchKey( Mm, reinterpret_cast<apfs_key*>(Key), ApfsXattrKey )
    , m_bLocal(false)
  {
  }

  CXAttrSearchKey(
    IN api::IBaseMemoryManager* Mm
    )
    : CCommonSearchKey(Mm, NULL, ApfsXattrKey)
    , m_bLocal(false)
  {
  }

  ~CXAttrSearchKey()
  {
    if (m_bLocal )
      Free2( m_DiskKey );
  }

  void SetId(UINT64 InodeId) const
  {
    m_DiskKey->xattr.id = MAKE_KEY(GET_ID(InodeId), ApfsAttribute);
  }

  int Init(UINT64 Id, const char* Name, unsigned short NameLen)
  {
    const size_t KeySize = sizeof( apfs_xattr_key ) + NameLen;
    apfs_key* Key = reinterpret_cast<apfs_key*>(Malloc2( KeySize ));
    CHECK_PTR( Key );

    Free2( m_DiskKey );
    m_DiskKey = Key;

    Memcpy2( m_DiskKey->xattr.name, Name, NameLen );
    m_DiskKey->xattr.name[NameLen] = '\0';
    m_DiskKey->xattr.name_len = NameLen + 1;
    SetId(Id);

    m_bLocal = true;
    return ERR_NOERROR;
  }

  unsigned short GetBufferLen() const
  {
    return sizeof(apfs_xattr_key) - 1 + m_DiskKey->xattr.name_len;
  }

  virtual bool operator==(const CCommonSearchKey& Key) const
  {
    apfs_xattr_key* k1 = &m_DiskKey->xattr;
    apfs_xattr_key* k2 = &Key.m_DiskKey->xattr;

    return k1->id == CPU2LE(k2->id) && k1->name_len == CPU2LE(k2->name_len)
        && StrCompare(k1->name, k1->name_len, k2->name, k2->name_len) == 0;
  }

  virtual bool operator<=(const CCommonSearchKey& Key) const
  {
    apfs_xattr_key* k1 = &m_DiskKey->xattr;
    apfs_xattr_key* k2 = &Key.m_DiskKey->xattr;

    return IS_ID_GREATER(CPU2LE(k2->id), k1->id) || (k1->id == CPU2LE(k2->id)
        && StrCompare(k1->name, k1->name_len, k2->name, k2->name_len) <= 0);
  }

  virtual bool operator>(const CCommonSearchKey& Key) const
  {
    apfs_xattr_key* k1 = &m_DiskKey->xattr;
    apfs_xattr_key* k2 = &Key.m_DiskKey->xattr;

    return IS_ID_GREATER(k1->id, CPU2LE(k2->id)) || (k1->id == CPU2LE(k2->id)
        && StrCompare(k1->name, k1->name_len, k2->name, k2->name_len) > 0);
  }
private:
  bool    m_bLocal;
};


struct CLocationSearchKey : CCommonSearchKey
{
  CLocationSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_location_table_key* Key
    )
    : CCommonSearchKey( Mm, reinterpret_cast<apfs_key*>(Key), ApfsLocationKey )
  {
  }

  unsigned short GetBufferLen() const { return sizeof(apfs_location_table_key); }

  virtual bool operator==(const CCommonSearchKey& Key) const
  {
    apfs_location_table_key* k1 = &m_DiskKey->location;
    apfs_location_table_key* k2 = &Key.m_DiskKey->location;

    return k1->ltk_id == CPU2LE(k2->ltk_id) && k1->ltk_checkpoint == CPU2LE(k2->ltk_checkpoint);
  }

  virtual bool operator<=(const CCommonSearchKey& Key) const
  {
    apfs_location_table_key* k1 = &m_DiskKey->location;
    apfs_location_table_key* k2 = &Key.m_DiskKey->location;

    return k1->ltk_id < CPU2LE(k2->ltk_id) || (k1->ltk_id == CPU2LE(k2->ltk_id) && k1->ltk_checkpoint <= CPU2LE(k2->ltk_checkpoint));
  }

  virtual bool operator>(const CCommonSearchKey& Key) const
  {
    apfs_location_table_key* k1 = &m_DiskKey->location;
    apfs_location_table_key* k2 = &Key.m_DiskKey->location;

    return k1->ltk_id > CPU2LE(k2->ltk_id) || (k1->ltk_id == CPU2LE(k2->ltk_id) && k1->ltk_checkpoint > CPU2LE(k2->ltk_checkpoint));
  }
};


struct CHardlinkSearchKey : CCommonSearchKey
{
  apfs_hlink_key      m_Key;

  CHardlinkSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_hlink_key*          Key
    )
    : CCommonSearchKey(Mm, reinterpret_cast<apfs_key*>(Key), ApfsHardlinkKey)
  {
    m_Key.id = m_Key.hlink_id = 0;
  }

  CHardlinkSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN UINT64 InodeId,
    IN UINT64 LinkId
    )
    : CCommonSearchKey( Mm, reinterpret_cast<apfs_key*>(&m_Key), ApfsHardlinkKey )
  {
    m_Key.id = MAKE_KEY(InodeId, ApfsHardlink);
    m_Key.hlink_id = LinkId;
  }

  unsigned short GetBufferLen() const { return sizeof(apfs_hlink_key); }

  virtual bool operator==(const CCommonSearchKey& Key) const
  {
    apfs_hlink_key* k1 = &m_DiskKey->hlink;
    apfs_hlink_key* k2 = &Key.m_DiskKey->hlink;

    return k1->id == CPU2LE(k2->id) && k1->hlink_id == CPU2LE(k2->hlink_id);
  }

  virtual bool operator<=(const CCommonSearchKey& Key) const
  {
    apfs_hlink_key* k1 = &m_DiskKey->hlink;
    apfs_hlink_key* k2 = &Key.m_DiskKey->hlink;

    return IS_ID_GREATER(CPU2LE(k2->id), k1->id) || (k1->id == CPU2LE(k2->id) && k1->hlink_id <= CPU2LE(k2->hlink_id));
  }

  virtual bool operator>(const CCommonSearchKey& Key) const
  {
    apfs_hlink_key* k1 = &m_DiskKey->hlink;
    apfs_hlink_key* k2 = &Key.m_DiskKey->hlink;

    return IS_ID_GREATER(k1->id, CPU2LE(k2->id)) || (k1->id == CPU2LE(k2->id) && k1->hlink_id > CPU2LE(k2->hlink_id));
  }
};

struct CHistorySearchKey : CCommonSearchKey
{
  CHistorySearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN apfs_history_table_key*  Key
  )
    : CCommonSearchKey( Mm, reinterpret_cast<apfs_key*>(Key), ApfsHistoryKey )
  {}

  unsigned short GetBufferLen() const { return sizeof( apfs_history_table_key ); }

  virtual bool operator==( const CCommonSearchKey& Key ) const
  {
    apfs_history_table_key* k1 = &m_DiskKey->history;
    apfs_history_table_key* k2 = &Key.m_DiskKey->history;

    return k1->htk_block == CPU2LE( k2->htk_block ) && k1->htk_checkpoint == CPU2LE( k2->htk_checkpoint );
  }

  virtual bool operator<=( const CCommonSearchKey& Key ) const
  {
    apfs_history_table_key* k1 = &m_DiskKey->history;
    apfs_history_table_key* k2 = &Key.m_DiskKey->history;

    return k1->htk_checkpoint < CPU2LE( k2->htk_checkpoint ) || (k1->htk_checkpoint == CPU2LE( k2->htk_checkpoint ) && k1->htk_block <= CPU2LE( k2->htk_block ));
  }

  virtual bool operator>( const CCommonSearchKey& Key ) const
  {
    apfs_history_table_key* k1 = &m_DiskKey->history;
    apfs_history_table_key* k2 = &Key.m_DiskKey->history;

    return k1->htk_checkpoint > CPU2LE( k2->htk_checkpoint ) || (k1->htk_checkpoint == CPU2LE( k2->htk_checkpoint ) && k1->htk_block > CPU2LE( k2->htk_block ));
  }
};



//Common key for other search operations
//Suitable for ApfsInode and ApfsHardLinkId key types
struct CSimpleSearchKey : CCommonSearchKey
{
  UINT64 id;

  CSimpleSearchKey(
    IN api::IBaseMemoryManager* Mm,
    IN UINT64                   Id = 0,
    IN unsigned char            Type = ApfsDefaultKey
    )
    : CCommonSearchKey( Mm )
  {
    id = MAKE_KEY(Id, Type);
  }

  virtual UINT64 GetId() const { return id; }
  virtual unsigned char GetType() const { return GET_TYPE(id); }
  virtual void SetKey( apfs_key* pKeyBuf )
  {
    id = pKeyBuf != NULL ? pKeyBuf->id : 0;
    //if ( pKeyBuf != NULL ) // aligning
    //  Memcpy2( &id, pKeyBuf, sizeof( UINT64 ) );
  }
  virtual void* GetKey() const { return (void*)&id; }

  unsigned short GetBufferLen() const { return sizeof(UINT64); }

  virtual bool operator==(const CCommonSearchKey& Key) const
  {
    return id == CPU2LE(Key.GetId());
  }
  virtual bool operator<=(const CCommonSearchKey& Key) const
  {
    return IS_ID_LE(id, Key.GetId());
  }
  virtual bool operator>(const CCommonSearchKey& Key) const
  {
    return IS_ID_GREATER(id, Key.GetId());
  }
};


struct CKeyFactory : UMemBased<CKeyFactory>
{
  CKeyFactory(api::IBaseMemoryManager* Mm)
  : UMemBased<CKeyFactory>(Mm)
  {
  }

  int CreateKey( ApfsSearchKeyType Type, apfs_key* pDiskKey, CCommonSearchKey** Key ) const
  {
    if ( *Key != NULL )
    {
      if ( (*Key)->m_Type == Type )
      {
        (*Key)->SetKey( pDiskKey );
        return ERR_NOERROR;
      }

      delete *Key;
      *Key = NULL;
    }

    switch (Type)
    {
    case ApfsDefaultKey:
    case ApfsSimpleKey:
      *Key = new(m_Mm) CSimpleSearchKey( m_Mm, pDiskKey != NULL ? pDiskKey->id : 0 );
      break;
    case ApfsEntryKey:
      *Key = new(m_Mm) CEntrySearchKey( m_Mm, &pDiskKey->direntry );
      break;
    case ApfsExtentKey:
      *Key = new(m_Mm) CExtentSearchKey( m_Mm, &pDiskKey->extent );
      break;
    case ApfsHardlinkKey:
      *Key = new(m_Mm) CHardlinkSearchKey( m_Mm, &pDiskKey->hlink );
      break;
    case ApfsLocationKey:
      *Key = new(m_Mm) CLocationSearchKey( m_Mm, &pDiskKey->location );
      break;
    case ApfsXattrKey:
      *Key = new(m_Mm) CXAttrSearchKey( m_Mm, &pDiskKey->xattr );
      break;
    case ApfsHistoryKey:
      *Key = new(m_Mm) CHistorySearchKey( m_Mm, &pDiskKey->history );
      break;
    default:
      assert( !"Wrong key type" );
    }

    assert( *Key != NULL );
    return ( NULL == *Key ) ? ERR_NOMEMORY : ERR_NOERROR;
  }
};


//Struct for working with empty list items
struct CEmptyListItem
{
  apfs_empty_entry* EmptyEntry;
  unsigned short* EmptyOffset;

  CEmptyListItem() : EmptyEntry(NULL), EmptyOffset(NULL) {}
  CEmptyListItem(const CEmptyListItem &item) : EmptyEntry(item.EmptyEntry), EmptyOffset(item.EmptyOffset) {}
};


//Cache block for apfs
class CApfsBlock: public CUnixBlock
{
public:
  CApfsBlock(api::IBaseMemoryManager* mm) : CUnixBlock(mm) {}

#ifndef UFSD_APFS_RO
  virtual int Flush();
  virtual void SetDirty(bool bDirty = true);
#else
  virtual int Flush() { return ERR_NOERROR;  }
  virtual void SetDirty(bool /*bDirty*/ = true) {}
#endif
};


class CApfsTreeInternal;

//Apfs table class
class CApfsTable: public UMemBased<CApfsTable>
{
  CApfsTreeInternal*     m_pTree;            //Pointer to the tree which contains this table
  CApfsSuperBlock*       m_pSuper;           //Pointer to sb
  apfs_table*            m_pTable;           //Pointer to table block buffer
  void*                  m_pIndexArea;
  void*                  m_pDataArea;
  apfs_table_footer*     m_pFooter;
  CUnixSmartBlockPtr     m_TableBlock;       //Block cache element for storing table content
  UINT64                 m_BlockNum;         //Number of block where table is stored
  UINT64                 m_Id;               //Table id
  UINT64                 m_CheckPointId;     //Table checkpoint id
  unsigned int           m_Count;            //Number of records in the table
  unsigned short         m_Level;            //Level of current table
  unsigned short         m_ContentType;      //Type of table content
  bool                   m_bLenFixed;        //Are length of data and keys fixed?
  bool                   m_bHasFooter;       //Has the table footer?
  bool                   m_bOrdered;         //key and data offsets in index fields are growing
  unsigned char          m_VolIndex;         //Volume index which this tree belongs to

public:
  CApfsTable(CApfsTreeInternal *pTree);

  //Read table from disk and init class members
  int Init(UINT64 BlockNumber, unsigned char VolIndex, bool bMayBeEncrypted);

  //ReInit class members by m_pTable
  int ReInit();

  int Flush() const
  {
    return m_TableBlock.Flush();
  }

  //Load table buffer (from disk or cache)
  int LoadBuffer();

  //Unload table buffer (move disk block to cache)
  int UnloadBuffer();

  //Return true if table buffer is loaded
  bool IsBufferLoaded() const { return m_pTable != NULL; }

  //Get table parameters
  unsigned int GetRecordsCount() const { return m_Count; }
  unsigned short GetLevel() const { return m_Level; }
  unsigned short GetContentType() const{ return m_ContentType; }
  bool IsItemLenFixed() const { return m_bLenFixed; }
  UINT64 GetBlockNumber() const { return m_BlockNum; }
  UINT64 GetId() const { return m_Id; }
  UINT64 GetCheckPointId() const { return m_CheckPointId; }
  unsigned char GetVolumeIndex() const{ return m_VolIndex; }

  bool MayBeEncrypted() const { return m_ContentType == APFS_CONTENT_FILES; }

  //Get pointer to table
  void* GetTableBuffer() const { return m_pTable; }

  //Get pointer to block header
  apfs_block_header* GetBlockHeader() const { return &m_pTable->header; }

  //Get item by index
  void GetItem(
      IN  unsigned int    Index,
      OUT void**          Key,
      OUT void**          Data,
      OUT unsigned short* KeyLen = NULL,
      OUT unsigned short* DataLen = NULL
      ) const;

  //Find last index that satisfies condition key[index] <= SearchKey
  int FindDataIndex(
      IN const CCommonSearchKey*  pSearchKey,
      IN unsigned short           SearchRegime
      );

  //Calculate crc and set block dirty
  void SetDirty() const
  {
    m_TableBlock.SetDirty();
    if (m_VolIndex != BLOCK_BELONGS_TO_CONTAINER)
      m_pSuper->SetNeedFixup();
  }

  //get log object for trace
  api::IBaseLog* GetLog() const { return m_pSuper->GetLog(); }

  //Get pointer to superblock object
  CApfsSuperBlock* GetSuper() const { return m_pSuper; }

  //Get pointer to footer
  apfs_table_footer* GetFooter() const { return m_pFooter; }

  //Get free_area_sizee field in the table
  unsigned short GetFreeSize() const { return m_pTable ? m_pTable->table_header.free_area_size : 0; }

private:
  void InitTablePointers()
  {
    m_pIndexArea = Add2Ptr(m_pTable, sizeof(apfs_block_header) + sizeof(apfs_table_header));
    m_pDataArea = Add2Ptr(m_pTable, m_pSuper->GetBlockSize() - (m_bHasFooter ? sizeof(apfs_table_footer) : 0));
    m_pFooter = (m_pTable && m_bHasFooter) ? reinterpret_cast<apfs_table_footer*>(Add2Ptr(m_pTable, m_pSuper->GetBlockSize() - sizeof(apfs_table_footer))) : NULL;
  }
  void* GetKeyArea() const { return Add2Ptr(m_pIndexArea, CPU2LE(m_pTable->table_header.index_area_size)); }

  //Get data item by DataOffset
  void* GetDataItem(unsigned short offset) const { return (char*)m_pDataArea - offset; }

  //Get offset of next not deleted item
  template <class T>
  unsigned short GetNextKeyOffset(T* items, unsigned int index)
  {
    return index + 1 < m_Count ? CPU2LE(items[index + 1].key_offset) : NEXT_KEY_NOT_FOUND;
  }

  //Get table data offset by offset from block beginning
  unsigned short GetTableDataOffset(unsigned short OffsetFromBeginning) const
  {
    return static_cast<unsigned short>(m_pSuper->GetBlockSize() - (m_bHasFooter ? sizeof(apfs_table_footer) : 0)) - OffsetFromBeginning;
  }

  //Get ofsets and sizes of items by index
  void GetOffsetsAndSizes(
      IN unsigned int     Index,
      IN unsigned short*  KeyOffset,
      IN unsigned short*  KeySize,
      IN unsigned short*  DataOffset,
      IN unsigned short*  DataSize
      ) const;

  //Get fixed item size for this table
  void GetFixedItemSize(
      IN unsigned short* KeySize,
      IN unsigned short* DataSize
      ) const;

  //Returns size of index
  unsigned short GetIndexSize() const { return m_bLenFixed ? sizeof(apfs_table_fixed_item) : sizeof(apfs_table_var_item); }

#ifndef UFSD_APFS_RO
  //Calc footer->tf_unknown_0x00 field
  static unsigned int SetFlags(IN unsigned short ContentType);

  //Help function for filling table header with default values
  void CalculateKeyAndDataArea(IN apfs_table* pTable) const;

  //Find empty space for key or data with requested len or more in emty list. Return Offset of found area, recalculate table fields
  static unsigned short AllocateEmptyOffset(void* pBuffer, unsigned short* StartOffset, unsigned short* TotalLen, unsigned short RequiredLen, bool bIsKey);

  //Add new item to empty list
  static void AddEmptyOffset(void* pBuffer, unsigned short* StartOffset, unsigned short* TotalLen, unsigned short KeyOffset, unsigned short KeyLen, bool bIsKey);

  //Copy records from m_pTable to pNewTable
  void CopyRecords(unsigned int FirstRecord, unsigned int LastRecord, apfs_table* pNewTable) const;
public:
  //Create table an write it to disk / to blocks cache
  int Create(
      IN unsigned short BlockHeaderFlag,
      IN unsigned short ContentType,
      IN unsigned short Level,
      IN bool           bLenFixed,
      IN unsigned char  VolIndex,
      IN bool           bHasFooter  = false,
      IN UINT64         Id          = 0,
      IN UINT64         BlockNum    = 0
      );

  //Move 1/2 records to the new table
  int SplitTable(unsigned int index, unsigned short NewItemSize, CApfsTable* pNewTable);

  //Defragment table
  int Defragment();

  //Trace this table
  void Trace() const;

  //Reset table to initial values
  void Reset(IN unsigned short Level);

  //Add item to selected table
  void AddItem(unsigned int Index, void* Key, void* Data, unsigned short DataLen, unsigned short KeyLen);

  //Remove item form table by index
  void RemoveItem(unsigned int index);

  //Update item data with specified index
  bool UpdateItemData(unsigned int index, void* NewData, unsigned short NewDataLen);

  //Update item key with specified index
  bool UpdateItemKey(unsigned int index, void* NewKey, unsigned short NewKeyLen);

  //Checks if we can add item to table
  bool CanAdd(unsigned short KeyLen, unsigned short DataLen) const;

  //Table become leaf because it is empty
  void BecomeLeaf();

  //Copy all records from pSrcTable
  void CopyFrom(CApfsTable* pSrcTable)
  {
    pSrcTable->CopyRecords(0, pSrcTable->GetRecordsCount(), m_pTable);
    m_Count = CPU2LE(m_pTable->table_header.count);
  }
#endif
};

} // namespace apfs

} // namespace UFSD

#endif
