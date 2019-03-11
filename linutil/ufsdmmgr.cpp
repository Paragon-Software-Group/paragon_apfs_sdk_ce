// <copyright file="ufsdmmgr.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file implements CMemoryManager for UFSD library
//
////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#if defined _WIN32

  #include <windows.h>

  // Let next functions be always inline
  #pragma intrinsic(memcpy, memcmp, memset)

  #ifdef NDEBUG
    #undef assert
    #define assert(e) do {} while((void)0,0)
    #define verify(f)     ((void)(f))
  #else
    #ifndef __MINGW32__
    #include <crtdbg.h>
    #endif
    //
    // For debug purpose only
    //
    //#define USE_HEAP
    //#define TRACK_ALLOC

    //#define ASSERT_ALLOC( hdr )   assert( 0x299 != hdr->cnt );

    #if 0
      #ifdef USE_HEAP
        #define CHECK_MEMORY()  assert( HeapValidate( m_Heap, 0, NULL ) );
      #else
        #define CHECK_MEMORY()  assert( _CrtCheckMemory() );
      #endif //ifdef USE_HEAP
      #pragma message ("heap check is activated")
    #endif

    #undef assert

    #ifdef __MINGW32__
      #define assert(e) do {} while((void)0,0)
    #elif _MSC_VER >= 1400
      #define assert(e) do { if (!(0,e) ){ __debugbreak(); }} while((void)0,0)
    #else
      #define assert(e) do { if (!(0,e) ){ __asm int 3 }} while((void)0,0)
    #endif

    #define verify(f)     assert(f)
    #define UFSD_TRACE
  #endif //ifdef NDEBUG

#else

  #include <assert.h>
  #include <unistd.h>
  #if defined __APPLE__
    #include <sys/sysctl.h>
  #elif defined __QNX__
    #include <sys/syspage.h>
  #elif ! defined __FreeBSD__
    #include <sys/sysinfo.h>
  #endif

  #ifdef NDEBUG
    #undef UFSD_TRACE
  #else
    //#define TRACK_ALLOC
  #endif

#endif //if defined _WIN32

#ifndef ASSERT_ALLOC
  #define ASSERT_ALLOC( hdr )
#endif

#ifndef CHECK_MEMORY
  #define CHECK_MEMORY()
#endif

#ifndef NDEBUG
  #define DEBUG_ONLY(e) e
#else
  #define DEBUG_ONLY(e)
#endif

// Include UFSD specific files
#include <ufsd.h>
#include "funcs.h"

#if !defined UFSDTrace && !defined NDEBUG

  EXTERN_C void ufsd_vtrace( const char *fmt, va_list ap );
  #ifdef __GNUC__
  static __inline void ufsd_trace2( const char* fmt, ... )  __attribute__ ((format (printf, 1, 2)));
  #endif

  static __inline void ufsd_trace2( const char* fmt, ... )
  {
    va_list ap;
    va_start( ap, fmt );
    ufsd_vtrace( fmt, ap );
    va_end( ap );
  }
  #define UFSDTrace(a)        ufsd_trace2 a
#endif // #ifndef UFSDTrace

struct UFSD_MemoryManager : public api::IBaseMemoryManager
{
#ifndef NDEBUG
    unsigned  m_MemsetCnt, m_MemcpyCnt, m_MemmoveCnt, m_MemcmpCnt,
              m_MallocCnt, m_FreeCnt;
    size_t  m_MinRequest, m_MaxRequest;

    size_t  m_UsedMem, m_UsedMemMax;

#ifdef USE_HEAP
    HANDLE m_Heap;
#endif
#ifdef TRACK_ALLOC
    UFSD::list_head m_Allocs;
#endif

    UFSD_MemoryManager()
      : m_MemsetCnt(0)
      , m_MemcpyCnt(0)
      , m_MemmoveCnt(0)
      , m_MemcmpCnt(0)
      , m_MallocCnt(0)
      , m_FreeCnt(0)
      , m_MinRequest((size_t)-1)
      , m_MaxRequest(0)
      , m_UsedMem(0)
      , m_UsedMemMax(0)
    {
#ifdef USE_HEAP
      m_Heap = HeapCreate( HEAP_NO_SERIALIZE, 0, 0 );
#else
//      _CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | 0x2F );
#endif
#ifdef TRACK_ALLOC
      m_Allocs.init();
#endif
      CHECK_MEMORY();
    }

    ~UFSD_MemoryManager()
    {
      if ( (size_t)-1 != m_MinRequest )
      {
        unsigned Mb   = (unsigned)(m_UsedMemMax/(1024*1024));
        unsigned Kb   = (unsigned)((m_UsedMemMax%(1024*1024)) / 1024);
        unsigned b    = (unsigned)(m_UsedMemMax%1024);
        unsigned leak = (unsigned)(m_UsedMem > 0? m_UsedMem : 0);
        assert( 0 == leak );

        if ( 0 != Mb )
        {
          UFSDTrace(( "MemoryManager report: Peak usage %u.%03u Mb (%" PZZ "u bytes), leak %u bytes\n", Mb, Kb, m_UsedMemMax, leak ));
        }
        else
        {
          UFSDTrace(( "MemoryManager report: Peak usage %u.%03u Kb (%" PZZ "u bytes), leak %u bytes\n", Kb, b, m_UsedMemMax, leak ));
        }

        UFSDTrace(( "MemoryManager report: Min/MaxRequest %" PZZ "u/%" PZZ "u bytes\n", m_MinRequest, m_MaxRequest ));

        if ( m_MemsetCnt || m_MemcpyCnt || m_MemcmpCnt || m_MemmoveCnt || m_MallocCnt || m_FreeCnt )
          UFSDTrace(( "MemoryManager statistic:\n" ));

        if ( m_MemsetCnt )
          UFSDTrace(( "\tMemset   %u\n", m_MemsetCnt ));
        if ( m_MemcpyCnt )
          UFSDTrace(( "\tMemcpy   %u\n", m_MemcpyCnt ));
        if ( m_MemcmpCnt )
          UFSDTrace(( "\tMemcmp   %u\n", m_MemcmpCnt ));
        if ( m_MemmoveCnt )
          UFSDTrace(( "\tMemmove  %u\n", m_MemmoveCnt ));
        if ( m_MallocCnt )
          UFSDTrace(( "\tMalloc   %u\n", m_MallocCnt ));
        if ( m_FreeCnt )
          UFSDTrace(( "\tFree     %u\n", m_FreeCnt ));

#ifdef TRACK_ALLOC
        UFSD::list_head* pos, *n;
        list_for_each_safe( pos, n, &m_Allocs )
        {
          Hdr* h = list_entry( pos, Hdr, entry );
          UFSDTrace(( "**** leak (seq=0x%x, size=0x%x) ***", (unsigned)h->cnt, (unsigned)h->size ));
          pos->remove();
#ifndef USE_HEAP
          free( h );
#endif
        }
#endif
      }

#ifdef USE_HEAP
      verify( HeapDestroy( m_Heap ) );
#endif
    }

    //
    // Header aligned on 16 bytes
    // Align should not be less than 8 bytes!
    // Used when UFSD_TRACE is activated
    //
    struct Hdr{
      size_t    size;
      size_t    cnt;
#ifdef TRACK_ALLOC
      UFSD::list_head entry;
#else
      size_t    staff[2];
#endif
    };
#endif //ifndef NDEBUG

    //=============================================
    //    api::IBaseMemoryManager virtual functions
    //=============================================
    virtual void Memset(void * dst, unsigned char bVal, size_t count)
    {
      DEBUG_ONLY( m_MemsetCnt += 1; )
      memset( dst, bVal, count );
    }
    virtual void Memcpy( void *dst, const void *src, size_t count )
    {
      DEBUG_ONLY( m_MemcpyCnt += 1; )
      memcpy( dst, src, count );
    }
    virtual int Memcmp( const void *dst, const void *src, size_t count)
    {
      DEBUG_ONLY( m_MemcmpCnt += 1; )
      return memcmp( dst, src, count );
    }
    virtual void Memmove( void *dst, const void *src, size_t count )
    {
      DEBUG_ONLY( m_MemmoveCnt += 1; )
      memmove( dst, src, count );
    }
    virtual void* Malloc( size_t size, unsigned flags );
    virtual void Free( void* p );
    virtual void* Malloc( size_t size, const char*  , size_t, unsigned flags )
    {
      return Malloc( size, flags );
    }
    virtual void Destroy()
    {
    }

    //=============================================
    //    end of api::IBaseMemoryManager virtual functions
    //=============================================
};


///////////////////////////////////////////////////////////
// UFSD_MemoryManager::Malloc
//
//
///////////////////////////////////////////////////////////
void*
UFSD_MemoryManager::Malloc(
    IN size_t   size,
    IN unsigned flags
    )
{
#ifdef NDEBUG
  void* p = ::malloc( size );
  if ( NULL != p && FlagOn( flags, BASE_MEMORY_FLAG_ZERO ) )
    memset( p, 0, size );
  return p;
#else
//  assert( size > 2 );
  if ( size < sizeof(size_t) )
    size = sizeof(size_t);

  if ( size > m_MaxRequest )
    m_MaxRequest = size;
  if ( size < m_MinRequest )
    m_MinRequest = size;
  m_UsedMem += size;
  if ( m_UsedMemMax < m_UsedMem )
    m_UsedMemMax = m_UsedMem;

  CHECK_MEMORY();

  //
  // Allocate required memory + header
  //
#ifdef USE_HEAP
  Hdr* hdr = (Hdr*)HeapAlloc( m_Heap, HEAP_NO_SERIALIZE, size + sizeof(Hdr) );
#else
  Hdr* hdr = (Hdr*)malloc( size + sizeof(Hdr) );
#endif
  if ( NULL == hdr )
  {
    assert( 0 );
    UFSDTrace(( "Malloc( size=0x%x, flags=0x%x ). failed at %u. Used %u\n", (unsigned)size, flags, m_MallocCnt, (unsigned)m_UsedMem ));
    return NULL;
  }

  hdr->size = size;
  hdr->cnt  = m_MallocCnt;

#ifdef TRACK_ALLOC
  hdr->entry.insert_before( &m_Allocs );
  UFSDTrace(( "malloc(%x,%x) => %p (%x)\n", (unsigned)size, flags, hdr + 1, m_MallocCnt ));
//  UFSDTrace(( "malloc(%x,%x) => (%x)\n", (unsigned)size, flags, m_MallocCnt ));
#endif

  ASSERT_ALLOC( hdr );
  m_MallocCnt += 1;
  if ( FlagOn( flags, BASE_MEMORY_FLAG_ZERO ) )
    memset( hdr + 1, 0, size );
  return hdr + 1;
#endif //ifdef NDEBUG
}


///////////////////////////////////////////////////////////
// UFSD_MemoryManager::Free
//
//
///////////////////////////////////////////////////////////
void
UFSD_MemoryManager::Free(
    IN void* p
    )
{
#ifdef NDEBUG
  ::free( p );
#else
  if ( NULL == p )
    return;

  Hdr* hdr = (Hdr*)p - 1;
  ASSERT_ALLOC( hdr );

#ifdef TRACK_ALLOC
  hdr->entry.remove();
  UFSDTrace(( "free(%p) => (%x,%x)\n", hdr + 1, (unsigned)hdr->cnt, (unsigned)hdr->size ));
//  UFSDTrace(( "free() => (%x,%x)\n", (unsigned)hdr->cnt, (unsigned)hdr->size ));
#endif

  CHECK_MEMORY();

  size_t size = hdr->size;
//  memset( hdr, 0xfe, sizeof(Hdr) + size );

#ifdef USE_HEAP
  verify( HeapFree( m_Heap, HEAP_NO_SERIALIZE, hdr ) );
#else
  free( hdr );
#endif

  m_UsedMem -= size;
  m_FreeCnt += 1;
#endif //ifdef NDEBUG
}


// The only memory manager
static UFSD_MemoryManager s_MemMngr;

#ifdef STATIC_MEMORY_MANAGER

api::IBaseMemoryManager* api::StaticMemBased::m_Mm = &s_MemMngr;


#ifndef BASE_USE_BUILTIN_MEMCPY
///////////////////////////////////////////////////////////
// api::IBaseMemoryManager::Memset
//
//
///////////////////////////////////////////////////////////
void
api::IBaseMemoryManager::Memset(
    IN void*          dest,
    IN unsigned char  c,
    IN size_t         count
    )
{
  s_MemMngr.Memset( dest, c, count );
}


///////////////////////////////////////////////////////////
// api::IBaseMemoryManager::Memcpy
//
//
///////////////////////////////////////////////////////////
void
api::IBaseMemoryManager::Memcpy(
    IN void*          to,
    IN const void*    from,
    IN size_t         count
    )
{
  s_MemMngr.Memcpy( to, from, count );
}


///////////////////////////////////////////////////////////
// api::IBaseMemoryManager::Memcmp
//
//
///////////////////////////////////////////////////////////
int
api::IBaseMemoryManager::Memcmp(
    IN const void*    b1,
    IN const void*    b2,
    IN size_t         count
    )
{
  return s_MemMngr.Memcmp( b1, b2, count );
}
#endif


#ifndef BASE_USE_BUILTIN_MEMMOVE
///////////////////////////////////////////////////////////
// UFSD_MemoryManager::Memmove
//
// Copies count bytes of characters from 'from' to 'to'. If some regions of the source
// area and the destination overlap, Memmove ensures that the original source bytes
// in the overlapping region are copied before being overwritten
///////////////////////////////////////////////////////////
void
api::IBaseMemoryManager::Memmove(
  IN void*        to,
  IN const void*  from,
  IN size_t       count
  )
{
  s_MemMngr.Memmove( to, from, count );
}
#endif


///////////////////////////////////////////////////////////
// api::IBaseMemoryManager::Malloc
//
//
///////////////////////////////////////////////////////////
void*
api::IBaseMemoryManager::Malloc( size_t size, unsigned flags )
{
  return s_MemMngr.Malloc( size, flags );
}


///////////////////////////////////////////////////////////
// UFSD_MemoryManager::Free
//
//
///////////////////////////////////////////////////////////
void
api::IBaseMemoryManager::Free( void * p )
{
  s_MemMngr.Free( p );
}


#ifdef _DEBUG
///////////////////////////////////////////////////////////
// api::IBaseMemoryManager::Malloc
//
//
///////////////////////////////////////////////////////////
void*
api::IBaseMemoryManager::Malloc( size_t size, const char*, size_t, unsigned flags )
{
  return s_MemMngr.Malloc( size, flags );
}
#endif


///////////////////////////////////////////////////////////
// api::IBaseMemoryManager::Destroy
//
//
///////////////////////////////////////////////////////////
void
api::IBaseMemoryManager::Destroy()
{
  s_MemMngr.Destroy();
}

#endif // #ifdef STATIC_MEMORY_MANAGER


///////////////////////////////////////////////////////////
// UFSD_GetMemoryManager
//
// The API exported from this module.
// It returns the pointer to the static class.
///////////////////////////////////////////////////////////
api::IBaseMemoryManager*
UFSD_GetMemoryManager()
{
  return &s_MemMngr;
}


#if !defined _WIN32 && !defined __APPLE__
///////////////////////////////////////////////////////////
// operator 'new'
//
//
///////////////////////////////////////////////////////////
void*
operator new(size_t cb) throw() {
  return ::malloc(cb);
}


///////////////////////////////////////////////////////////
// operator 'delete'
//
//
///////////////////////////////////////////////////////////
void
operator delete(void* p) {
  ::free(p);
}

#if __cplusplus > 201103L
#include <cstddef>

void
operator delete(void* p, std::size_t s) {
  ::free(p);
}
#endif

#endif


///////////////////////////////////////////////////////////
// UFSD_HeapAlloc
//
//
///////////////////////////////////////////////////////////
void*
UFSD_HeapAlloc(
    IN unsigned long Size,
    IN int    Zero
    )
{
  void* p = ::malloc( Size );
  if ( NULL != p && Zero )
    memset( p, 0, Size );
  return p;
}


///////////////////////////////////////////////////////////
// UFSD_HeapFree
//
//
///////////////////////////////////////////////////////////
void
UFSD_HeapFree(
    IN void *p
    )
{
  ::free( p );
}
