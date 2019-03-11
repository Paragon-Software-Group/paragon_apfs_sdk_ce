// <copyright file="memory_mgm.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __MEMORY_MGM_H__
#define __MEMORY_MGM_H__

#pragma once

#include <api/types.hpp>

#if defined(__APPLE__) && !defined(KERNEL)
#include <stdlib.h>
#endif


//
// Use inline memset/memcpy/memcmp/memmove, if it possible
//

#ifndef BASE_NO_CRT_USE

#if defined _MSC_VER && _MSC_VER < 1400
  //
  // VC 8 ignores intrinsic functions
  //
  extern "C" {
    void *  __cdecl memset(void *, int, size_t);
    void *  __cdecl memcpy(void *, const void *, size_t);
    int     __cdecl memcmp(const void *buf1, const void *buf2, size_t count);
  }

  #pragma intrinsic(memcpy, memset, memcmp)
  #define Memcpy2  memcpy
  #define Memset2  memset
  #define Memcmp2  memcmp
  #define BASE_USE_BUILTIN_MEMCPY

#elif defined __GNUC__

  #define Memcpy2   __builtin_memcpy
  #define Memset2   __builtin_memset
  #define Memcmp2   __builtin_memcmp
  #define BASE_USE_BUILTIN_MEMCPY
  #if __GNUC__ >= 4 && !defined __i386__
    #define Memmove2  __builtin_memmove
    #define BASE_USE_BUILTIN_MEMMOVE
  #endif

#endif

#endif //#ifndef BASE_NO_CRT_USE


//#define STATIC_MEMORY_MANAGER
#ifdef STATIC_MEMORY_MANAGER
  #define DECLARE_FUNC(f)  static f
#else
  #define DECLARE_FUNC(f)  virtual f = 0
#endif

#if defined __GNUC__ && (__GNUC__ >= 5 || (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6))
  #define ATTRIBUTE_ALLOC_SIZE  __attribute__((alloc_size(1)))
#else
  #define ATTRIBUTE_ALLOC_SIZE
#endif


#define MEMORY_NOLIMIT ((UINT64)(PI64(-1)))


namespace api {

struct ILimitedMManager;

// Allocate and zero memory
#define BASE_MEMORY_FLAG_ZERO       0x01
// Allocate page aligned
//#define BASE_MEMORY_FLAG_PAGE_ALIGN 0x02

struct BASE_ABSTRACT_CLASS IBaseMemoryManager
{
    // Sets the first count chars of dest to the character c.
    DECLARE_FUNC(
    void Memset(
        IN void*         dest,
        IN unsigned char c,
        IN size_t        count
    ));

    // Copies count bytes of 'from' to 'to'. If the source and destination overlap,
    // the behavior of Memcpy is undefined.
    // Use Memmove to handle overlapping regions.
    DECLARE_FUNC(
    void Memcpy(
        IN void*         to,
        IN const void*   from,
        IN size_t        count
    ));

    // Compares the first count bytes of buf1 and buf2 and returns a value indicating
    // their relationship.
    // < 0  b1 less than b2
    //   0  b1 identical to b2
    // > 0  b1 greater than b2
    DECLARE_FUNC(
    int Memcmp(
        IN const void*   b1,
        IN const void*   b2,
        IN size_t        c
    ));

    // Copies count bytes of characters from 'from' to 'to'. If some regions of the source
    // area and the destination overlap, Memmove ensures that the original source bytes
    // in the overlapping region are copied before being overwritten
    DECLARE_FUNC(
    void Memmove(
        IN void*         to,
        IN const void*   from,
        IN size_t        count
    ));

    // Malloc returns a void pointer to the allocated space, or NULL if there is
    // insufficient memory available
    DECLARE_FUNC(
    void* Malloc(
        IN size_t size,
        IN unsigned flags = 0 // BASE_MEMORY_FLAG_XXX
    ) ATTRIBUTE_ALLOC_SIZE );

    // The Free function deallocates a memory block 'p' that was previously
    // allocated by a call to Malloc
    DECLARE_FUNC(
    void Free(
        IN void*  p
    ));

    // Debugging version for looking of the memory leak
    // Malloc returns a void pointer to the allocated space, or NULL if there is
    // insufficient memory available
    DECLARE_FUNC(
    void* Malloc(
        IN size_t       size,
        IN const char*  FileName,
        IN size_t       LineNumbar,
        IN unsigned     flags = 0 // BASE_MEMORY_FLAG_XXX
    ) ATTRIBUTE_ALLOC_SIZE );

    DECLARE_FUNC( void Destroy() );
};


struct BASE_ABSTRACT_CLASS IMemoryManager : public IBaseMemoryManager
{
    struct MemStat
    {
      size_t FreeCnt;
      size_t MallocCnt;
      size_t UsedMem;
      size_t UsedMemMax;
    };

#ifndef STATIC_MEMORY_MANAGER
    virtual int AddRef() = 0;
    virtual int Release() = 0;

    //check memory
    virtual int Check() = 0;

    //get current memory statistic
    virtual int GetStat(MemStat* pStat) = 0;

    //creating new MemoryManager
    virtual int CreateMemoryManager(
        IN unsigned int Flags,
        OUT IMemoryManager** Mm
    ) = 0;

    //creating new MemoryManager with additional limitation
    virtual int CreateMemoryManager(
        IN unsigned int Flags,
        IN const UINT64& RamLimit,
        OUT ILimitedMManager** Mm
    ) = 0;
#endif  //STATIC_MEMORY_MANAGER
};


struct BASE_ABSTRACT_CLASS ILimitedMManager : public IMemoryManager
{
    //Returns total available RAM for allocation
    //or MEMORY_NOLIMIT if no any limitation or not supported
    virtual UINT64 GetRamSize() const = 0;

    //Dynamically change memory limit
    //if new limit more over then currently allocated size
    //then any allocations will falls while enough memory will not be free
    //Limit -1 (MEMORY_NOLIMIT) means no limitation
    virtual void  SetMemoryLimit(const UINT64& Limit) = 0;
    virtual UINT64 GetMemoryLimit() const = 0;
};


#ifndef STATIC_MEMORY_MANAGER
  #ifndef Memset2
    #define Memcpy2  m_Mm->Memcpy
    #define Memset2  m_Mm->Memset
    #define Memcmp2  m_Mm->Memcmp
  #endif

  #ifndef Memzero2
    #define Memzero2(p,s) Memset2( (p), 0, (s) )
  #endif

  #ifndef Memmove2
    #define Memmove2  m_Mm->Memmove
  #endif

  #ifdef _DEBUG
    #define Malloc2( s )  m_Mm->Malloc( s, __FILE__, __LINE__, 0 )
    #define Zalloc2( s )  m_Mm->Malloc( s, __FILE__, __LINE__, BASE_MEMORY_FLAG_ZERO )
  #else
    #define Malloc2( s )  m_Mm->Malloc( s, 0 )
    #define Zalloc2( s )  m_Mm->Malloc( s, BASE_MEMORY_FLAG_ZERO )
  #endif
  #define Free2( p )      m_Mm->Free( p )

#else //STATIC_MEMORY_MANAGER
  #ifndef Memset2
    #define Memcpy2             api::IBaseMemoryManager::Memcpy
    #define Memset2             api::IBaseMemoryManager::Memset
    #define Memcmp2             api::IBaseMemoryManager::Memcmp
  #endif

  #ifndef Memzero2
    #define Memzero2(p,s)       Memset2( (p), 0, (s) )
  #endif

  #ifndef Memmove2
    #define Memmove2            api::IBaseMemoryManager::Memmove
  #endif

  #ifdef UFSD_HeapAlloc
    #define Malloc2( s )        UFSD_HeapAlloc( s, 0 )
    #define Zalloc2( s )        UFSD_HeapAlloc( s, 1 )
    #define Free2( p )          UFSD_HeapFree( p )
  #elif defined _DEBUG
    #define Malloc2( s )        api::IBaseMemoryManager::Malloc( s, __FILE__, __LINE__, 0 )
    #define Zalloc2( s )        api::IBaseMemoryManager::Malloc( s, __FILE__, __LINE__, BASE_MEMORY_FLAG_ZERO )
    #define Free2( p )          api::IBaseMemoryManager::Free( p )
  #else
    #define Malloc2( s )        api::IBaseMemoryManager::Malloc( s, 0 )
    #define Zalloc2( s )        api::IBaseMemoryManager::Malloc( s, BASE_MEMORY_FLAG_ZERO )
    #define Free2( p )          api::IBaseMemoryManager::Free( p )
  #endif
#endif  //STATIC_MEMORY_MANAGER


#ifdef STATIC_MEMORY_MANAGER
struct StaticMemBased
{
  static IBaseMemoryManager * m_Mm;

  //
  // Inplace operator new
  //
  static void* __cdecl operator new( size_t, void* p ) throw()
  {
    return p;
  }

  //
  // operator new for CMemBased derived classes with virtual functions
  //
  static void* __cdecl operator new( size_t cb, IBaseMemoryManager* ) throw()
  {
    return Zalloc2( cb );
  }

  //
  // operator delete for CMemBased derived classes
  //
  static void __cdecl operator delete(void* p)
  {
    Free2( p );
  }

  //=======================================================
  // a-la "C" runtime memory related functions
  //=======================================================
  static void* Malloc( size_t size ) ATTRIBUTE_ALLOC_SIZE
  {
    return Malloc2( size );
  }

  static void Free( void* p )
  {
    Free2( p );
  }

  //=======================================================
  // End of "C" runtime memory related functions
  //=======================================================
};
#endif //STATIC_MEMORY_MANAGER

//  Class with overloaded "new/delete" operators.
//  Class is used to allocate and free memory
//  using our own memory manager.
template<class T>
class CMemBased
#ifdef STATIC_MEMORY_MANAGER
  : public StaticMemBased
#endif
{
public:

#ifdef STATIC_MEMORY_MANAGER
  CMemBased( IBaseMemoryManager * ) {}

#else //STATIC_MEMORY_MANAGER

  IMemoryManager * m_Mm;

private:
  // Make compiler happy (just declare but not implement):
  CMemBased();
  CMemBased(const CMemBased&);
  CMemBased& operator=(const CMemBased&);
  // By default new[] and delete[] are empty
  // redefine in your class if necessary
  static void* __cdecl operator new[] (size_t) throw() { return NULL; }
  static void  __cdecl operator delete[] (void*) {}

protected:
  CMemBased( IMemoryManager * mm )
  : m_Mm(mm)
  {
    //if(m_Mm) m_Mm->AddRef();
  }

  ~CMemBased()
  {
    //if(m_Mm) m_Mm->Release();
  } // non virtual!

public:

  IMemoryManager* GetMemoryManager() const
  {
    return m_Mm;
  }

  //
  // Inplace operator new
  //
  static void* __cdecl operator new( size_t, void* p ) throw() { return p; }

  //
  // operator new for CMemBased derived classes with virtual functions
  //
  static void* __cdecl operator new( size_t cb, IBaseMemoryManager * m_Mm ) throw() {
    return Zalloc2( cb );
  }

  //
  // operator delete for CMemBased derived classes
  //
  static void __cdecl operator delete(void* p)
  {
    ((T*)p)->m_Mm->Free(p);
  }

  //=======================================================
  // a-la "C" runtime memory related functions
  //=======================================================
  void* Malloc( size_t size ) const ATTRIBUTE_ALLOC_SIZE
  {
    return Malloc2( size );
  }

  void* Zalloc( size_t size ) const ATTRIBUTE_ALLOC_SIZE
  {
    return Zalloc2( size );
  }

  void Free( void* p ) const
  {
    Free2( p );
  }

  //=======================================================
  // End of "C" runtime memory related functions
  //=======================================================
#endif //STATIC_MEMORY_MANAGER
};

} // namespace api

#endif //__MEMORY_MGM_H__
