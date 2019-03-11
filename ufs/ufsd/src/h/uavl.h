// <copyright file="uavl.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// avl intrusive tree
//

namespace UFSD {

enum avl_result{ // == TABLE_SEARCH_RESULT
  AvlEmptyTree      = 0,
  AvlFoundNode      = 1,
  AvlInsertAsLeft   = 2,
  AvlInsertAsRight  = 3
};

#define UFSD_USE_COMPACT_AVL  "Activate this define to use compact variant of avl"

struct avl_link{
#ifdef UFSD_USE_COMPACT_AVL

  size_t    parent;   // 0x00
  avl_link* links[2]; // 0x08/0x10: index 0 == links[0], index 1 == links[1]

  #define avl_get_balance( n )      ((int)((n)->parent & 3) - 1)
  #define avl_get_parent( n )       ((avl_link*)((n)->parent & ~3))
  #define avl_set_balance( n, b )   (n)->parent = ((n)->parent & ~3) | ((b) + 1)
  #define avl_set_parent( n, p )    (n)->parent = (size_t)(p) | (((n)->parent & 3))

#else

  avl_link* parent;   // 0x00
  avl_link* links[2]; // 0x08/0x10: index 0 == links[0], index 1 == links[1]

//  avl_link* left;     // 0x08
//  avl_link* right;    // 0x10
  int       balance;  // 0x18: == -1/0/+1: TODO: merge with parent
//#if __WORDSIZE > 32
//  char      Res[sizeof(size_t) - sizeof(int)];  // to align
//#endif

  #define avl_get_balance( n )      (n)->balance
  #define avl_get_parent( n )       (n)->parent
  #define avl_set_balance( n, b )   (n)->balance  = b
  #define avl_set_parent( n, p )    (n)->parent   = p

#endif

  avl_link* next() const;
  avl_link* prev() const;

  avl_link* next_ex(
      IN OUT avl_result* res
      ) const
  {
    const avl_link* node = AvlFoundNode == *res || AvlInsertAsRight == *res? next() : this;

    if ( NULL != node )
      *res = AvlFoundNode;
    return (avl_link*)node;
  }

  avl_link* prev_ex(
      IN OUT avl_result* res
      ) const
  {
    const avl_link* node = AvlFoundNode == *res || AvlInsertAsLeft == *res? prev() : this;
    if ( NULL != node )
      *res = AvlFoundNode;
    return (avl_link*)node;
  }

};

#ifdef UFSD_USE_COMPACT_AVL
C_ASSERT( 0 == (sizeof(avl_link) % sizeof(size_t)) );
#endif


struct avl_tree
{
  avl_link      Root;         // 0x00
  size_t        Count;        // 0x20
  size_t        DeleteCount;  // 0x24
  size_t        DepthOfTree;  // 0x28

  avl_tree()
  {
    init();
  }

  void init()
  {
#ifdef UFSD_USE_COMPACT_AVL
    assert( 0 == ((size_t)&Root & 3) );
    Root.parent = (size_t)&Root | 1;
#else
    Root.parent   = &Root;  // avl_set_parent( &Root, &Root );
    Root.balance  = 0;      // avl_set_balance( &Root, 0 );
#endif
    Root.links[0] = Root.links[1] = NULL;
    Count         = 0;
    DeleteCount   = 0;
    DepthOfTree   = 0;
  }

  bool is_empty() const
  {
    bool bRet = 0 == Count;
    assert( bRet == ( NULL == Root.links[1] ) );
    return bRet;
  }

  avl_link* first() const;
  avl_link* last() const;

  // Finds node using compare function
  avl_result lookup_ex(
      IN int (*cmp_func)(
          IN const void*  expr,
          IN const void*  data,
          IN void*        arg
          ),
      IN const void*  expr,
      IN void*        arg,
      OUT avl_link**  node
      ) const;


  // Insert node after lookup_ex
  void  insert_ex(
      IN avl_link*  node,
      IN avl_link*  node_ex,  // from lookup_ex
      IN avl_result res_ex    // from lookup_ex
      )
  {
    node->links[0] = node->links[1] = NULL;
    avl_set_balance( node, 0 );
    if ( AvlEmptyTree == res_ex )
    {
      Root.links[1] = node;
      DepthOfTree   = 1;
      Count = 1;
      avl_set_parent( node, &Root );
    }
    else
    {
      node_ex->links[AvlInsertAsLeft == res_ex? 0 : 1] = node;
      avl_set_parent( node, node_ex );
      insert_color( node, node_ex );
    }
  }


  // Finds node using compare function
  avl_link* lookup(
      IN int (*cmp_func)(
          IN const void*  expr,
          IN const void*  data,
          IN void*        arg
          ),
      IN const void* expr,
      IN void*       arg
      ) const
  {
    DEBUG_ONLY( size_t found = 0; )
    avl_link* node = Root.links[1];

    while( NULL != node )
    {
      assert( ++found <= DepthOfTree );
      int cmp = (*cmp_func)( expr, node + 1, arg );
      if ( 0 == cmp )
        return node;
      node = node->links[ cmp < 0 ? 0 : 1 ];
    }

    return NULL;
  }

  // Insert node using compare function
  avl_link* insert(
      IN avl_link*  node,
      IN int (*cmp_func)(
          IN const void*  expr,
          IN const void*  data,
          IN void*        arg
          ),
      IN const void* expr,
      IN void*       arg
      )
  {
    avl_link* pr = Root.links[1];

    node->links[0] = node->links[1] = NULL;
    avl_set_balance( node, 0 );

    if ( NULL == pr )
    {
      Root.links[1] = node;
      DepthOfTree   = 1;
      Count = 1;

      avl_set_parent( node, &Root );
      return node;
    }

    DEBUG_ONLY( size_t found = 0; )

    for ( ;; )
    {
      assert( ++found <= DepthOfTree );

      int cmp = (*cmp_func)( expr, pr + 1, arg );
      if ( 0 == cmp )
        return NULL;  // already exists

      avl_link** p = &pr->links[cmp < 0? 0 : 1];
      avl_link* t = *p;
      if ( NULL == t )
      {
        *p = node;
        avl_set_parent( node, pr );
        insert_color( node, pr );
        return node;
      }

      pr = t;
    }
  }

  avl_link* start_enum(
      IN avl_link*    node,
      IN int (*cmp_func)(
          IN const void*  expr,
          IN const void*  data,
          IN void*        arg
          ),
      IN const void*  expr,
      IN void*        arg,
      IN bool         bForward,
      IN bool         bAutoInc,
      IN unsigned     delCount
      );

  void remove(
      IN avl_link* node
      );

  void insert_color(
      IN avl_link*  node,
      IN avl_link*  new_p
      );

  void swap(
      IN avl_tree* tree
      );

  //
  // Useful way to free the tree
  // for ( avl_link* node = Tree.ToList(); NULL != node; )
  // {
  //   avl_link* next = node->links[1];
  //   free( node );
  //   node = next;
  // }
  // assert( Tree.is_empty() );
  //
  avl_link* ToList();

};


///////////////////////////////////////////////////////////
// avl_for_each
//
// iterate over tree
///////////////////////////////////////////////////////////
#define avl_for_each(pos, tree) \
  for ( pos = (tree)->first(); NULL != pos; pos = pos->next() )


///////////////////////////////////////////////////////////
// avl_for_each_final
//
// Final iterate over tree
///////////////////////////////////////////////////////////
#define avl_for_each_final(pos, n, tree) \
  for ( pos = (tree)->ToList(); n = NULL != pos? pos->links[1] : NULL, NULL != pos; pos = n )


//
// Template 'C++' version of avl_link includes 'avl_lookup' and 'avl_insert'
//
template <class K>
struct avl_linkT : public avl_link
{
  K key;
};


///////////////////////////////////////////////////////////
// avl_lookup
//
// Returns node->key == key
///////////////////////////////////////////////////////////
template <class K>
static inline
avl_linkT<K>*
avl_lookup(
    IN const avl_tree*  tree,
    IN K          key
    )
{
  DEBUG_ONLY( size_t found = 0; )
  avl_linkT<K>* node = (avl_linkT<K>*)tree->Root.links[1];

  while( NULL != node )
  {
    assert( ++found <= tree->DepthOfTree );
    K n_key = ((avl_linkT<K>*)node)->key;
    if ( key == n_key )
      return node;
    node = (avl_linkT<K>*)node->links[ key < n_key? 0 : 1];
  }

  return NULL;
}


///////////////////////////////////////////////////////////
// lookup_ex
//
// Returns node->key == key or place to insert
///////////////////////////////////////////////////////////
template <class K>
static inline
avl_result
lookup_ex(
    IN const avl_tree*  tree,
    IN K                key,
    OUT avl_linkT<K>**  node
    )
{
  avl_linkT<K>* n = (avl_linkT<K>*)tree->Root.links[1];

  if ( NULL == n )
  {
    *node = NULL;
    return AvlEmptyTree;
  }

  DEBUG_ONLY( size_t found = 0; )

  for ( ;; )
  {
    assert( ++found <= tree->DepthOfTree );

    K n_key = ((avl_linkT<K>*)n)->key;
    if ( key == n_key )
    {
      *node = n;
      return AvlFoundNode;
    }

    avl_linkT<K>* child;

    if ( key < n_key )
    {
      child = (avl_linkT<K>*)n->links[0];
      if ( NULL == child )
      {
        *node = n;
        return AvlInsertAsLeft;
      }
    }
    else
    {
      child = (avl_linkT<K>*)n->links[1];
      if ( NULL == child )
      {
        *node = n;
        return AvlInsertAsRight;
      }
    }
    n = child;
  }
}


///////////////////////////////////////////////////////////
// avl_lookup_p
//
// Returns node->key <= key
///////////////////////////////////////////////////////////
template <class K>
static inline
avl_linkT<K>*
avl_lookup_p(
    IN const avl_tree*  tree,
    IN K          key
    )
{
  avl_linkT<K>* node  = (avl_linkT<K>*)tree->Root.links[1];
  avl_linkT<K>* r     = NULL;

  while( NULL != node )
  {
    K n_key = node->key;
    if ( key < n_key )
      node = (avl_linkT<K>*)node->links[0];
    else if ( key > n_key )
    {
      r = node;
      node = (avl_linkT<K>*)node->links[1];
    }
    else
      return node;
  }

  return r;
}


///////////////////////////////////////////////////////////
// avl_lookup_n
//
// Returns node->key >= key
///////////////////////////////////////////////////////////
template <class K>
static inline
avl_linkT<K>*
avl_lookup_n(
    IN avl_tree*  tree,
    IN K          key
    )
{
  avl_linkT<K>* node  = (avl_linkT<K>*)tree->Root.links[1];
  avl_linkT<K>* r     = NULL;

  while( NULL != node )
  {
    K n_key = node->key;
    if ( key < n_key )
    {
      r = node;
      node = (avl_linkT<K>*)node->links[0];
    }
    else if ( n_key < key )
      node = (avl_linkT<K>*)node->links[1];
    else
      return node;
  }

  return r;
}


///////////////////////////////////////////////////////////
// avl_insert
//
//
///////////////////////////////////////////////////////////
template <class K>
static inline
avl_linkT<K>*
avl_insert(
    IN avl_tree*      tree,
    IN avl_linkT<K>*  node // only n->key is used as input, other fields will be filled
    )
{
  avl_link* pr = tree->Root.links[1];

  node->links[0] = NULL;
  node->links[1] = NULL;
  avl_set_balance( node, 0 );

  if ( NULL == pr )
  {
    tree->Root.links[1] = node;
    tree->DepthOfTree   = 1;
    tree->Count         = 1;
    avl_set_parent( node, &tree->Root );
    return node;
  }

  DEBUG_ONLY( size_t found = 0; )
  const K& key = node->key;

  for ( ;; )
  {
    assert( ++found <= tree->DepthOfTree );
    K n_key = ((avl_linkT<K>*)pr)->key;
    if ( n_key == key )
      return NULL;   // already exists
    avl_link** p  = &pr->links[ key < n_key? 0 : 1 ];
    avl_link* t   = *p;
    if ( NULL == t )
    {
      *p = node;
      avl_set_parent( node, pr );
      tree->insert_color( node, pr );
      return node;
    }

    pr = t;
  }
}

// A bit complicated containing record computation
// used to cheat with GCC detection of NULL pointer reference.
#define avl_entry(ptr, type, member) \
  ((type *)((char*)(ptr) - ((char*)&(((type *)(sizeof(type)))->member) - sizeof(type))))


typedef avl_linkT<unsigned int> avl_link32;
typedef avl_linkT<UINT64>       avl_link64;

} // namespace UFSD



#if 0
typedef enum _RTL_GENERIC_COMPARE_RESULTS {
  GenericLessThan = 0,
  GenericGreaterThan = 1,
  GenericEqual = 2
} RTL_GENERIC_COMPARE_RESULTS;

enum MS_CREATE_DISPOSITION{
  MS_CREATE_NEW           = 0,
  MS_CREATE_DISPOSITION_1 = 1,
  MS_OPEN_EXISTING        = 2
};

#endif

