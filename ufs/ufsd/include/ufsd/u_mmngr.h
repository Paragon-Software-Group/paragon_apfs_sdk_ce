// <copyright file="u_mmngr.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef UFSD_NO_CRT_USE

#if defined __GNUC__
  #if defined UFSD_DRIVER_LINUX && defined __LINUX_ARM_ARCH__

    // Some arm architectures, like one on NVidia Carma, does not export __memzero in userspace
    #if defined UFSD_FUSE
      #include <string.h>
      extern "C" void bzero(void*, size_t);
      #undef Memzero2
      #define Memzero2 bzero
    #else
      extern "C" void   __memzero(void*, size_t);
      #undef Memzero2
      #define Memzero2  __memzero
    #endif
  #elif defined UFSD_DRIVER_LINUX && defined  __mips__
    // unfortunately mips kernel does not export bzero/__bzero
  #else
    #undef Memzero2
    #define Memzero2  __builtin_bzero
  #endif

#endif

#endif // #ifndef UFSD_NO_CRT_USE


namespace UFSD {

//  Class with overloaded "new/delete" operators.
//  Class is used to allocate and free memory
//  using our own memory manager.
#ifdef STATIC_MEMORY_MANAGER
template<class T>
struct UMemBased  : public api::StaticMemBased
{
  UMemBased( api::IBaseMemoryManager* ) {}
};

#else

template<class T>
struct UMemBased
{
  api::IBaseMemoryManager * m_Mm;

  //
  // Inplace operator new
  //
  static void* __cdecl operator new( size_t, void* p ) throw() { return p; }

  //
  // operator new for UMemBased derived classes with virtual functions
  //
  static void* __cdecl operator new( size_t cb, api::IBaseMemoryManager* Mm ) throw() {
    return Mm->Malloc( cb, BASE_MEMORY_FLAG_ZERO );
  }

  //
  // operator delete for UMemBased derived classes
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


private:
  // Make compiler happy (just declare but not implement):
  UMemBased();
  UMemBased(const UMemBased&);
  UMemBased& operator=(const UMemBased&);
  // By default new[] and delete[] are empty
  // redefine in your class if necessary
  static void* __cdecl operator new[] (size_t) throw() { return NULL; }
  static void  __cdecl operator delete[] (void*) {}

protected:
  UMemBased( api::IBaseMemoryManager * mm ) : m_Mm(mm)
  {
  }

  ~UMemBased()
  {
  } // non virtual!
};
#endif

} // namespace UFSD {
