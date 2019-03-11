// <copyright file="fsbase.cpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File UFSD/src/fs/fsbase.cpp
 *
 *  Here is implementation of non pure virtual functions
 *  for Base classes of file, directory and file system
 *  in Universal File System Driver (UFSD).
 *
 *  - Short and pseudo name generation functions
 *  - VerifySectors (a-la surface test)
 ****************************************************/

#include "../h/versions.h"
#include <ufsd.h>
#include "../h/verstr.h"
#include "../h/utrace.h"
#include "../h/assert.h"

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName [] = __FILE__ ",$Revision: 331543 $";
#endif

namespace UFSD{

#ifdef _MSC_VER
// Show compiled UFSD version
#pragma message ("         Compile " UFSD_VER_STRING )
#endif

// This string is used for manual checking UFSD version
// Just open UFSD*.lib in any viewer and look for "UFSD version X.X"
const char szUFSDVersion[] =
#ifndef UFSD_DRIVER_LINUX
UFSD_VER_STRING " (" __DATE__ ", " __TIME__ ")\n"
#endif
UFSD_FEATURES_STR
//
// Public part of hint
//
#ifdef UFSD_BUILD_ENVIRONMENT_HINT
UFSD_BUILD_ENVIRONMENT_HINT
#endif
#ifdef UFSD_EVAL_BUILD
  " Evaluation copy - not for resale! " UFSD_NEW_LINE
#endif
#ifndef NDEBUG
  "debug"  UFSD_NEW_LINE
#endif
;

//
// Hidden part of hint
//
#ifdef UFSD_BUILD_ENVIRONMENT
const char szUFSDVersionEnv[] __attribute__ ((used)) = UFSD_BUILD_ENVIRONMENT;
#endif

#ifndef UFSD_NO_PRINTK
///////////////////////////////////////////////////////////
// CheckUFSDEndian
//
//
///////////////////////////////////////////////////////////
const char*
__stdcall CheckUFSDEndian()
{
  volatile unsigned short Word = 0x1234;
  volatile unsigned char* p    = (unsigned char*)&Word;
#ifdef BASE_BIGENDIAN
  assert( 0x12 == p[0] && 0x34 == p[1] );
  if ( 0x34 == p[0] && 0x12 == p[1] )
    return _T("Error: big endian library, little endian platform");
#else
  assert( 0x34 == p[0] && 0x12 == p[1] );
  if ( 0x12 == p[0] && 0x34 == p[1] )
    return _T("Error: little endian library, big endian platform");
#endif
  return NULL;
}
#endif


///////////////////////////////////////////////////////////
// GetUFSDVersion
//
// We need to create at least one reference to this string
// Certainly smart linker can remove GetUFSDVersion if it is not called
// If GetUFSDVersion is removed linker can remove szUFSDVersion too.
// Lets pray and pray
///////////////////////////////////////////////////////////
const char*
__stdcall
GetUFSDVersion()
{
#ifdef UFSD_NO_PRINTK
  return szUFSDVersion;
#else
  const char* Ret = CheckUFSDEndian();
  return NULL == Ret? szUFSDVersion : Ret;
#endif
}


#ifndef UFSD_DRIVER_LINUX

////////////////////////////////////////////////
// Implementation of CFSObject methods
// name_len < 0 - means that name is zero terminating string
////////////////////////////////////////////////
int
CFSObject::Init(
    IN CDir *       parent,
    IN api::STRType name_type,
    IN const void*  name,
    IN FSObjectType obj_type,
    IN size_t       name_len,
    IN api::STRType AltName_type,   // StrUnknown,
    IN const void*  AltName,        // NULL
    IN size_t       AltNameLen      // 0
    )
{
  if( obj_type != FSObjFile && obj_type != FSObjDir )
    return ERR_BADOBJECTTYPE;
  //Fill internal data
  m_tObjectType = (unsigned char)obj_type;
  m_Parent      = parent;

  if ( 0 != name_len )
  {
    int err = SetName( name_type, name, name_len );
    if ( !UFSD_SUCCESS(err) )
      return err;
  }

  return 0 == AltNameLen? ERR_NOERROR : SetAltName( AltName_type, AltName, AltNameLen );
}


///////////////////////////////////////////////////////////
// CFSObject::GetFullName
//
// FullName       - Buffer for full name
// chSize         - Maximum number of symbols in buffer (including last zero)
//
// Returns the length of filled buffer
// If FullName is NULL then the returned value is the length
// (not including last zero!) required for full name
//
// NOTE: recursive function
///////////////////////////////////////////////////////////
size_t
CFSObject::GetFullName(
    IN api::IBaseStringManager*  Str,
    OUT char*                    FullName,
    IN size_t                    chSize
    ) const
{
  size_t Size, SrcLen, DstLen, ParentLen;
  unsigned char NameType;
  const unsigned short* aName;
  unsigned short Zero;

  if ( NULL == m_Parent )
  {
Root:
    // This is a root. Root always has name "/"
    if ( NULL != FullName && chSize >= 2 )
    {
      FullName[0] = SLASH_CHAR;
      FullName[1] = 0;
    }
    return 1;
  }

  // First try to use "long" name
  // If there are no long name, use short one
  if ( NULL != m_aName )
  {
    NameType = m_tNameType;
    aName    = m_aName;
    SrcLen   = m_NameLen;
  }
  else if( m_aAltName != NULL )
  {
    NameType = m_tAltNameType;
    aName    = m_aAltName;
    SrcLen   = m_AltNameLen;
  }
  else
  {
    NameType = api::StrUTF8;
    Zero     = 0;
    aName    = &Zero;
    SrcLen   = 0;
  }

  if ( api::StrANSI == NameType || api::StrUTF8 == NameType || api::StrOEM == NameType )
  {
    DstLen = SrcLen;
  }
  else if ( api::StrUTF16 == NameType || api::StrUCS16 == NameType )
  {
    DstLen = 4*SrcLen; // assume bad
    void* szTmp = Malloc2( DstLen + 1 );
    if ( NULL != szTmp )
    {
      verify( UFSD_SUCCESS( Str->Convert( static_cast<api::STRType>(NameType), aName, SrcLen, api::StrUTF8, szTmp, DstLen + 1, &DstLen ) ) );
      Free2( szTmp );
    }
  }
  else
  {
    assert( api::StrUnknown == NameType );
    // Detached object
    goto Root;
  }
  assert( 0 != DstLen );
  // Include the last '/' for directories
  if ( FSObjDir == m_tObjectType )
    DstLen += 1;

  // Left bytes
  Size  = chSize <= DstLen? 0 : chSize - DstLen;

  // Len is the length of filled buffer ended with '/'
  ParentLen = m_Parent->GetFullName( Str, FullName, Size );

  if ( NULL != FullName && Size > ParentLen )
  {
    // Add this object name
    char* ptr = FullName + ParentLen;
    if ( api::StrUTF8 == NameType )
      Memcpy2( ptr, aName, SrcLen + 1 );
    else if ( api::StrANSI == NameType || api::StrOEM == NameType )
      verify( UFSD_SUCCESS( Str->Convert( static_cast<api::STRType>(NameType), aName, SrcLen, api::StrUTF8, ptr, DstLen + 1, NULL ) ) );
    else
    {
      size_t DstLen2;
      verify( UFSD_SUCCESS( Str->Convert( static_cast<api::STRType>(NameType), aName, SrcLen, api::StrUTF8, ptr, DstLen + 1, &DstLen2 ) ) );
      assert( (FSObjDir == m_tObjectType && DstLen2 == DstLen - 1)
           || (FSObjDir != m_tObjectType && DstLen2 == DstLen) );
    }

    // Include the last '/'
    if ( FSObjDir == m_tObjectType )
      ptr[DstLen-1] = SLASH_CHAR;
    ptr[DstLen] = 0;
  }
  return ParentLen + DstLen;
}


///////////////////////////////////////////////////////////
// CFSObject::SetName
//
//
///////////////////////////////////////////////////////////
int
CFSObject::SetName(
    IN api::STRType name_type,
    IN const void * name,
    IN size_t       name_len
    )
{
  unsigned int CharSize = api::IBaseStringManager::GetCharSize(name_type);
  //Check if file name type is Ok
  if( 0 == CharSize )
    return ERR_BADSTRTYPE;
  //Check parameters
  if( NULL == name )
    return ERR_BADPARAMS;
  if( name_len > MAX_FILENAME*2 )
    return ERR_BADNAME_LEN;

  m_tNameType  = api::StrUnknown;
  Free2( m_aName );
  m_aName      = (unsigned short*)Malloc2( (name_len + 1)*CharSize );
#ifndef UFSD_MALLOC_CANT_FAIL
  if ( NULL == m_aName )
    return ERR_NOMEMORY;
#endif
  Memcpy2( m_aName, name, name_len*CharSize );
  m_tNameType = (unsigned char)name_type;
  C_ASSERT( (unsigned short)(-1) >= MAX_FILENAME );
  m_NameLen   = (unsigned short)name_len;
  // Set last zero
  if ( CharSize == 1 )
    reinterpret_cast<char*>(m_aName)[name_len] = 0;
  else
    reinterpret_cast<unsigned short*>(m_aName)[name_len] = 0;

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CFSObject::SetAltName
//
//
///////////////////////////////////////////////////////////
int
CFSObject::SetAltName(
    IN api::STRType name_type,
    IN const void * name,
    IN size_t       name_len
    )
{
  // Fill Alternative name
  Free2( m_aAltName );
  m_aAltName      = NULL;
  m_tAltNameType  = api::StrUnknown;

  unsigned CharSize = api::IBaseStringManager::GetCharSize(name_type);
  //Check if file name type is Ok
  if( 0 != CharSize && NULL != name )
  {
    if( name_len + 1 <= MAX_DOSFILENAME )
    {
      m_aAltName = (unsigned short*)Malloc2( (name_len + 1)*CharSize );
#ifndef UFSD_MALLOC_CANT_FAIL
      if ( NULL == m_aAltName )
        return ERR_NOMEMORY;
#endif
      m_tAltNameType = (unsigned char)name_type;
      Memcpy2( m_aAltName, name, name_len*CharSize );
      C_ASSERT( (unsigned short)(-1) >= MAX_DOSFILENAME );
      m_AltNameLen  = (unsigned short)name_len;
      // Set last zero
      if ( CharSize == 1 )
        reinterpret_cast<char*>(m_aAltName)[name_len] = 0;
      else
        reinterpret_cast<unsigned short*>(m_aAltName)[name_len] = 0;
    }
    else
    {
      assert(!"Alternative name is too big");
    }
  }

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CFSObject::GetName
//
// Returns pointer to 'name'
// and sets type value into NameType value
///////////////////////////////////////////////////////////
const void*
CFSObject::GetName(
    IN  api::STRType&  type,
    IN  bool      bAltName,
    OUT size_t*   NameLen
    ) const
{
  if ( bAltName ) {
    type = static_cast<api::STRType>(m_tAltNameType);
    if ( NULL != NameLen )
      *NameLen = m_AltNameLen;
    return m_aAltName;
  }
  // Default way
  type = static_cast<api::STRType>(m_tNameType);
  if ( NULL != NameLen )
    *NameLen = m_NameLen;
  return m_aName;
}


///////////////////////////////////////////////////////////
// CFSObject::CompareName
//
// Compare filename
///////////////////////////////////////////////////////////
int
CFSObject::CompareName(
    IN api::IBaseStringManager* str,
    IN api::STRType             type,
    IN const void*              name,
    IN size_t                   name_len
    )
{
  void *dest        = NULL;
  const void *wname = name;
  int err;

  // Check arguments
//  if ( (long)name_len < 0 )
//    name_len = NULL == name? 0 : str->strlen(type,name);
  if ( NULL == name || 0 == name_len )
    return ERR_BADPARAMS;

  if ( type != m_tNameType
      #ifdef UFSD_PSEUDONAMES
       || type != m_tAltNameType
      #endif
     )
  { // Normalize !!!MASK!!! to match our name type.
    size_t dst_len = (name_len + 1) * 2; // always by two keeping mbcs in mind.
    dest = Malloc2(dst_len);
#ifndef UFSD_MALLOC_CANT_FAIL
    if( NULL == dest )
      return  ERR_NOMEMORY;
#endif
    err = str->Convert(type, name, name_len, type != m_tNameType ? m_tNameType : m_tAltNameType, dest, dst_len, NULL);
    if ( !UFSD_SUCCESS( err ) )
      goto fin;
  }

  if ( type != m_tNameType )
    wname=dest;         // compare Name
  err = name_len != m_NameLen || 0 != Memcmp2(m_aName, wname, m_NameLen * str->GetCharSize(m_tNameType));

#ifdef UFSD_PSEUDONAMES
  if ( err && !IsEmpty( static_cast<api::STRType>(m_tAltNameType), m_aAltName ) )
  {                    // compare AltName
    wname = name;
    if ( type != m_tAltNameType )
      wname = dest;
    err = name_len != m_AltNameLen || 0 != Memcmp2(m_aAltName, wname, name_len);
  }
#endif

  if ( err )
    err = ERR_NOT_SUCCESS;

 fin:
  Free2(dest);
  return err;
}


///////////////////////////////////////////////////////////
// CFSObject::GetObjectInfo
//
// Fills names info from internal members
///////////////////////////////////////////////////////////
int
CFSObject::GetObjectInfo(
    OUT FileInfo& fi
    )
{
  // And add name information from this particular object.
  fi.NameType  = static_cast<api::STRType>(m_tNameType);
  fi.NameLen   = m_NameLen;
  if( m_NameLen )
    Memcpy2( fi.Name, m_aName, (fi.NameLen + 1) * api::IBaseStringManager::GetCharSize( fi.NameType) );
  else
    Memzero2( fi.Name, api::IBaseStringManager::GetCharSize( fi.NameType) );

  fi.AltNameType = static_cast<api::STRType>(m_tAltNameType);
  if ( NULL != m_aAltName ){
    fi.AltNameLen = m_AltNameLen;
    Memcpy2( fi.AltName, m_aAltName, (fi.AltNameLen + 1) * api::IBaseStringManager::GetCharSize( fi.AltNameType) );
  }
  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CFSObject::GetId128
//
// Returns unique USN (ntfs/refs) ID
///////////////////////////////////////////////////////////
void
CFSObject::GetId128(
    OUT FILE_ID_128* Id
    ) const
{
  Id->Lo = GetObjectID();
  Id->Hi = 0;
}


///////////////////////////////////////////////////////////
// CFSObject::GetParentID
//
// Returns parent unique ID.
// -1 means error
///////////////////////////////////////////////////////////
UINT64
CFSObject::GetParentID(
    IN unsigned int Pos
    ) const
{
  return 0 != Pos || NULL == m_Parent? MINUS_ONE_64 : m_Parent->GetObjectID();
}


///////////////////////////////////////////////////////////
// CFSObject::GetNameByPos
//
// Returns name
// NULL means error
///////////////////////////////////////////////////////////
const void*
CFSObject::GetNameByPos(
    IN  unsigned int   Pos,
    OUT api::STRType&  Type,
    OUT size_t&        NameLen
    ) const
{
  return 0 != Pos? NULL : GetName( Type, false, &NameLen );
}


///////////////////////////////////////////////////////////
// CFSObject::ListEa
//
// Copy a list of extended attribute names into the buffer
// provided, or compute the buffer size required
///////////////////////////////////////////////////////////
int
CFSObject::ListEa(
    OUT void*,
    IN  size_t,
    OUT size_t* Bytes
    )
{
  *Bytes = 0;
  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CFSObject::GetEa
//
// Reads extended attribute
///////////////////////////////////////////////////////////
int
CFSObject::GetEa(
    IN  const char*,
    IN  size_t,
    OUT void*,
    IN  size_t,
    OUT size_t*
    )
{
//  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CFSObject::SetEa
//
// Creates/Writes extended attribute
///////////////////////////////////////////////////////////
int
CFSObject::SetEa(
    IN  const char*,
    IN  size_t,
    IN  const void*,
    IN  size_t,
    IN  size_t
    )
{
  //assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CFSObject::ReadSymLink
//
// This function reads symbolic link by id and allocate pData as output buffer
///////////////////////////////////////////////////////////
int
CFSObject::ReadSymLink(
    OUT char* ,
    IN  size_t,
    OUT size_t*
    )
{
  //assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CFile::GetMap
//
// Translates Virtual Byte Offset into Logical Byte Offset
///////////////////////////////////////////////////////////
int
CFile::GetMap(
    IN  const UINT64&,
    IN  const UINT64&,
    IN  size_t,
    OUT MapInfo*
    )
{
  //assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CFile::fAllocate
//
// Allocates the disk space within the range [Offset,Offset+Bytes)
///////////////////////////////////////////////////////////
int
CFile::fAllocate(
    IN const UINT64&  Offset,
    IN const UINT64&  Bytes,
    IN size_t         Flags
    )
{
  UNREFERENCED_PARAMETER( Flags );
  MapInfo Map;

  UINT64 VboEnd = Offset + Bytes;

  for ( UINT64 Vbo = Offset; Vbo < VboEnd; Vbo += Map.Len )
  {
    int Status = GetMap( Vbo, VboEnd - Vbo, UFSD_MAP_VBO_CREATE, &Map );
    if ( !UFSD_SUCCESS( Status ) )
      return Status;
    if ( 0 == Map.Len )
    {
      assert( UFSD_VBO_LBO_COMPRESSED == Map.Lbo || UFSD_VBO_LBO_ENCRYPTED == Map.Lbo );
      return ERR_NOTIMPLEMENTED; // ??
    }
  }

  return ERR_NOERROR;
}


#if defined UFSD_NTFS_DELETE_SUBFILE | defined UFSD_HFS_DELETE_SUBFILE
///////////////////////////////////////////////////////////
// CFile::Delete
//
// Deletes the range of file
///////////////////////////////////////////////////////////
int
CFile::Delete(
    IN const UINT64&,
    IN const UINT64&
    )
{
  return ERR_NOTIMPLEMENTED;
}
#endif


///////////////////////////////////////////////////////////
// CDir::Close
//
// This function closes file or directory
///////////////////////////////////////////////////////////
int
CDir::Close(
    IN CFSObject* Fso
    )
{
  if ( NULL == Fso )
    return ERR_BADPARAMS;
  assert( 0 == Fso->GetReffCount() );
  return Fso->Flush( true );
}


///////////////////////////////////////////////////////////
// CDir::Open
//
//
///////////////////////////////////////////////////////////
int
CDir::Open(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    OUT CFSObject**,
    OUT FileInfo*
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::Create
//
//
///////////////////////////////////////////////////////////
int
CDir::Create(
    IN api::STRType,
    IN const void*,
    IN size_t,
    IN unsigned short,
    IN unsigned int,
    IN unsigned int,
    IN const void*,
    IN size_t,
    OUT CFSObject**
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::OpenDir
//
//
///////////////////////////////////////////////////////////
int
CDir::OpenDir(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    OUT CDir*&
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::CloseDir
//
//
///////////////////////////////////////////////////////////
int
CDir::CloseDir(
    IN CDir* dir
    )
{
  if ( NULL == dir )
    return ERR_BADPARAMS;

  assert( 0 == dir->GetReffCount() );

  return dir->Flush( true );
}


///////////////////////////////////////////////////////////
// CDir::CloseFile
//
//
///////////////////////////////////////////////////////////
int
CDir::CloseFile(
    IN CFile* file
    )
{
  if ( NULL == file )
    return ERR_BADPARAMS;

  assert( 0 == file->GetReffCount() );

  return file->Flush( true );
}


///////////////////////////////////////////////////////////
// CDir::CreateDir
//
//
///////////////////////////////////////////////////////////
int
CDir::CreateDir(
    IN api::STRType,
    IN const void*,
    IN size_t,
    OUT CDir**
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::CreateFile
//
//
///////////////////////////////////////////////////////////
int
CDir::CreateFile(
    IN api::STRType,
    IN const void*,
    IN size_t,
    OUT CFile**
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::OpenFile
//
//
///////////////////////////////////////////////////////////
int
CDir::OpenFile(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    OUT CFile*&
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::SetFileInfo
//
//
///////////////////////////////////////////////////////////
int
CDir::SetFileInfo(
    IN api::STRType,
    IN const void*,
    IN size_t,
    IN const FileInfo&,
    IN size_t
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::Link
//
//
///////////////////////////////////////////////////////////
int
CDir::Link(
    IN CFSObject*,
    IN api::STRType,
    IN const void*,
    IN size_t
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::GetFileInfo
//
//
///////////////////////////////////////////////////////////
int
CDir::GetFileInfo(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    OUT FileInfo&
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::GetFSObjID
//
//
///////////////////////////////////////////////////////////
int
CDir::GetFSObjID(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    OUT UINT64&,
    IN  int
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::CreateSymLink
//
//
///////////////////////////////////////////////////////////
int
CDir::CreateSymLink(
    IN api::STRType,
    IN const void*,
    IN size_t,
    IN const char*,
    IN size_t,
    OUT UINT64*
    )
{
  //assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::GetSymLink
//
//
///////////////////////////////////////////////////////////
int
CDir::GetSymLink(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    OUT char*,
    IN  size_t,
    OUT size_t*
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::CheckSymLink
//
//
///////////////////////////////////////////////////////////
int
CDir::CheckSymLink(
    IN  api::STRType,
    IN  const void*,
    IN  size_t
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CDir::LookupOpenObject
//
// Default implementation for CDir::LookupOpenObject
// that enumerates all opened objects and compares it with given name
///////////////////////////////////////////////////////////
CFSObject*
CDir::LookupOpenObject(
    IN api::IBaseStringManager*  str,
    IN api::STRType              type,
    IN const void*               name,
    IN size_t                    name_len,
    IN int                       IsDir // < 0 if any (file or dir)
    )
{
  list_head* pos;
  if ( IsDir <= 0 )
  {
    // Enum files in current directory
    list_for_each( pos, &m_OpenFiles )
    {
      CFile* f = list_entry( pos, CFile, m_Entry );
      if ( 0 == f->CompareName( str, type, name, name_len ) )
        return f;
    }
  }

  if ( 0 != IsDir )
  {
    // Enum dirs in current directory
    list_for_each( pos, &m_OpenDirs )
    {
      CDir* d = list_entry( pos, CDir, m_Entry );
      if ( 0 == d->CompareName( str, type, name, name_len ) )
        return d;
    }
  }

  return NULL;
}


///////////////////////////////////////////////////////////
// CDir::GetSubDirAndFilesCount
//
// Default implementation of function that counts subdirectories and or files
///////////////////////////////////////////////////////////
int
CDir::GetSubDirAndFilesCount(
    OUT bool*   IsEmpty,        // may be NULL
    OUT size_t* SubDirsCount,   // may be NULL
    OUT size_t* FilesCount      // may be NULL
    )
{
  int Status;
  FileInfo* fi = (FileInfo*)Malloc2( sizeof(FileInfo) );
#ifndef UFSD_MALLOC_CANT_FAIL
  if ( NULL == fi )
    return ERR_NOMEMORY;
#endif

  CEntryNumerator* Enum = NULL;

  Status = StartFind( Enum );
  if ( !UFSD_SUCCESS( Status ) )
  {
    Free2( fi );
    return Status;
  }

  size_t Dirs = 0, Files = 0;

  if ( NULL != IsEmpty )
    *IsEmpty = true;

  while( UFSD_SUCCESS( Status = FindNext( Enum, *fi ) ) )
  {
    if ( api::StrUTF8 == fi->NameType || api::StrOEM == fi->NameType || api::StrANSI == fi->NameType )
    {
      const char* ptr = (const char*)fi->Name;
      if ( '\0' == ptr['.' != ptr[0]? 0 : '.' != ptr[1]? 1 : 2] )
        continue;
    }
    else if ( api::StrUTF16 == fi->NameType || api::StrUCS16 == fi->NameType )
    {
      if ( '\0' == fi->Name['.' != fi->Name[0]? 0 : '.' != fi->Name[1]? 1 : 2] )
        continue;
    }
    else
    {
      Status = ERR_BADPARAMS;
      break;
    }

    if ( NULL != IsEmpty )
    {
      *IsEmpty = false;
      if ( NULL == SubDirsCount && NULL == FilesCount )
        break;
    }

    if ( fi->Attrib & UFSD_SUBDIR )
      Dirs += 1;
    else
      Files += 1;
  }

  Enum->Destroy();
  Free2( fi );

  if ( !UFSD_SUCCESS(Status) && ERR_NOFILEEXISTS != Status )
    return Status;

  if ( NULL != SubDirsCount )
    *SubDirsCount = Dirs;
  if ( NULL != FilesCount )
    *FilesCount = Files;

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CDir::GetSize
//
//
///////////////////////////////////////////////////////////
UINT64
CDir::GetSize()
{
  return 0;
}
#endif // #ifndef UFSD_DRIVER_LINUX


#if defined UFSD_FAT || defined UFSD_EXTFS2 || defined UFSD_APFS || (!defined UFSD_NO_USE_IOCTL && (defined UFSD_NTFS || defined UFSD_HFS))
///////////////////////////////////////////////////////////
// CDir::EnumOpenedDirs
//
// This function enumerates opened directories
///////////////////////////////////////////////////////////
size_t
CDir::EnumOpenedDirs(
    IN bool (*fn)(
        IN CDir*  dir,
        IN void*  Param
        ),
    IN void*  Param,
    OUT bool* bBreak
    ) const
{
  size_t nDirs = 0;
  // Enum opened dirs
  list_head* pos, *n;
  list_for_each_safe( pos, n, &m_OpenDirs )
  {
    nDirs  += 1;
    *bBreak = (*fn)( list_entry( pos, CDir, m_Entry ), Param );
    if ( *bBreak )
      return nDirs;
  }

  // Enum opened subdirs
  list_for_each_safe( pos, n, &m_OpenDirs )
  {
    nDirs += list_entry( pos, CDir, m_Entry )->EnumOpenedDirs( fn, Param, bBreak );
    if ( *bBreak )
      return nDirs;
  }
  return nDirs;
}


///////////////////////////////////////////////////////////
// CDir::EnumOpenedFiles
//
// This function enumerates opened files
///////////////////////////////////////////////////////////
size_t
CDir::EnumOpenedFiles(
    IN bool (*fn)(
        IN CFile* file,
        IN void*  Param
        ),
    IN void*  Param,
    OUT bool* bBreak
    ) const
{
  size_t nFiles = 0;
  // Enum files in current directory
  list_head* pos, *n;
  list_for_each_safe( pos, n, &m_OpenFiles )
  {
    nFiles += 1;
    *bBreak = (*fn)( list_entry( pos, CFile, m_Entry ), Param );
    if ( *bBreak )
      return nFiles;
  }

  // Enum files in other directories
  list_for_each_safe( pos, n, &m_OpenDirs )
  {
    nFiles += list_entry( pos, CDir, m_Entry )->EnumOpenedFiles( fn, Param, bBreak );
    if ( *bBreak )
      return nFiles;
  }

  return nFiles;
}
#endif // #if defined UFSD_FAT || (!defined UFSD_NO_USE_IOCTL && (defined UFSD_NTFS || defined UFSD_HFS))


///////////////////////////////////////////////////////////
// CFSObject::ListStreams
//
// Copy a list of available stream names into the buffer
// provided, or compute the buffer size required
///////////////////////////////////////////////////////////
int
CFSObject::ListStreams(
    OUT void*     ,//Buffer,
    IN  size_t    ,//BytesPerBuffer,
    OUT size_t*   Bytes,
    OUT api::STRType*  NameType
    )
{
  *Bytes    = 0;
  *NameType = api::StrUnknown;
  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CFSObject::ReadStream
//
// Reads named stream
///////////////////////////////////////////////////////////
int
CFSObject::ReadStream(
    IN  api::STRType  ,//NameType,
    IN  const void*   ,//Name,
    IN  size_t        ,//NameLen,
    IN  const UINT64& ,//Offset,
    OUT void*         ,//Buffer,
    IN  size_t        ,//BytesPerBuffer,
    OUT size_t*        //Read
    )
{
  assert(!"Not implemented");
  return ERR_NOFILEEXISTS;
}


///////////////////////////////////////////////////////////
// CFSObject::WriteStream
//
// Creates/Writes named stream
///////////////////////////////////////////////////////////
int
CFSObject::WriteStream(
    IN  api::STRType,
    IN  const void*,
    IN  size_t,
    IN  const UINT64&,
    IN  const void*,
    IN  size_t,
    OUT size_t*
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CFSObject::RemoveStream
//
// Delete named stream
///////////////////////////////////////////////////////////
int
CFSObject::RemoveStream(
    IN  api::STRType,
    IN  const void*,
    IN  size_t
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


#if defined UFSD_TRACE | defined UFSD_NTFS_CHECK | defined UFSD_EXFAT_CHECK \
  | defined UFSD_HFS_CHECK | defined UFSD_NTFS_GET_VOLUME_META_BITMAP
#define UFSD_FS_USE_TRACE_BUFFER  "Do not forget to free trace memory"
#endif

///////////////////////////////////////////////////////////
// CDir::Dtor
//
//
///////////////////////////////////////////////////////////
void
CDir::Dtor()
{
  while( !m_OpenFiles.is_empty() )
  {
    CFile* f = list_entry( m_OpenFiles.next, CFile, m_Entry );
    UFSDTrace(( "Nonclosed file %p r=%" PLL "x\n", f, f->GetObjectID() ));
    f->m_Parent = NULL;
    f->m_Entry.remove();
    f->m_Entry.init();
  }

  while( !m_OpenDirs.is_empty() )
  {
    CDir* d = list_entry( m_OpenDirs.next, CDir, m_Entry );
    UFSDTrace(( "Nonclosed dir %p r=%" PLL "x\n", d, d->GetObjectID() ));
    d->m_Parent = NULL;
    d->m_Entry.remove();
    d->m_Entry.init();
  }
}


///////////////////////////////////////////////////////////
// CDir::Relink
//
// Helper function for rename
///////////////////////////////////////////////////////////
void
CDir::Relink(
    IN CFSObject* Fso
    )
{
  CDir* OldParent = Fso->m_Parent;

  //
  // Check if FS does this for us.
  // If it does then there is no need to do it manually.
  //
  if ( OldParent == this )
    return;

  //
  // Unlink it from the old parent.
  //
  Fso->m_Entry.remove();

  //
  // Link it to the new parent.
  //
  Fso->m_Parent = this;
  switch ( Fso->m_tObjectType ) {

  case CFSObject::FSObjDir:
    Fso->m_Entry.insert_after( &m_OpenDirs );
    break;

  case CFSObject::FSObjFile:
    Fso->m_Entry.insert_after( &m_OpenFiles );
    break;

  case CFSObject::FSObjUnknown:
  default:
    assert( !"Impossible" );
    break;
  }
}


///////////////////////////////////////////////////////////
// CFileSystem::~CFileSystem
//
//
///////////////////////////////////////////////////////////
CFileSystem::~CFileSystem()
{
  CloseAll();
#ifdef UFSD_FS_USE_TRACE_BUFFER
  Free2( m_TraceBuffer );
#endif
}

#if !defined UFSD_DRIVER_LINUX && defined UFSD_TRACE
///////////////////////////////////////////////////////////
// CFileSystem::GetFullNameA
//
//
///////////////////////////////////////////////////////////
const char*
CFileSystem::GetFullNameA(
    IN const CFSObject* obj
    )
{
  if ( NULL == m_TraceBuffer )
    m_TraceBuffer = (char*)Malloc2( 512 );

  if ( NULL == m_TraceBuffer
    || 0 == obj->GetFullName( m_Strings, m_TraceBuffer, 512 ) )
  {
    return "";
  }

  m_TraceBuffer[511] = 0;
  return m_TraceBuffer;
}
#endif


//#ifdef UFSD_FS_USE_TRACE_BUFFER

///////////////////////////////////////////////////////////
// CFileSystem::U2A
//
//
///////////////////////////////////////////////////////////
const char*
CFileSystem::U2A(
    IN const unsigned short* szUnicode,
    IN size_t Len,
    OUT char* szBuffer
    )
{
  size_t MaxOut, Len2 = Len;
  if ( NULL != szBuffer )
  {
    MaxOut = Len + 1;
  }
  else
  {
    //
    // NOTE: one unicode symbol can be translated in 1-4 utf8 chars
    // 256 unicode symbols require up to 1024 utf8 chars
    //
    MaxOut  = 1024;

    if ( NULL == m_TraceBuffer )
    {
      m_TraceBuffer = (char*)Malloc2( 1024 );
      if ( NULL == m_TraceBuffer )
        return NULL;
    }

    szBuffer  = m_TraceBuffer;
  }

  if ( NULL == m_Strings )
  {
    if ( Len >= MaxOut )
      Len = MaxOut - 1;
    for ( size_t i = 0; i < Len; i++ )
      szBuffer[i] = (char)szUnicode[i];
  }
  else if ( 0 != Len
    && 0 != m_Strings->Convert( api::StrUTF16, szUnicode, Len, api::StrUTF8, szBuffer, MaxOut, &Len2 ) )
  {
    assert( !"Failed to convert unicode string" );
    Len = 0;
  }
  else if ( Len2 < Len )
  {
    Len = Len2;
  }

  szBuffer[Len] = 0;
  return szBuffer;
}

//#endif

#if !defined UFSD_DRIVER_LINUX && !defined UFSD_DRIVER_OSX
///////////////////////////////////////////////////////////
// CFileSystem::ReInit (CFileSystem virtual function)
//
// This function is used to reinit FileSystem by new options
///////////////////////////////////////////////////////////
int
CFileSystem::ReInit(
    IN size_t     Options,
    OUT size_t*   ,
    IN const PreInitParams*
    )
{
  m_Options = Options;
  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// CFileSystem::OpenByID  (CFileSystem virtual function)
//
// Opens file or dir by its id
///////////////////////////////////////////////////////////
int
CFileSystem::OpenByID(
    IN  UINT64,
    OUT CFSObject**,
    IN  int
    )
{
  //assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}


///////////////////////////////////////////////////////////
// CFileSystem::GetParent (CFileSystem virtual function)
//
// Returns any parent for given file/dir
///////////////////////////////////////////////////////////
int
CFileSystem::GetParent(
    IN  CFSObject*,
    OUT CFSObject**
    )
{
  assert(!"Not implemented");
  return ERR_NOTIMPLEMENTED;
}

#endif // #ifndef UFSD_DRIVER_LINUX


#ifndef UFSD_DRIVER_LINUX
///////////////////////////////////////////////////////////
// FlushDirTree
//
//
///////////////////////////////////////////////////////////
static inline int
FlushDirTree(
    IN CDir* Dir
    )
{
  int FirstError = ERR_NOERROR;
  list_head* pos;

  list_for_each( pos, &Dir->m_OpenFiles )
  {
    CFile* File = list_entry( pos, CFile, m_Entry );
    int Status = File->Flush();
    if ( !UFSD_SUCCESS(Status) && ERR_NOERROR == FirstError )
      FirstError = Status;
  }

  list_for_each( pos, &Dir->m_OpenDirs )
  {
    CDir* SubDir = list_entry( pos, CDir, m_Entry );

    int Status = FlushDirTree( SubDir );
    if ( !UFSD_SUCCESS(Status) && ERR_NOERROR == FirstError )
      FirstError = Status;

    Status = SubDir->Flush();
    if ( !UFSD_SUCCESS(Status) && ERR_NOERROR == FirstError )
      FirstError = Status;
  }

  return FirstError;
}


///////////////////////////////////////////////////////////
// CFileSystem::Flush
//
//
///////////////////////////////////////////////////////////
int
CFileSystem::Flush(
    IN bool
    )
{
  if ( NULL == m_RootDir )
    return ERR_NOERROR;

  int Status   = FlushDirTree( m_RootDir );
  int Status2  = m_RootDir->Flush();

  return UFSD_SUCCESS( Status )? Status2 : Status;
}
#endif // #ifndef UFSD_DRIVER_LINUX


#if defined UFSD_EXFAT || defined UFSD_REFS
///////////////////////////////////////////////////////////
// CreateIntxLink
//
// Helper function for exfat/ntfs
///////////////////////////////////////////////////////////
int
CFileSystem::CreateIntxLink(
    IN  const char* Link,
    IN  size_t      LinkLen,
    OUT void**      IntxLnk,
    OUT size_t*     IntxLen
    )
{
  size_t uLen;
  size_t Bytes  = sizeof(UINT64) + sizeof(short) * 4 * LinkLen;
  void* intx    = Malloc2( Bytes );
  if ( NULL == intx )
    return ERR_NOMEMORY;

  *(UINT64*)intx = INTX_SYMBOLIC_LINK;

  if ( m_Strings->Convert( api::StrUTF8, Link, LinkLen - 1, api::StrUTF16, Add2Ptr( intx, sizeof(UINT64) ), sizeof(short) * 4 * LinkLen, &uLen ) )
  {
    Free2( intx );
    return ERR_BADNAME;
  }

#ifdef BASE_BIGENDIAN
  //
  // Convert into little endian unicode
  //
  unsigned short* uName = (unsigned short*)Add2Ptr( intx, sizeof(UINT64) );
  for ( unsigned int i = 0; i < uLen; i++, uName++ )
  {
    unsigned short x = *uName;
    *uName = (unsigned short)( ((x & 0xff) << 8) | ((x & 0xff000) >> 8) );
  }
#endif

  Bytes = sizeof(UINT64) + sizeof(short) * (unsigned)uLen;

  if ( uLen + 1 <= 4 * LinkLen )
  {
    ((short*)Add2Ptr( intx, sizeof(UINT64) ))[uLen] = 0;
    Bytes += sizeof(short);
  }

  *IntxLnk  = intx;
  *IntxLen  = Bytes;
  return ERR_NOERROR;
}
#endif


///////////////////////////////////////////////////////////
// CloseAll_hlp
//
// Returns true if there are lost files
///////////////////////////////////////////////////////////
static void
CloseAll_hlp(
    IN CDir* dir,
    IN api::IBaseLog* Log
    )
{
  list_head* pos, *n;

  list_for_each_safe( pos, n, &dir->m_OpenDirs )
  {
    CDir* d = list_entry( pos, CDir, m_Entry );
    ULOG_WARNING(( Log, "Close lost dir %p, r=%" PLL "x, rf=%d", d, d->GetObjectID(), d->m_Reff_count ));
    CloseAll_hlp( d, Log );
    d->Flush( true );
  }

  list_for_each_safe( pos, n, &dir->m_OpenFiles )
  {
    CFile* f = list_entry( pos, CFile, m_Entry );
    ULOG_WARNING(( Log, "Close lost file %p, r=%" PLL "x, rf=%d", f, f->GetObjectID(), f->m_Reff_count ));
    f->Flush( true );
  }
}


///////////////////////////////////////////////////////////
// CFileSystem::CloseAll
//
//
///////////////////////////////////////////////////////////
void
CFileSystem::CloseAll()
{
  if ( NULL != m_RootDir )
  {
    ULOG_TRACE(( GetVcbLog( this ), "CloseAll: Fs %p, root %p", (void*)this, m_RootDir ));

    bool bLost = !m_RootDir->m_OpenDirs.is_empty() || !m_RootDir->m_OpenFiles.is_empty();
    m_RootDir->m_Reff_count += 1;
    if ( bLost )
      CloseAll_hlp( m_RootDir, GetVcbLog( this ) );
    m_RootDir->m_Reff_count -= 1;
    m_RootDir->Flush( true );
    m_RootDir = NULL;
    if ( bLost )
      Flush( true );
  }
}


//=========================================================
//                UFSD filenames related API
//              contains following functions:
// - GetShortName
// - GetPseudoShortName
// - GetFirstUniqueName
// - GetNextUniqueName
//=========================================================

#if defined UFSD_SHORTNAMES | defined UFSD_PSEUDONAMES | defined UFSD_FAT

//---------------------------------------------------------
// UpcaseString<T>
//
// This helper full trusted function uppercases full string
//---------------------------------------------------------
template< class T>
static inline void
UpcaseString(
    IN api::IBaseStringManager*  pStrings,
    IN T*                        Str,
    IN size_t                    Len
    )
{
  for ( ; Len-- > 0; Str++ )
    *Str = (T)pStrings->toupper( *Str );
}


///////////////////////////////////////////////////////////
// IsSpecialSymbol
//
//
///////////////////////////////////////////////////////////
static inline bool
IsSpecialSymbol(
    IN unsigned c
    )
{
  switch( c )
  case '+': case ',' : case '.': case ';' : case '=': case '[': case ']':
  case ':': case '\"': case '?': case '\\': case '/': case '<': case '>': case '*':
    return true;
  return false;
}

//---------------------------------------------------------
// GetShortNameT<T>
//
// This helper full trusted function generates short name.
// It assumes that ShortName array is at least 13 symbols
//---------------------------------------------------------
template <class T>
static inline int
GetShortNameT(
    IN const T* LongName,
    IN size_t   LongNameLen,
    OUT T*      ShortName,
    OUT size_t* ShortNameLen
    )
{
  ShortName[0] = 0;
  // Skip first '.'
  while( LongNameLen > 0  && '.' == *LongName )
    LongName++, LongNameLen--;

  if ( 0 == LongNameLen )
  {
    if ( NULL != ShortNameLen )
      *ShortNameLen = 0;
    return ERR_NOERROR;
  }

  // Skip last '.'
  while( LongNameLen > 0  && '.' == LongName[LongNameLen-1] )
    LongNameLen--;

  if ( 0 == LongNameLen )
    return ERR_BADNAME;

  // Removes all '.' except last one
  size_t i,j, ext = ((size_t)-2);

  for ( j = i = 0; i < LongNameLen && j < 12; i++ )
  {
    if ( '.' == LongName[i] )
    {
      // Skip all but last '.'
      // Try to find next '.'
      size_t k = i + 1;
      do
      {
        if ( k >= LongNameLen )
        {
          ShortName[ext = j++] = '.';
          break;
        }
      } while( '.' != LongName[k++] );
    }
    else if ( ' ' != LongName[i]                           // Skip ' '
//      && ( 2 != sizeof(T) || 0 == (LongName[i] & 0xFF00) ) // Skip real UNICODE symbols
      && ( ((size_t)-2) != ext || j < 8 )                    // Main name is not greater than 8 symbols
      && ( ((size_t)-2) == ext || (j-ext) < 4 ) )            // Extensions is not greater than 3 symbols
    {
      // Replace special symbols
      if ( IsSpecialSymbol( LongName[i] ) )
        ShortName[j++] = '_';
      else
        ShortName[j++] = LongName[i];
    }
  }

  // Set last zero (remove last '.' if it exists)
  i = j == ext+1? ext : j;
  ShortName[i] = 0;
  if ( NULL != ShortNameLen )
    *ShortNameLen = i;
  return ERR_NOERROR;
}

//---------------------------------------------------------
// IsShortTheSame<T>
//
// This helper full trusted function checks two names
// for identity
//---------------------------------------------------------
template <class T>
static inline bool
IsShortTheSame(
    IN const T*   LongName,
    IN size_t     LongNameLen,
    IN const T*   ShortName
    )
{
  size_t i, GoodSymLong, GoodSymShort;

  // Calculate the number of good symbols in ShortName
  for ( i = GoodSymShort = 0; 0 != ShortName[i]; i++ )
    if ( '.' != ShortName[i] && ' ' != ShortName[i] )
      GoodSymShort += 1;

  // Check for empty short name
  if ( 0 == i )
    return false;

  // Calculate the number of good symbols in LongName
  for ( i = GoodSymLong = 0; i < LongNameLen; i++ )
    if ( '.' != LongName[i] && ' ' != LongName[i] )
      GoodSymLong += 1;

  // Number of good symbols must be the same and shortname must have at least one symbol
  return GoodSymShort == GoodSymLong;
}


//---------------------------------------------------------
// GetShortName (API function)
//
// This function translates long name to short one
// Algorithm:
// - Remove all not allowed in DOS symbols( ' ', UNICODE symbols )
// - Replace '+' by '_'
// - Remove the first '.' and the last '.'
// - Remove all '.' except last
// - Main name up to 8 symbols
// - Extension up to 3 symbols
//---------------------------------------------------------
int
GetShortName(
    IN api::IBaseStringManager*  pStrings,
    IN api::STRType              Type,
    IN const void*               LongName,
    IN size_t                    LongNameLen,
    OUT void*                    ShortName,
    IN size_t                    MaxShortNameLen,
    IN bool                      bDoUpCase,
    OUT size_t*                  ShortNameLen
    )
{
  // Check arguments
  if ( NULL == pStrings )
    return ERR_BADPARAMS;

  if ( (size_t)-1 == LongNameLen )
    LongNameLen = pStrings->strlen( Type, LongName );

  if ( 0 == LongNameLen || MaxShortNameLen < 13 )
    return ERR_BADPARAMS;

  unsigned ret;

  unsigned int CharSize = api::IBaseStringManager::GetCharSize(Type);
  if ( CharSize == 2 )
  {
    ret = GetShortNameT( (const unsigned short*)LongName,
                         LongNameLen,
                         (unsigned short*)ShortName,
                         ShortNameLen );

    if ( UFSD_SUCCESS( ret ) && bDoUpCase )
    {
      UpcaseString( pStrings, (unsigned short*)ShortName, *ShortNameLen );
    }
  }
  else if ( CharSize == 1 )
  {
    ret = GetShortNameT( (const unsigned char*)LongName,
                         LongNameLen,
                         (unsigned char*)ShortName,
                         ShortNameLen );
    if ( UFSD_SUCCESS( ret ) && bDoUpCase )
    {
      UpcaseString( pStrings, (unsigned char*)ShortName, *ShortNameLen );
    }
  }
  else
    return ERR_BADSTRTYPE;

  return ret;
}

#endif //#if defined UFSD_SHORTNAMES | defined UFSD_PSEUDONAMES | (defined UFSD_FAT & !defined UFSD_DRIVER_LINUX)


#if defined UFSD_PSEUDONAMES

static const unsigned char s_BaseSymbols[] = {
  '0','1','2','3','4','5','6','7','8','9',
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
  '$','_','!','{','}','#',
};

//---------------------------------------------------------
// GetPseudoName<T>
//
// This helper full trusted function generates pseudo name
// It assumes that ShortName array is at least 13 symbols
//---------------------------------------------------------
template <class T>
static inline unsigned
GetPseudoName(
    IN OUT T*   ShortName,
    IN  size_t  UniqueNum,
    OUT size_t* ShortNameLen,
    IN api::IBaseMemoryManager * m_Mm
    )
{
  size_t  NumLen = 0;
  // We need to generate short name
  // Calculate necessary number of symbols
  for ( size_t Tmp = UniqueNum; 0 != Tmp; NumLen++ )
    Tmp /= ARRSIZE(s_BaseSymbols);

  // Check special case 0 == UniqueNum
  if ( 0 == NumLen )
    NumLen += 1;

  // Reserve space for special sign ~
  NumLen += 1;

  assert( NumLen <= 7 );

  // NumLen + 1 symbols must be reserved before '.'
  if ( NumLen > 7 )
    return ERR_BADPARAMS;

  // Add ~123
  T* ptr = ShortName + 8 - NumLen;
  // Try to find '.'
  for( size_t i = 0; i < 8; i++ )
  {
    if ( '.' == ShortName[i] )
    {
      // Main name length is less than 8
      // Try to save as many symbols as possible
      if ( i + NumLen <= 8 )
        ptr = ShortName + i;
      // Move extension up (".123\0" = 5 symbols )
      Memmove2( ptr + NumLen, ShortName + i, 5 );
      break;
    }
    else if ( 0 == ShortName[i] )
    {
      // Name without extension
      if ( i + NumLen <= 8 )
        ptr = ShortName + i;
      ptr[NumLen] = 0;
      break;
    }
  }

  *ptr++ = '~';
  while( --NumLen > 0 )
  {
    *ptr++      = s_BaseSymbols[UniqueNum%ARRSIZE(s_BaseSymbols)];
    UniqueNum  /= ARRSIZE(s_BaseSymbols);
  }

  if ( NULL != ShortNameLen )
  {
    while( 0 != *ptr )
      ptr += 1;

    *ShortNameLen = ptr - ShortName;
  }

  return ERR_NOERROR;
}


//---------------------------------------------------------
// GetPseudoShortName ( API function )
//
// This function creates PseudoShortName of file
// using some unique number (MFTRecord number in NTFS)
// It copies the result string into ShortName array
// The size of this array (in symbols) is passed in ShortNameLen
//
// Parameters:
//
// LongName     - original long name
// api::STRType - it's type
// LongNameLen  - the length of LongName ( if < 0 then zero terminating string is assumed )
// ShortName    - the ouput buffer for generated short name
// ShortNameLen - it's maximum capacity in symbols (should be >= 13)
// UniqueNum    - Unique number
// bDoUpCase    - Do or not uppercase (NTFS does it in its owner manner)
//---------------------------------------------------------
int
GetPseudoShortName(
    IN  api::IBaseStringManager * pStrings,
    IN  api::STRType  Type,
    IN  const void*   LongName,
    IN  size_t        LongNameLen,
    OUT void*         ShortName,
    IN  size_t        MaxShortNameLen,
    IN  size_t        UniqueNum,
    IN  bool          bDoUpCase,
    OUT size_t*       ShortNameLen,
    IN api::IBaseMemoryManager * m_Mm
    )
{
  assert( MaxShortNameLen >= 13 );
  if ( (size_t)-1 == LongNameLen )
    LongNameLen = pStrings->strlen( Type, LongName );

  unsigned int CharSize = api::IBaseStringManager::GetCharSize(Type);
  if (LongNameLen<3) {                        // check for "."/".."
    unsigned Ch0, Ch1;
    if (CharSize == 1) {
      Ch0=((char *)LongName)[0];
      Ch1=((char *)LongName)[1];
      ((char *)ShortName)[1]=0;
      ((char *)ShortName)[2]=0;
    } else if (CharSize == 2) {
      Ch0=((short *)LongName)[0];
      Ch1=((short *)LongName)[1];
      ((short *)ShortName)[1]=0;
      ((short *)ShortName)[2]=0;
    }
    else
      return ERR_BADSTRTYPE;

    if (Ch0=='.' && (LongNameLen==1 || Ch1=='.' && LongNameLen==2)) {
      pStrings->strncpy(Type,ShortName,LongName,LongNameLen);
      if (LongNameLen < MaxShortNameLen)
      {
        if(CharSize == 2)
          ((short *)ShortName)[LongNameLen] = 0;
        else if(CharSize == 1)
          ((char *)ShortName)[LongNameLen] = 0;
        else
          return ERR_BADSTRTYPE;
      }
      if ( NULL != ShortNameLen )
        *ShortNameLen = LongNameLen;
      return ERR_NOERROR;
    }
  }

  unsigned ret = GetShortName( pStrings, Type,
                               LongName, LongNameLen,
                               ShortName, MaxShortNameLen,
                               bDoUpCase, ShortNameLen );

  if ( !UFSD_SUCCESS( ret ) )
    return ret;

  if ( CharSize == 2 )
  {
    if ( !IsShortTheSame( (const unsigned short*)LongName,
                          LongNameLen,
                          (const unsigned short*)ShortName ) )
    {
      return GetPseudoName( (unsigned short*)ShortName, UniqueNum, ShortNameLen, m_Mm );
    }
  }
  else if (CharSize == 1)
  {
    if ( !IsShortTheSame( (const unsigned char*)LongName,
                          LongNameLen,
                          (const unsigned char*)ShortName ) )
    {
      return GetPseudoName( (unsigned char*)ShortName, UniqueNum, ShortNameLen, m_Mm );
    }
  }
  else
    return ERR_BADSTRTYPE;

  return ERR_NOERROR;
}

#endif // #if defined UFSD_PSEUDONAMES


//
// FAT always requires this functions
// NTFS requires only if UFSD_SHORTNAMES is defined
//
#if defined UFSD_SHORTNAMES | defined UFSD_FAT

//---------------------------------------------------------
// GetFirstUniqueNameT<T>
//
// This helper full trusted function generates first unique name
//---------------------------------------------------------
template <class T>
static inline int
GetFirstUniqueNameT(
    IN OUT T* ShortName,
    OUT size_t* ShortNameLen,
    IN api::IBaseMemoryManager * m_Mm
    )
{
  UNREFERENCED_PARAMETER( m_Mm );
  // Assume simple case
  unsigned long k = 6;
  // Try to find '.' or 0
  for( unsigned long i = 1; i < 8; i++ )
  {
    if ( '.' == ShortName[i] )
    {
      // Move rest of name to the end
      k = i > 6? 6 : i;
      Memmove2( ShortName + k + 2, ShortName + i, 5 );
      break;
    }
    else if ( 0 == ShortName[i] )
    {
      k               = i;
      ShortName[i+2]  = 0;
      break;
    }
  }

  ShortName[k]   = '~';
  ShortName[k+1] = '1';
  if ( NULL != ShortNameLen )
  {
    k += 2;
    while( 0 != ShortName[k] )
      k += 1;
    *ShortNameLen = k;
  }

  return ERR_NOERROR;
}

//---------------------------------------------------------
// GetNextUniqueNameT<T>
//
// This helper full trusted function generates next unique name
//---------------------------------------------------------
template <class T>
static inline int
GetNextUniqueNameT(
    IN OUT T* ShortName,
    OUT size_t* ShortNameLen,
    IN api::IBaseMemoryManager * m_Mm
    )
{
  UNREFERENCED_PARAMETER( m_Mm );
  unsigned long pos, i, digits, Num, Max;
#if 0
  pos = 0;
  // Try to find '.' or 0
  while( pos < 8 && '.' != ShortName[pos] && 0 != ShortName[pos] )
    pos++;
#else
  pos = 6;
#endif

  // Find the nearest '~'
  while( pos > 0 && '~' != ShortName[pos] )
    pos--;
  assert( pos <= 6 );

  if ( 0 == pos || pos > 6 )
    return ERR_BADNAME;

  for ( i = pos+1, digits = Num = 0, Max = 1;
        0   != ShortName[i] &&  '.' != ShortName[i];
        i++, digits++, Max *= 10 )
  {
    if ( '0' > ShortName[i] || '9' < ShortName[i] )
      return ERR_BADNAME;
    Num = Num*10 + (ShortName[i]-'0');
  }

  // Increase Num
  Num += 1;
  if ( Num >= 1000000 )
    return ERR_BADNAME;

  if ( Num >= Max )
  {
    if ( i < 8
      && ( '.' == ShortName[i]
         || 0  == ShortName[i] ) )
    {
      Memmove2( ShortName + i + 1, ShortName + i, 5 );
    }
    else
    {
      ShortName[--pos] = '~';
    }
    digits++;
  }

  for( i = pos + digits; i > pos; i-- )
  {
    ShortName[i] = T('0' + (Num%10));
    Num /= 10;
  }

  if ( NULL != ShortNameLen )
  {
    i = pos + digits;
    while( 0 != ShortName[i] )
      i += 1;
    *ShortNameLen = i;
  }

  return ERR_NOERROR;
}

//---------------------------------------------------------
// GetUniqueNameNumberT<T>
//
// This helper full trusted function returned the number of unique name
//---------------------------------------------------------
template <class T>
static inline int
GetUniqueNameNumberT(
    IN OUT T* ShortName,
    OUT size_t* Number
    )
{
  unsigned long pos, i, digits, Num, Max;
  pos = 6;

  // Find the nearest '~'
  while( pos > 0 && '~' != ShortName[pos] )
    pos--;

  if ( 0 == pos)
    return ERR_BADNAME;

  for ( i = pos+1, digits = Num = 0, Max = 1;
        0 != ShortName[i] && '.' != ShortName[i] && ' ' != ShortName[i] && i < 8;
        i++, digits++, Max *= 10 )
  {
    if ( '0' > ShortName[i] || '9' < ShortName[i] )
      return ERR_BADNAME;
    Num = Num*10 + (ShortName[i]-'0');
  }

  if(Number)
    *Number = Num;

  return ERR_NOERROR;
}

//---------------------------------------------------------
// GetFirstUniqueName (API function)
//
// This function creates first "~n" name
//---------------------------------------------------------
int
GetFirstUniqueName(
    IN api::STRType    Type,
    IN OUT void*  ShortName,
    OUT size_t*   ShortNameLen,
    IN api::IBaseMemoryManager * Mm
    )
{
  unsigned int CharSize = api::IBaseStringManager::GetCharSize(Type);
  if ( CharSize == 2 )
    return GetFirstUniqueNameT( (unsigned short*)ShortName, ShortNameLen, Mm );
  else if (CharSize == 1)
    return GetFirstUniqueNameT( (unsigned char*)ShortName, ShortNameLen, Mm );
  else
    return ERR_BADSTRTYPE;
}

//---------------------------------------------------------
// GetNextUniqueName (API function)
//
// This function creates next "~n" name
//---------------------------------------------------------
int
GetNextUniqueName(
    IN api::STRType Type,
    IN OUT void*    ShortName,
    OUT size_t*     ShortNameLen,
    IN api::IBaseMemoryManager * Mm
    )
{
  unsigned int CharSize = api::IBaseStringManager::GetCharSize(Type);
  if ( CharSize == 2 )
    return GetNextUniqueNameT( (unsigned short*)ShortName, ShortNameLen, Mm );
  else if (CharSize == 1)
    return GetNextUniqueNameT( (unsigned char*)ShortName, ShortNameLen, Mm );
  else
    return ERR_BADSTRTYPE;
}

//---------------------------------------------------------
// GetUniqueNameNumber (API function)
//
// This function returns number from  "~n" name
//---------------------------------------------------------
int
GetUniqueNameNumber(
    IN api::STRType Type,
    IN void*        ShortName,
    OUT size_t*     Number
    )
{
  unsigned int CharSize = api::IBaseStringManager::GetCharSize(Type);
  if ( CharSize == 2 )
    return GetUniqueNameNumberT( (unsigned short*)ShortName, Number );
  else if (CharSize == 1)
    return GetUniqueNameNumberT( (unsigned char*)ShortName, Number );
  else
    return ERR_BADSTRTYPE;
}

#endif // #if defined UFSD_SHORTNAMES | defined UFSD_FAT

//=========================================================
//        END of UFSD filenames related API
//=========================================================


//---------------------------------------------------------
// GetNameA
//
// Most CDir functions gets arguments "type" and "name"
// This function allows to print this name
// NOTE: this function is implemented via U2A()
//---------------------------------------------------------
extern "C"
const char*
GetNameA(
    IN CFileSystem* Vcb,
    IN api::STRType type,
    IN const void * name,
    IN size_t       name_len
    )
{
  if ( api::IBaseStringManager::GetCharSize(type) == 2 )
    return Vcb->U2A( (unsigned short*)name, name_len );

  if ( (long)name_len < 0
    || ( (NULL != name) && (0 == ((const char*)name)[name_len]) ) )
  {
    return (const char*)name;
  }

  assert(!"To Test");
  return "";
}

#if defined UFSD_TRACE && defined UFSD_USE_STATIC_TRACE

//---------------------------------------------------------
// GetDigit
//
// Helper for UFSD_DumpMemory function
//---------------------------------------------------------
static inline char GetDigit( size_t Digit )
{
  Digit &= 0xF;
  if ( Digit <= 9 )
    return (char)('0' + Digit);

  return (char)('A' + Digit - 10 );
}


//---------------------------------------------------------
// UFSD_DumpMemory (API function)
//
// This function Dumps memory into trace
//---------------------------------------------------------
extern "C" void
UFSD_DumpMemory(
    IN const void*  Mem,
    IN size_t       nBytes
    )
{
  if ( NULL == Mem || 0 == nBytes )
    return;
  const unsigned char*  Arr  = (const unsigned char*)Mem;
  const unsigned char*  Last = Arr + nBytes;
  size_t  nRows              = (nBytes + 0xF)>>4;
  for ( size_t i = 0; i < nRows && Arr < Last; i++ )
  {
    char szLine[75];
    char* ptr = szLine;

    // Create address 0000, 0010, 0020, 0030, and so on
    if ( nBytes > 0x10000 )
      *ptr++ = GetDigit(i >> 16);
    if ( nBytes > 0x8000 )
      *ptr++ = GetDigit(i >> 12);
    *ptr++ = GetDigit(i >> 8);
    *ptr++ = GetDigit(i >> 4);
    *ptr++ = GetDigit(i >> 0);
    *ptr++ = '0';
    *ptr++ = ':';
    *ptr++ = ' ';

    const unsigned char* Arr0 = Arr;
    int j;
    for ( j = 0; j < 0x10 && Arr < Last; j++, Arr++ )
    {
      *ptr++ = GetDigit( *Arr >> 4 );
      *ptr++ = GetDigit( *Arr >> 0 );
      *ptr++ = ' ';
    }
    while( j++ < 0x10 )
    {
      *ptr++ = ' ';
      *ptr++ = ' ';
      *ptr++ = ' ';
    }

    *ptr++ = ' ';
    *ptr++ = ' ';
    for ( Arr = Arr0, j = 0; j < 0x10 && Arr < Last; j++ )
    {
      unsigned char ch = *Arr++;
      if ( ch >= 128 )
        ch -= 128;

      if ( ch < 32 || 0x7f == ch || '%' == ch )
        ch ='.';//^= 0x40;

      *ptr++ = ch;
    }
    // Set last zero
    *ptr++ = 0;
//    assert( ptr - szLine <= sizeof(szLine) );

    UFSDTrace(( "%s\n", szLine ));
  }
//  UFSDTrace(("\n"));
}

#else

///////////////////////////////////////////////////////////
// UFSD_DumpMemory
//
//
///////////////////////////////////////////////////////////
extern "C" void
UFSD_DumpMemory(
    IN const void*,
    IN size_t
    )
{

}
#endif

//---------------------------------------------------------
// FormatSizeGb
//
// Returns Gb,Mb to print with "%u.%02u Gb"
//---------------------------------------------------------
unsigned int
FormatSizeGb(
    IN  const UINT64& Bytes,
    OUT unsigned int* Mb
    )
{
  // Do simple right 30 bit shift of 64 bit value
  UINT64 kBytes = Bytes >> 10;
  unsigned int kBytes32 = (unsigned int)kBytes;

  *Mb = (100*(kBytes32 & 0xfffff) + 0x7ffff)>>20;
  assert( *Mb <= 100 );
  if ( *Mb >= 100 )
    *Mb = 99;

  return (kBytes32 >> 20) | (((unsigned int)(kBytes>>32)) << 12);
}

#if defined  UFSD_NTFS_FORMAT | defined UFSD_NTFS_EXTEND \
  | defined UFSD_FAT_FORMAT | defined UFSD_FAT_EXTEND \
  | defined UFSD_HFS_FORMAT | defined UFSD_EXFAT_FORMAT \
  | defined UFSD_EXT_FORMAT | defined UFSD_EXT2_FORMAT \
  | defined UFSD_REFS_FORMAT | defined UFSD_REFS3_FORMAT \
  | defined UFSD_APFS_FORMAT

///////////////////////////////////////////////////////////
// CheckRange (recursive function)
//
// Returns false if out of memory
// TODO: To reduce memory usage in deep recursion
// we can pack all arguments in common struct
// or use global variables
///////////////////////////////////////////////////////////
static inline bool
CheckRange(
    IN api::IDeviceRWBlock*     Rw,
    IN const UINT64&            Lbo,
    IN size_t                   Bytes,
    IN api::IBaseMemoryManager* m_Mm,
    IN api::IMessage*           Msg,
    IN OUT unsigned long*       nBlocks,
    OUT t_BadBlock**            BadBlocks
    )
{
//  assert( 0 != Bytes );
  if ( Bytes < 512 )
    return true;

  int Status = Rw->ReadBytes( Lbo, NULL, Bytes, RWB_FLAGS_VERIFY );

  if ( UFSD_SUCCESS( Status ) || ERR_NOTIMPLEMENTED == Status )
  {
    // This block of sectors is OK. Get next block
    return true;
  }

  //ULOG_DEBUG1(( GetVcbLog( this ), "%" PLL "x, %lx, %lx", Sector, Count, *nBlocks ));

  if ( 512 == Bytes )
  {
    t_BadBlock* p = (*BadBlocks) + *nBlocks - 1;
    if ( 0 != *nBlocks && p->Lbo + p->Bytes == Lbo )
    {
      // Just increase size of block
      p->Bytes += 512;
    }
    else
    {
      p = (t_BadBlock*)Malloc2( sizeof(t_BadBlock) * (*nBlocks+2) );
#ifndef UFSD_MALLOC_CANT_FAIL
      if ( NULL == p )
        return false;
#endif

      Memcpy2( p, *BadBlocks, sizeof(t_BadBlock) * *nBlocks );
      Free2( *BadBlocks );
      *BadBlocks = p;
      p += *nBlocks;

      p->Lbo      = Lbo;
      p->Bytes    = 512;
      // Set last zero
      p[1].Bytes  = 0;
      *nBlocks += 1;

      if ( NULL != Msg )
        Msg->PutMessage( api::MsgLog, _T("Bad block found, started at offset 0x%" PLL "x"), Lbo );
    }
    return true;
  }

  size_t Half = Bytes >> 1;

  //
  // Check left range
  //
  if ( !CheckRange( Rw, Lbo, Half, m_Mm, Msg, nBlocks, BadBlocks ) )
    return false;

  //
  // Check right range
  //
  if ( !CheckRange( Rw, Lbo + Half, Bytes - Half, m_Mm, Msg, nBlocks, BadBlocks ) )
    return false;

  return true;
}


///////////////////////////////////////////////////////////
//  VerifyBytes
//
//  Lbo       - Start byte to verify
//  Bytes     - Total bytes to verify (relative Start)
//  BadBlocks - The array of bad sectors
//  Msg       - Outlet for messages and progress bar
//
// Verifies sectors for bad blocks
//
//  NOTE: If the function returns ERR_NOERROR
//        then caller should free the returned array pBadBlocks
///////////////////////////////////////////////////////////
extern "C" int
VerifyBytes(
    IN const UINT64&            Lbo,
    IN const UINT64&            Bytes,
    IN api::IDeviceRWBlock*     Rw,
    IN api::IBaseMemoryManager* m_Mm,
    IN api::IMessage*           Msg,
    IN const char*              Hint,
    OUT t_BadBlock**            BadBlocks
    )
{
  if( !Rw || !m_Mm || !BadBlocks )
    return ERR_BADPARAMS;

  unsigned long nBlocks = 0;  // Number of elements
  int Status  = ERR_NOERROR;

  if ( NULL != Msg )
    Msg->NewProgress( 0, Bytes, _T("Verifying Area %s..."), NULL == Hint? "" : Hint );

  // Set last plus 1 sector to verify
  for ( UINT64 Done = 0; Done < Bytes; )
  {
    if ( NULL != Msg )
    {
      Msg->SetPos( Done );
      // Check abort flag
      if ( Msg->IsBreak() )
      {
        Status = ERR_CANCELLED;
        break;
      }
    }

    UINT64 ToCheck = Bytes - Done;
    if ( ToCheck > 512*512 )
      ToCheck = 512*512;

    if ( !CheckRange( Rw, Lbo + Done, (size_t)ToCheck, m_Mm, Msg, &nBlocks, BadBlocks ) )
    {
#ifndef UFSD_MALLOC_CANT_FAIL
      Status = ERR_NOMEMORY;
      break;
#endif
    }
    Done += ToCheck;
  }

  if ( !UFSD_SUCCESS( Status ) )
  {
    Free2( *BadBlocks );
    *BadBlocks = NULL;
  }

  return Status;
}


///////////////////////////////////////////////////////////
// DiscardRange
//
// Discards (not necessary zeroing) range of bytes (at least 512 bytes aligned)
///////////////////////////////////////////////////////////
extern "C" int
DiscardRange(
    IN UINT64             Offset,
    IN UINT64             Bytes,
    IN api::IDeviceRWBlock* Rw,
    IN api::IMessage*     Msg,
    IN const char*        Hint
    )
{
  //
  // Check if discard really works on the device
  //
  const unsigned ProbeRange = 4*1024*1024;
  int Status = Rw->DiscardRange( Offset, Bytes < ProbeRange? Bytes : ProbeRange );
  if ( !UFSD_SUCCESS( Status ) )
    return Status;

  if ( Bytes <= ProbeRange )
    return ERR_NOERROR;

  Bytes  -= ProbeRange;
  Offset += ProbeRange;

  if ( NULL != Msg )
    Msg->NewProgress( 0, Bytes, _T("Discarding area %s..."), NULL == Hint? "" : Hint );

  UINT64 Off0 = Offset;
  for ( ;; )
  {
    if ( NULL != Msg )
    {
      Msg->SetPos( Offset - Off0 );
      // Check abort flag
      if ( Msg->IsBreak() )
        return ERR_CANCELLED;
    }

    // Trim 1Gb at once
    UINT64 Len = Bytes < 1024*1024*1024? Bytes : 1024*1024*1024;
    Status = Rw->DiscardRange( Offset, Len );
    if ( !UFSD_SUCCESS( Status ) )
      return Status;
    Bytes -= Len;
    if ( 0 == Bytes )
      break;
    Offset += Len;
  }

  return ERR_NOERROR;
}


///////////////////////////////////////////////////////////
// ComputeVolId
//
// This function calculates unique volume id
///////////////////////////////////////////////////////////
unsigned int
ComputeVolId(
    IN unsigned int Seed,
    IN api::ITime*  Time
    )
{
  unsigned int volid = Seed;

  do
  {
    if ( 0 == volid )
    {
      UINT64 t = Time->Time();
      volid = (unsigned int)t;
      if ( 0 == volid )
        volid = (unsigned int)(t>>32);
      if ( 0 == volid )
        // This should never happen.
        volid = 0x11111111;
    }

    unsigned char* p = (unsigned char*) &volid;
    for ( size_t i = 0; i < sizeof(volid); i++)
    {
      volid += *p++;
      volid = (volid >> 2) + (volid << 30);
    }
  } while ( 0 == volid );

  return volid;
}

#endif // defined  UFSD_NTFS_FORMAT | defined UFSD_NTFS_EXTEND | defined UFSD_FAT_FORMAT | defined UFSD_FAT_EXTEND | defined UFSD_HFS_FORMAT | defined UFSD_EXFAT_FORMAT


#if defined  UFSD_NTFS_FORMAT | defined UFSD_FAT_FORMAT | defined UFSD_HFS_FORMAT | defined UFSD_EXT_FORMAT | defined UFSD_EXT2_FORMAT | defined UFSD_EXFAT_FORMAT \
  | defined  UFSD_NTFS_CHECK | defined  UFSD_FAT_CHECK | defined  UFSD_HFS_CHECK | defined  UFSD_EXT_CHECK | defined UFSD_EXFAT_CHECK | defined UFSD_REFS3_CHECK \
  | defined  UFSD_NTFS_WIPE | defined UFSD_REFS_FORMAT | defined UFSD_REFS3_FORMAT | defined UFSD_LSWAP \
  | defined UFSD_NTFS_GET_VOLUME_META_BITMAP | defined UFSD_HFS_GET_VOLUME_META_BITMAP | defined UFSD_EXFAT_GET_VOLUME_META_BITMAP | defined UFSD_EXTFS_GET_VOLUME_META_BITMAP

///////////////////////////////////////////////////////////
// U64toAh
//
// A-la sprintf( Buf, "%llx", N )
///////////////////////////////////////////////////////////
char*
U64toAh(
    IN  UINT64  N,
    OUT char*   Buf
    )
{
  static char Tmp[32];
  if ( NULL == Buf )
    Buf = Tmp;
  char* buf = Buf;

  for ( int m = 0; m < 16; m += 1 )
  {
    unsigned int d = (unsigned int)(N >> 60);
    N <<= 4;
    assert( d < 16 );
    if ( 0 == d && buf == Buf )
      continue; // skip leading zero

    *Buf++ = (char)(d < 10? ('0' + d) : ('a' + d - 10));
  }
  if ( buf == Buf )
    *Buf++ = '0';
  *Buf = 0;

  return buf;
}

#endif


#if defined UFSD_HFS | defined UFSD_EXFAT_CHECK

///////////////////////////////////////////////////////////
// ItoT
//
// A-la _sntprintf( Buf, MaxChars, TEXT("%u"), N )
///////////////////////////////////////////////////////////
template<class T>
int ItoT(
    IN  unsigned int  N,
    OUT T*            Buf,
    IN  int           MaxChars
    )
{
  int Len = 0;
  assert( MaxChars >= 10 );   // 0xffffffff == 4294967295 - 10 symbols
  C_ASSERT( (int)1000000000 > 0 );
  for ( int z = 1000000000; 0 != z; z /= 10 )
  {
    int d = N/z;
    N    %= z;
    assert( d < 10 );
    if ( 0 == d && 0 == Len )
      continue; // skip leading zero

    if ( MaxChars-- <= 0 )
    {
      assert( !"TtoW" );
      return Len;
    }
    *Buf++ = (T)('0' + d);
    Len += 1;
  }
  return Len;
}


///////////////////////////////////////////////////////////
// ItoW
//
// A-la _snwprintf( Buf, MaxChars, L"%u", N )
///////////////////////////////////////////////////////////
int
ItoW(
    IN  unsigned int    N,
    OUT unsigned short* Buf,
    IN  int             MaxChars
    )
{
  return ItoT( N, Buf, MaxChars );
}

#endif // #if defined UFSD_HFS | defined UFSD_EXFAT_CHECK

#if defined UFSD_HFS

///////////////////////////////////////////////////////////
// ItoA
//
// A-la _snprintf( Buf, MaxChars, "%u", N )
///////////////////////////////////////////////////////////
int
ItoA(
    IN  unsigned int    N,
    OUT unsigned char*  Buf,
    IN  int             MaxChars
    )
{
  return ItoT( N, Buf, MaxChars );
}


///////////////////////////////////////////////////////////
// TtoI
//
// A-la stscanf( Buf, TEXT("%u"), &N )
///////////////////////////////////////////////////////////
template <class T>
unsigned int
TtoI(
    IN const T* Buf,
    IN int      Len,
    OUT bool*   Error
    )
{
  unsigned int Ret = 0;
  *Error = false;
  assert( Len <= 10 );   // 0xffffffff == 4294967295 - 10 symbols
  for ( int i = 0; i < Len; i++ )
  {
    unsigned short c = *Buf++;
    if ( '0' <= c && c <= '9' )
    {
      Ret *= 10;
      Ret += c - '0';
    }
    else
    {
      assert( !"TtoI" );
      *Error = true;
    }
  }
  return Ret;
}


///////////////////////////////////////////////////////////
// AtoI
//
// A-la sscanf( Buf, "%u", &N )
///////////////////////////////////////////////////////////
unsigned int
AtoI(
    IN  const unsigned char*  Buf,
    IN  int                   Len,
    OUT bool*                 Error
    )
{
  return TtoI( Buf, Len, Error );
}


#ifdef UFSD_HFS_CHECK
///////////////////////////////////////////////////////////
// WtoI
//
// A-la swscanf( Buf, L"%u", &N )
///////////////////////////////////////////////////////////
unsigned int
WtoI(
    IN  const unsigned short* Buf,
    IN  int                   Len,
    OUT bool*                 Error
    )
{
  return TtoI( Buf, Len, Error );
}
#endif // #ifdef UFSD_HFS_CHECK

#endif // #ifdef UFSD_HFS


#if defined UFSD_REFS_FORMAT || defined UFSD_REFS3 || defined UFSD_APFS_FORMAT
///////////////////////////////////////////////////////////
// UFSD_UuidCreate
//
// Simple method of creating GUID's
///////////////////////////////////////////////////////////
extern "C" void
UFSD_UuidCreate(
    IN api::ITime*  Tt,
    OUT GUID*       Guid
    )
{
  UINT64 v[4];
  C_ASSERT( 4 == sizeof(GUID)/sizeof(unsigned) );

  v[0]  = Tt->Time();
  v[1]  = ~v[0];
  v[2]  = ((UINT64)(size_t)Tt) << 30;
  v[2] |= (UINT64)(size_t)Guid;
  v[3]  = ~v[2];

  unsigned* guid = (unsigned*)Guid;
  for ( unsigned int i = 0; i < 4; i++ )
  {
    //  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1
    #define GOLDEN_RATIO_PRIME PU64(0x9e37fffffffc0001)
    // High bits are more random, so use them.
    *guid++ = (unsigned)( (v[i]*GOLDEN_RATIO_PRIME) >> (32 - 9));
  }
}

static const unsigned char s_GuidMap[] = {
  3, 2, 1, 0, '-', 5, 4, '-', 7, 6, '-',
  8, 9, '-', 10, 11, 12, 13, 14, 15
};

static const char s_DigitsUp[]  = "0123456789ABCDEF";
static const char s_DigitsLow[] = "0123456789abcdef";

///////////////////////////////////////////////////////////
// UFSD_Guid2String
//
// Converts GUID into XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX form
// Returns strlen(szGuid) or 0 if buffer is too small.
///////////////////////////////////////////////////////////
extern "C" int
UFSD_Guid2String(
    IN  const GUID* Guid,
    OUT char*       szGuid,
    IN  int         MaxChars,
    IN  bool        bUpCase
    )
{
  const unsigned char* pBytes = (const unsigned char*)Guid;
  const char* Digits = bUpCase? s_DigitsUp : s_DigitsLow;
#ifndef NDEBUG
  char* p = szGuid;
#endif

  if ( MaxChars <= STRING_GUID_LEN )
    return 0;

  for ( unsigned int i = 0; i < sizeof(s_GuidMap); i++ )
  {
    unsigned char c = s_GuidMap[i];
    if ( '-' == c )
      *szGuid++ = '-';
    else
    {
      c         = pBytes[c];
      *szGuid++ = Digits[ (c & 0xF0) >> 4 ];
      *szGuid++ = Digits[ c & 0x0F ];
    }
  }
  *szGuid  = '\0';

  assert( STRING_GUID_LEN == szGuid - p );

  return STRING_GUID_LEN;
}
#endif

} // namespace UFSD {
