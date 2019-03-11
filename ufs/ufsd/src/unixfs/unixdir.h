// <copyright file="unixdir.h" company="Paragon Software Group">
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
//     08-August-2017  - created.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __UFSD_UNIXDIR_H
#define __UFSD_UNIXDIR_H

#include "unixfs.h"
#include "unixenum.h"
#include "unixfile.h"

namespace UFSD
{

//=================================================================
class CUnixDir : public CDir
{
protected:
  CUnixFileSystem*       m_pFS;
  CUnixInode*            m_pInode;
  CUnixEnumerator*       m_pEnum;
#ifdef UFSD_DRIVER_LINUX
  unsigned short*        m_aName;
  unsigned short         m_NameLen;
  char                   m_tNameType;
#endif

#ifdef UFSD_CacheAlloc
  // To free root directory (as FileSystem object is already destroyed)
  void*                   m_DirCache;
#endif

public:
  CUnixDir(CUnixFileSystem* vcb);
  virtual ~CUnixDir();

  virtual int Init(
    IN UINT64        Inode,
    IN CDir*         pParent,
    IN CUnixInode*   pInode,
    IN api::STRType  Type,
    IN const void*   Name,
    IN unsigned int  NameLength
  );

  api::IBaseLog* GetLog() const { return GetVcbLog( m_pFS ); }
  CUnixInode* GetInode() const { return m_pInode; }
  CUnixEnumerator* GetEnum() const { return m_pEnum; }

  int FollowLink(
    IN FileInfo* Info
    );

  int OpenPath(
    IN  char*       path,
    IN  FileInfo*   Info,
    OUT CUnixDir**  StartDir,
    OUT CUnixDir**  FoundDir
    );

  static bool IsDot(IN const FileInfo& fi);
  static bool IsDotDot(IN const FileInfo& fi);

#ifndef UFSD_DRIVER_LINUX
  void CloseAll();
#endif

  CUnixDir* CloseParent();

#ifdef UFSD_DRIVER_LINUX
  template<typename T>
  int SetObjectName(
    IN T*           Fso,
    IN CDir*        pParent,
    IN api::STRType Type,
    IN const void*  Name,
    IN FSObjectType obj_type,
    IN unsigned int NameLen
    )
  {
    Fso->m_tObjectType = obj_type;
    Fso->m_Parent      = pParent;
    Free2( Fso->m_aName );
    Fso->m_aName      = (unsigned short*)Malloc2( (NameLen + 1)*sizeof(unsigned short) );
  #ifndef UFSD_MALLOC_CANT_FAIL
    if ( NULL == Fso->m_aName )
      return ERR_NOMEMORY;
  #endif
    Memcpy2( Fso->m_aName, Name, NameLen );
    Fso->m_tNameType = (unsigned char)Type;
    C_ASSERT( (unsigned short)(-1) >= MAX_FILENAME );
    Fso->m_NameLen   = (unsigned short)NameLen;
    reinterpret_cast<unsigned short*>(Fso->m_aName)[NameLen] = 0;

    return ERR_NOERROR;
  }
#endif

  //=================================================================
  //            Extended Attributes
  //=================================================================

  // Copy a list of extended attribute names into the buffer
  // provided, or compute the buffer size required
  virtual int ListEa(
    OUT void*     Buffer,
    IN  size_t    BytesPerBuffer,
    OUT size_t*   Bytes
    )
  {
    return m_pInode->ListEa( Buffer, BytesPerBuffer, Bytes );
  }

  // Reads extended attribute
  virtual int GetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    OUT void*       Value,
    IN  size_t      BytesPerValue,
    OUT size_t*     Len
    )
  {
    return m_pInode->GetEa( Name, NameLen, Value, BytesPerValue, Len );
  }

  // Creates/Writes extended attribute
  virtual int SetEa(
    IN  const char* Name,
    IN  size_t      NameLen,
    IN  const void* Value,
    IN  size_t      BytesPerValue,
    IN  size_t      Flags
  );

  //=================================================================
  //          CFSObject  virtual functions
  //=================================================================

  virtual void Destroy()
  {
    CUnixDir* p = this;
    while (p)
    {
      p = p->CloseParent();
    }
  }

  virtual int GetObjectInfo(
    OUT t_FileInfo&  file_info
    );

  virtual int SetObjectInfo(
    IN const t_FileInfo&  file_info,
    IN size_t             Flags
    );

  virtual int Flush(
    IN bool bDelete = false
    );

  // Returns this object ID
  virtual UINT64 GetObjectID() const { return m_pInode->Id(); }

  // Returns parent unique ID
  virtual UINT64 GetParentID(
    IN unsigned int Pos
    ) const
  {
    return (!Pos && m_Parent) ? m_Parent->GetObjectID() : MINUS_ONE_64;
  }

#ifdef UFSD_DRIVER_LINUX
  virtual const void* GetNameByPos(
    IN  unsigned int  Pos,
    OUT api::STRType& Type,
    OUT size_t&       NameLen
    ) const
  {
    if (Pos == 0)
    {
      Type = api::StrUTF16;
      NameLen = m_NameLen;
      return m_aName;
    }
    return NULL;
  }
#endif

  //=================================================================
  //          END OF CFSObject  virtual functions
  //=================================================================

  //=================================================================
  //          CDir  virtual functions
  //=================================================================

  //
  // This function opens file or directory
  //
  virtual int Open(
    IN  api::STRType    type,
    IN  const void*     name,
    IN  size_t          name_len,
    OUT CFSObject**     Fso,
    OUT FileInfo*       fi
  );

  /////////////////////////////////////////////////////////////////////////////
  //Creates sub directory with the specified name
  //  type - defines name type (UNICODE or ASCII)
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int CreateDir(
    IN api::STRType     type,
    IN const void *     name,
    IN size_t           name_len,
    OUT CDir**          dir = NULL
  );

  //Creates file in this dir
  //  type - defines name type (UNICODE or ASCII)
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int CreateFile(
    IN api::STRType     type,
    IN const void *     name,
    IN size_t           name_len,
    OUT CFile**         file = NULL
  );

  //Deletes file or empty directory with name 'name' in dir;
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int Unlink(
    IN api::STRType     type,
    IN const void *     name,
    IN size_t           name_len,
    IN CFSObject*       Fso = NULL
  );

  // Creates a hard link to the file/directory
  // NOTE: hard link to directory usually is not allowed
  virtual int Link(
    IN CFSObject*       Fso,
    IN api::STRType     type,
    IN const void *     name,
    IN size_t           name_len
  );

  //Finds file or directory corresponding to name_mask specified
  //  type - defines ASCII or UNICODE
  //  name_maks - name mask informational, may be ignored
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int StartFind(
    OUT CEntryNumerator*& dentry_num,
    IN  size_t            options = 0 // See UFSD_ENUM_XXX
  );

  //Finds file or directory corresponding to name_mask specified
  //  type - defines ASCII or UNICODE
  //  name_maks - name mask informational, may be ignored
  //  fi - file information found
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  //Remark:
  //  Using methods that modify directory content (Create*, Unlink, etc.)
  //  between FindFirst() / FindNext() will lead to unpredictable
  //  search results

  virtual int FindNext(
    IN  CEntryNumerator*    dentry_num,
    OUT FileInfo&           fi
    )
  {
    return FindNext(dentry_num, fi, NULL);
  }

  //Opens directory with specified name and places it to the list of
  //open directories.
  //  type - defines type of name (UNICODE or ASCII)
  //  dir  - pointer to the opened directory
  //Returns:
  //  0 - on success
  //  error code otherwise
  virtual int OpenDir(
    IN api::STRType     type,
    IN  const void*     name,
    IN  size_t          name_len,
    OUT CDir *&         dir
  )
  {
    return OpenFSObject(type, name, name_len, CFSObject::FSObjDir, &dir);
  }


  /////////////////////////////////////////////////////////////////////////////
  //Opens file in this directory
  //  type - defines type of 'name' (UNICODE or ASCII)
  //  file - pointer to the opened file
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int OpenFile(
    IN api::STRType     type,
    IN  const void*     name,
    IN  size_t          name_len,
    OUT CFile *&        file
  )
  {
    return OpenFSObject(type, name, name_len, CFSObject::FSObjFile, &file);
  }

#ifdef UFSD_DRIVER_LINUX
  //Closes opened file 'file'
  //Return:
  //  0 - if successful
  //  error code otherwise
  virtual int CloseFile(
      IN CFile * file
      );
#endif

  //Retrieve file information
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int GetFileInfo(
    IN  api::STRType    type,
    IN  const void*     name,
    IN  size_t          name_len,
    OUT FileInfo&       fi
    );

  //Set file information
  //  type - file name type (ASCII or UNCODE)
  //  name - file name
  //  fi   - file information
  //Returns:
  //  0 - if successfully
  //  error code otherwise
  virtual int SetFileInfo(
    IN api::STRType     type,
    IN const void *     name,
    IN size_t           name_len,
    IN const FileInfo&  fi,
    IN size_t           flags
    );

  // This function gets unique ID of FS object by its' path
  virtual int GetFSObjID(
    IN  api::STRType    type,
    IN  const void*     name,
    IN  size_t          name_len,
    OUT UINT64&         ID,
    IN  int             fFollowSymlink = 1
    );

  virtual int CreateSymLink(
    IN api::STRType     type,
    IN const void*      name,
    IN size_t           name_len,
    IN const char*      symlink,
    IN size_t           sym_len,
    OUT UINT64*         pId
  );

  //This function reads symbolic link name
  virtual int GetSymLink(
    IN  api::STRType    type,
    IN  const void*     name,
    IN  size_t          name_len,
    OUT char*           buffer,
    IN  size_t          buflen,
    OUT size_t*         read
    );

  //This function checks is link name referenced to existing file object
  virtual int CheckSymLink(
    IN  api::STRType    type,
    IN  const void*     name,
    IN  size_t          name_len
    );

  virtual int ReadSymLink(
    OUT char*       /*Buffer*/,
    IN  size_t      /*BufLen*/,
    OUT size_t*     /*Read*/
    )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int GetSubDirAndFilesCount(
    OUT bool*   /*IsEmpty*/,
    OUT size_t* /*SubDirsCount*/,
    OUT size_t* /*FilesCount*/
    )
  {
    return ERR_NOTIMPLEMENTED;
  }


  virtual UINT64 GetSize() { return m_pInode->GetSize(); }

  //=================================================================
  //          END OF CDir  virtual functions
  //=================================================================

private:
  template<typename T>
  int OpenEx(
    IN  const FileInfo&  fi,
    IN  list_head&       list,
    IN  CUnixInode*      pInode,
    OUT T**              fso
    )
  {
    T* pNewFso;
    CHECK_CALL(CreateInitFSObject(fi, pInode, &pNewFso));

    if (fso != NULL)
    {
      *fso = pNewFso;
      pNewFso->m_Entry.insert_after(&list);
    }
    else
      pNewFso->Destroy();

    return ERR_NOERROR;
  }

  virtual int FindNext(
    IN CEntryNumerator* pEnum,
    OUT FileInfo&       Info,
    OUT CUnixInode**    pInode    // NULL if Inode doesn't required (will released inside)
  ) = 0;

  template<typename T>
  int CreateInitFSObject(
    IN const FileInfo&  fi,
    IN CUnixInode*      pInode,
    OUT T**             fso
    )
  {
    CreateFsObject(&fi, fso);
    CHECK_PTR(*fso);
#ifdef UFSD_DRIVER_LINUX
    int Status = (*fso)->Init(fi.Id, this, pInode, api::StrUTF8, fi.Name, fi.NameLen);
#else
    int Status = (*fso)->Init(fi.Id, this, pInode, fi.NameType, fi.Name, fi.NameLen);
#endif

    if (!UFSD_SUCCESS(Status))
    {
      (*fso)->Destroy();
      *fso = NULL;
    }

    return Status;
  }

  template<typename T>
  int OpenFSObject(
    IN  api::STRType  type,
    IN  const void*   name,
    IN  size_t        name_len,
    IN  FSObjectType  fsoType,
    OUT T**           fso
    )
  {
    CFSObject* pObj;

    CHECK_CALL_SILENT(Open(type, name, name_len, &pObj, NULL));
    if (pObj->m_tObjectType != fsoType)
    {
      pObj->Destroy();
      return ERR_NOFILEEXISTS;
    }

    *fso = static_cast<T*>(pObj);

    return ERR_NOERROR;
  }

  virtual int FindDirEntry(
    IN  api::STRType  type,
    IN  const void*   name,
    IN  size_t        name_len,
    OUT FileInfo*     info,
    OUT CUnixInode**  pInode
    ) = 0;

  virtual void CreateFsObject(
    IN  const FileInfo* info,
    OUT CUnixDir**      fso
  ) const = 0;

  virtual int InitEnumerator(
    IN  size_t            Options,
    OUT CUnixEnumerator** pEnum,
    IN  CUnixInode*       pInode = NULL
    ) const = 0;

  virtual void CreateFsObject(
    IN  const FileInfo* info,
    OUT CUnixFile**     fso
  ) const;

  //internal function for destoying object called from CunixDir::CloseParent
  virtual void DestroyInternal()
  {
#ifdef UFSD_CacheAlloc
    this->~CUnixDir();
    UFSD_CacheFree(m_DirCache, this);
#else
    delete this;
#endif
  }

  //return ERR_DIRNOTEMPTY if directory is not empty, return ERR_NOERROR if directory is empty, return other error code if error
  virtual int CheckForEmpty(CUnixInode* pInode);

  virtual int UnlinkInternal(
    IN CUnixInode*  /*pInode*/,
    IN CFSObject*   /*Fso*/,
    IN FileInfo     /*Info*/
    ) const
  {
    return ERR_WPROTECT;
  }
};

} //namespace UFSD


#endif   // __UFSD_UNIXDIR_H
