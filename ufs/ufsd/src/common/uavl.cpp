// <copyright file="uavl.cpp" company="Paragon Software Group">
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

#include "../h/versions.h"

//#if defined UFSD_NTFS || defined UFSD_EXFAT || defined UFSD_FAT || defined UFSD_HFS || defined UFSD_REFS || defined UFSD_REFS3

#include <ufsd.h>
#include "../h/assert.h"
#include "../h/uavl.h"

using namespace UFSD;

#ifndef NDEBUG
static const unsigned int WorstCaseFill[] = {
  0x00000, 0x00001, 0x000002, 0x000004, 0x000007, 0x00000C, 0x00014, 0x00021, 0x00036,
  0x00058, 0x0008F, 0x0000E8, 0x000178, 0x000261, 0x0003DA, 0x0063C, 0x00A17, 0x01054,
  0x01A6C, 0x02AC1, 0x00452E, 0x006FF0, 0x00B51F, 0x012510, 0x1DA30, 0x2FF41, 0x4D972,
  0x7D8B4, 0xCB227, 0x148ADC, 0x213D04, 0x35C7E1, 0x5704E6,
};

C_ASSERT( 33 == ARRSIZE( WorstCaseFill ) );

static const unsigned int BestCaseFill[] = {
  0x00000, 0x00001, 0x000003, 0x000007, 0x00000F, 0x00001F, 0x0003F, 0x0007F, 0x000FF,
  0x001FF, 0x003FF, 0x0007FF, 0x000FFF, 0x001FFF, 0x003FFF, 0x07FFF, 0x0FFFF, 0x1FFFF,
  0x3FFFF, 0x7FFFF, 0x0FFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF,
  0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

C_ASSERT( 33 == ARRSIZE( BestCaseFill ) );

//
//   for ( unsigned int i = 2; i < ARRSIZE( WorstCaseFill ); i++ )
//   {
//     assert( WorstCaseFill[i] == WorstCaseFill[i-1] + WorstCaseFill[i-2]+1 );
//   }


#endif


///////////////////////////////////////////////////////////
// promote
//
//
///////////////////////////////////////////////////////////
static void
promote(
    IN avl_link* node
    )
{
  avl_link* p0   = avl_get_parent( node );
  avl_link* p00  = avl_get_parent( p0 );

  if ( p0->links[0] == node )
  {
    p0->links[0] = node->links[1];
    if ( NULL != p0->links[0] )
      avl_set_parent( p0->links[0], p0 );
    node->links[1] = p0;
  }
  else
  {
    assert( p0->links[1] == node );
    p0->links[1] = node->links[0];
    if ( NULL != p0->links[1] )
      avl_set_parent( p0->links[1], p0 );
    node->links[0] = p0;
  }

  avl_set_parent( p0, node );
  if ( p00->links[0] == p0 )
  {
    p00->links[0] = node;
  }
  else
  {
    assert( p00->links[1] == p0 );
    p00->links[1] = node;
  }

  avl_set_parent( node, p00 );
}


///////////////////////////////////////////////////////////
// rebalance
//
//
///////////////////////////////////////////////////////////
static bool
rebalance(
    IN avl_link* node
    )
{
  int balance = avl_get_balance( node );
  assert( -1 <= balance && balance <= 1 );
  avl_link* next = 1 == balance? node->links[1] : node->links[0];
  if ( balance == avl_get_balance( next ) )
  {
    promote( next );
    avl_set_balance( next, 0 );
    avl_set_balance( node, 0 );
    return false;
  }

  if ( avl_get_balance( next ) == -balance )
  {
    avl_link* next2 = 1 == balance? next->links[0] : next->links[1];
    promote( next2 );
    promote( next2 );
    avl_set_balance( node, 0 );
    avl_set_balance( next, 0 );

    int b2 = avl_get_balance( next2 );
    if ( b2 == balance )
      avl_set_balance( node, -balance );
    else if ( b2 == -balance )
      avl_set_balance( next, balance );

    avl_set_balance( next2, 0 );
    return false;
  }

  promote( next );
  avl_set_balance( next, -balance );
  return true;
}


///////////////////////////////////////////////////////////
// avl_tree::search_idx
//
//
///////////////////////////////////////////////////////////
avl_link*
avl_tree::first() const
{
  if ( is_empty() )
    return NULL;

  avl_link* n = Root.links[1];
  for( ;; )
  {
    avl_link* child = n->links[0];
    if ( NULL == child )
      return n;
    n = child;
  }
}


///////////////////////////////////////////////////////////
// avl_tree::last
//
//
///////////////////////////////////////////////////////////
avl_link*
avl_tree::last() const
{
  if ( is_empty() )
    return NULL;

  avl_link* n = Root.links[1];
  for( ;; )
  {
    avl_link* child = n->links[1];
    if ( NULL == child )
      return n;
    n = child;
  }
}


///////////////////////////////////////////////////////////
// avl_link::prev
//
//
///////////////////////////////////////////////////////////
avl_link*
avl_link::prev() const
{
  const avl_link* p = links[0];

  if ( NULL != p )
  {
    while( NULL != p->links[1] )
      p = p->links[1];
    return (avl_link*)p;
  }

  const avl_link* pr;

  for ( p = this; ; p = pr )
  {
    pr = avl_get_parent( p );
    if ( pr->links[0] != p )
    {
      if ( pr->links[1] == p && avl_get_parent( pr ) != pr )
        return (avl_link*)pr;

      return NULL;
    }
  }
}


///////////////////////////////////////////////////////////
// avl_link::next
//
//
///////////////////////////////////////////////////////////
avl_link*
avl_link::next() const
{
  const avl_link* p = links[1];
  if ( NULL != p )
  {
    while( NULL != p->links[0] )
      p = p->links[0];
    return (avl_link*)p;
  }

  const avl_link* pr;

  for ( p = this; ; p = pr )
  {
    pr = avl_get_parent( p );
    if ( pr->links[1] != p )
    {
      if ( pr->links[0] == p )
        return (avl_link*)pr;

      assert( pr == p );
      return NULL;
    }
  }
}


///////////////////////////////////////////////////////////
// avl_tree::lookup_ex
//
// Finds node using compare function
///////////////////////////////////////////////////////////
avl_result
avl_tree::lookup_ex(
    IN int (*cmp_func)(
        IN const void*  expr,
        IN const void*  data,
        IN void*        arg
        ),
    IN const void*  expr,
    IN void*        arg,
    OUT avl_link**  node
    ) const
{
  if ( is_empty() )
  {
    *node = NULL;
    return AvlEmptyTree;
  }

  DEBUG_ONLY( size_t found = 0; )
  avl_link* n = Root.links[1];

  for ( ;; )
  {
    assert( ++found <= DepthOfTree );
    int cmp = (*cmp_func)( expr, n + 1, arg );
    if ( 0 == cmp )
    {
      *node = n;
      return AvlFoundNode;
    }

    avl_link* child;

    if ( cmp < 0 )
    {
      child = n->links[0];
      if ( NULL == child )
      {
        *node = n;
        return AvlInsertAsLeft;
      }
    }
    else
    {
      child = n->links[1];
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
// avl_tree::insert_color
//
//
///////////////////////////////////////////////////////////
void
avl_tree::insert_color(
    IN avl_link*  node,
    IN avl_link*  new_p
    )
{
  Count += 1;

  avl_link* node2  = node;
  avl_set_balance( &Root, -1 );
  for ( ;; )
  {
    int NewBalance = avl_get_parent( node2 )->links[0] == node2? -1 : 1;

    if ( 0 == avl_get_balance( new_p ) )
    {
      avl_set_balance( new_p, NewBalance );
      node2 = new_p;
      new_p = avl_get_parent( new_p );
      continue;
    }

    int new_b = avl_get_balance( new_p );
    assert( -1 <= new_b && new_b <= 1 );
    if ( new_b != NewBalance )
    {
      avl_set_balance( new_p, 0 );
      if ( 0 == avl_get_balance( &Root ) )
        DepthOfTree += 1;
      break;
    }
    rebalance( new_p );
    break;
  }

  assert( Count >= WorstCaseFill[DepthOfTree] && Count <= BestCaseFill[DepthOfTree] );
}


///////////////////////////////////////////////////////////
// avl_tree::remove
//
//
///////////////////////////////////////////////////////////
void
avl_tree::remove(
    IN avl_link* node
    )
{
  avl_link* n;
  if ( NULL == node->links[0] || NULL == node->links[1] )
  {
    n = node;
  }
  else if ( avl_get_balance( node ) >= 0 )
  {
    n = node->links[1];
    while( NULL != n->links[0] )
      n = n->links[0];
  }
  else
  {
    n = node->links[0];
    while( NULL != n->links[1] )
      n = n->links[1];
  }

  int balance = -1;

  avl_link* NewParent = avl_get_parent( n );

  if ( NULL == n->links[0] )
  {
    if ( NewParent->links[0] == n )
      NewParent->links[0] = n->links[1];
    else
    {
      NewParent->links[1] = n->links[1];
      balance = 1;
    }
    if ( NULL != n->links[1] )
      avl_set_parent( n->links[1], NewParent );
  }
  else
  {
    if ( NewParent->links[0] == n )
      NewParent->links[0] = n->links[0];
    else
    {
      NewParent->links[1] = n->links[0];
      balance = 1;
    }

    avl_set_parent( n->links[0], NewParent );
  }

  avl_set_balance( &Root, 0 );
  assert( NewParent == avl_get_parent( n ) );

  for ( ;; )
  {
    if ( avl_get_balance( NewParent ) == balance )
    {
      avl_set_balance( NewParent, 0 );
    }
    else
    {
      if ( 0 == avl_get_balance( NewParent ) )
      {
        assert( -1 <= balance && balance <= 1 );
        avl_set_balance( NewParent, -balance );
        if ( 0 != avl_get_balance( &Root ) )
          DepthOfTree -= 1;
        break;
      }
      if ( rebalance( NewParent ) )
        break;
      NewParent = avl_get_parent( NewParent );
    }

    balance = -1;
    avl_link* pr = avl_get_parent( NewParent );
    if ( pr->links[1] == NewParent )
      balance = 1;
    NewParent = pr;
  }

  if ( n != node )
  {
#ifndef BASE_USE_BUILTIN_MEMCPY
    n->parent   = node->parent;
    n->links[0] = node->links[0];
    n->links[1] = node->links[1];
//    n->balance  = node->balance;
#else
    Memcpy2( n, node, sizeof(avl_link) );
#endif

    avl_link* pr = avl_get_parent( node );

    if ( pr->links[0] == node )
    {
      pr->links[0] = n;
    }
    else
    {
      assert( pr->links[1] == node );
      pr->links[1] = n;
    }

    if ( NULL != n->links[0] )
      avl_set_parent( n->links[0], n );
    if ( NULL != n->links[1] )
      avl_set_parent( n->links[1], n );
  }

  DeleteCount += 1;
  Count -= 1;

  assert( Count >= WorstCaseFill[DepthOfTree] && Count <= BestCaseFill[DepthOfTree] );
}


///////////////////////////////////////////////////////////
// avl_tree::start_enum
//
//
///////////////////////////////////////////////////////////
avl_link*
avl_tree::start_enum(
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
    )
{
  if ( is_empty() )
    return NULL;

  if ( delCount != DeleteCount )
    node = NULL;

  if ( NULL == node )
  {
    node = Root.links[1];
    DEBUG_ONLY( size_t found = 0; )

    for( ;; )
    {
      int cmp = (*cmp_func)( expr, node + 1, arg );
      assert( ++found <= DepthOfTree );
      if ( 0 == cmp )
        break; // exactly

      avl_link* child;
      if ( cmp < 0 )
      {
        child = node->links[0];
        if ( NULL == child )
          return !bForward? node->prev() : node;
      }
      else
      {
        child = node->links[1];
        if ( NULL == child )
          return bForward? node->next() : node;
      }
      node = child;
    }
  }

  if ( bAutoInc )
  {
    assert( NULL != node );
    node = bForward? node->next() : node->prev();
  }

  return node;
}


///////////////////////////////////////////////////////////
// avl_tree::ToList
//
//
///////////////////////////////////////////////////////////
avl_link*
avl_tree::ToList()
{
  avl_link* next = NULL;
  for ( avl_link* node = last(); NULL != node; node = node->prev() )
  {
    node->links[1] = next;
    next = node;
  }
  init();
  return next;
}


#ifdef UFSD_REFS3
///////////////////////////////////////////////////////////
// avl_tree::swap
//
//
///////////////////////////////////////////////////////////
void
avl_tree::swap(
    IN avl_tree* tree
    )
{
  avl_link* t1 = Root.links[1];
  avl_link* t2 = tree->Root.links[1];

  Root.links[1] = t2;
  if ( NULL != t2 )
    avl_set_parent( t2, &Root );

  tree->Root.links[1] = t1;
  if ( NULL != t1 )
    avl_set_parent( t1, &tree->Root );

  // Check this case!!
  assert( NULL == Root.links[0] );
  assert( NULL == tree->Root.links[0] );

#ifndef UFSD_USE_COMPACT_AVL
  int i32 = Root.balance;
  Root.balance = tree->Root.balance;
  tree->Root.balance = i32;
#endif

  size_t u32 = Count;
  Count = tree->Count;
  tree->Count = u32;

  u32 = DeleteCount;
  DeleteCount = (tree->DeleteCount > DeleteCount? tree->DeleteCount : DeleteCount) + 1;
  tree->DeleteCount = u32;

  u32 = DepthOfTree;
  DepthOfTree = tree->DepthOfTree;
  tree->DepthOfTree = u32;
}
#endif

//#endif //#if defined UFSD_NTFS || defined UFSD_EXFAT || defined UFSD_FAT || defined UFSD_HFS || defined UFSD_REFS || defined UFSD_REFS3

#ifdef _MODULE_TEST
#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#define _CRT_SECURE_NO_WARNINGS

extern "C"
unsigned __stdcall
GetTickCount();

extern "C"
{
  __declspec( dllimport ) void  __stdcall OutputDebugStringA( IN const char* );
  void ufsd_trace( const char* fmt, ... )
  {
    va_list arglist;
    va_start( arglist, fmt );

    char szBuffer[1024];
    _vsnprintf( szBuffer, ARRSIZE(szBuffer), fmt, arglist );
    szBuffer[ARRSIZE(szBuffer)-1] = 0;

    OutputDebugStringA( szBuffer );
    va_end( arglist );
  }

  void __cdecl
  ufsd_vtrace( const char *fmt, va_list arglist )
  {
  }

  void ufsd_error( int , const char *, int )
  {
    assert( 0 );
  }
}


using namespace UFSD;

#undef _MODULE_TEST

api::IBaseMemoryManager* UFSD_GetMemoryManager();
api::IBaseMemoryManager* m_Mm;

#include "urbtree.cpp"

#if 0
namespace REFS{
#include "uavl_refs.cpp"

static int
REFS_AVL_COMPARE( const void* expression, const void* entry, void* )
{
  unsigned e = *(unsigned*)expression;
  unsigned d = *(unsigned*)entry;
  if ( e > d )
    return +1;
  if ( e < d )
    return -1;
  return 0;
}

void*
REFS_AVL_ALLOCATE( size_t Bytes, void * )
{
  return Malloc2( Bytes );
}


REFS::avl_func AvlFuncs = {
  // 0x00
  &REFS_AVL_COMPARE,
  // 0x08
  &REFS_AVL_ALLOCATE
};
#endif


void __cdecl main()
{
  m_Mm = UFSD_GetMemoryManager();
  unsigned TickCount = GetTickCount();

  rb_node32* rb_root = NULL;
  avl_tree  avl_new;
//  avl_tree  avl_refs;
//  avl_refs.init();

  time_t t = 0x567958d5;
//  time_t t = time( NULL );
//  time_t t = 0;
//  if ( 0 != t )
//    srand( t );

  for ( size_t m = 0; ; m++ )
  {
    printf( "%u: %x\n", m, (unsigned)t );
#define NCOUNT 100000
    unsigned* arr = (unsigned*)malloc( NCOUNT * sizeof(unsigned) );
    unsigned i;

    for ( i = 0; i < NCOUNT; i++ )
    {
      arr[i] = abs( rand() );
      unsigned k = arr[i];

      rb_node32* rb = (rb_node32*)Malloc2( sizeof(rb_node32) );
      rb->key = k;

      rb_node32* rb_ins = rb_insert( &rb_root, rb );
      if ( NULL == rb_ins )
      {
        // already exist
        Free2( rb );
      }

//      assert( 1 != i );
      avl_link32* avl = (avl_link32*)Malloc2( sizeof(avl_link32) );
      avl->key = k;
      avl_link32* avl_ins = avl_insert( &avl_new, avl );
      if ( NULL == avl_ins )
      {
        Free2( avl_ins );
      }
      assert( (NULL == rb_ins) == ( NULL == avl_ins ) );

//      assert( avl_refs.Count == avl_new.Count );
//      assert( avl_refs.DepthOfTree == avl_new.DepthOfTree );
      assert( (NULL == rb_ins) == ( NULL == avl_ins ) );

      rb_node32* n = (rb_node32*)rb_node32::first( rb_root );
      avl_link32* a_new = (avl_link32*)avl_new.first();

      while( NULL != n )
      {
        assert( n->key == a_new->key );
//#endif

        n = (rb_node32*)n->next();
        a_new = (avl_link32*)a_new->next();
      }

      assert( NULL == a_new );
//#endif
    }

    for ( i = 0; i < NCOUNT; i++ )
    {
      unsigned k    = arr[i];
      rb_node32* rb = rb_lookup( rb_root, k );

#if 0
      avl_link* a_ok = NULL;
      avl_result bFound = avl_refs.find( &AvlFuncs, NULL, &k, &a_ok );
      if ( AvlFoundNode != bFound )
      {
        a_ok = NULL;
      }
#endif

      avl_link32* a_new  = avl_lookup( &avl_new, k );

      if ( NULL == rb )
      {
//        assert( NULL == a_ok );
        assert( NULL == a_new );
      }
      else
      {
        assert( NULL != a_new );
        rb_remove( &rb_root, rb );
        Free2( rb );

#if 0
        avl_refs.delete_node_ex( a_ok );
        Free2( a_ok );
#endif

        avl_new.remove( a_new );
        Free2( a_new );
      }
    }

    assert( NULL == rb_root );
//    assert( 0 == avl_refs.Count );
    assert( 0 == avl_new.Count );

//    t = time( NULL );
    t = (time_t)((size_t)t + 1);
    srand( (unsigned)t );
  }
}

#endif //_MODULE_TEST


