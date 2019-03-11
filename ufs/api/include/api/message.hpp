// <copyright file="message.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#pragma once

#include <api/types.hpp>

namespace api {

/*
*---------------------------------------------------------*
*                 Progress dialog                         *
*---------------------------------------------------------*
* |---------------------------------------------------|   *
* |  Log messages goes here                           |   *
* |  scrollable window                                |   *
* |                                                   |   *
* |---------------------------------------------------|   *
*                                                         *
*                                                         *
* |---------------------------------------------------|   *
* |  rewritable messages goes here                    |   *
* |---------------------------------------------------|   *
*                                                         *
*---------------------------------------------------------*

  Typical sequence of operation:

  - OpenBar();
  - PutMessage( Title, "Test" );

  - NewProgress( Max1, "Cycle 1" )
  for ( i = 0; i < Max1; ++i )
  {
    - SetPos( i );
    - PutMessage( Cur, "%d", i );
  }

  - NewProgress( Max2, " Cycle 2")
  for ( i = 0; i < Max2; ++i )
  {
    - SetPos( i );
    - PutMessage( Cur, "%d", i );
  }

  - CloseBar();


*/


// Possible values for first argument of PutMessage
enum MsgType{
  MsgTitle,     // Set new title
  MsgLog,       // Put message in scrollable window
  MsgCur        // Put message in rewritable window
};

struct BASE_ABSTRACT_CLASS IMessage
{
  //=======================================================
  //           Progress bar related functions
  //=======================================================
  // Open progress bar.
  // Returns 0 if error
  virtual int OpenBar(
      IN const char*  szTitle, ...
      ) __attribute__ ((format (printf, 2, 3))) = 0;

  // Close progress bar.
  // Returns 0 if error
  virtual int CloseBar() = 0;

  // Starts a new progress bar with given maximum value of position
  virtual int NewProgress(
      IN size_t         Flags,      // see UMSG_FLAGS_XXX
      IN const UINT64&  MaxValue,
      IN const char*    szTitle, ...
      )  __attribute__ ((format (printf, 4, 5))) = 0;

  // Gets the upper limits of the range of the progress bar
  virtual UINT64 GetMax(
      ) = 0;

  // Sets the current position for a progress bar
  virtual void  SetPos(
      IN const UINT64& Pos
      ) = 0;

  // Gets the current position of the progress bar
  virtual UINT64 GetPos(
      ) = 0;

  // Advances the current position for a progress bar by the step increment
  virtual void  StepIt(
      IN const UINT64& Step = 1
      ) = 0;

  // Returns current percent value
  virtual size_t GetPercent() = 0;

  // Set information for read/write speed calculation
  virtual int SetDataRate(
      IN const UINT64& Bytes,
      IN size_t Miliseconds,
      IN unsigned int OperationCode //UMSG_OP_ XXX
      ) = 0;

  //=======================================================
  //           Messages functions
  //=======================================================

  // Put formatted message
  // Returns the number of put chars
  // NOTE: "this" is passed implicitly as a first parameter
  // so the "fmt" will be actually the third parameter
  virtual int PutMessage(
      IN MsgType      type,
      IN const char*  fmt, ...
      ) __attribute__ ((format (printf, 3, 4))) = 0;


  //=======================================================
  //  These "callback" functions allow to ask user
  //=======================================================
  // Show message box
  // Return value 0 means that request is not implemented or is not processed
  virtual size_t MessageBox(
      IN size_t         flags,
      IN const char*    fmt, ...
      ) __attribute__ ((format (printf, 3, 4)) ) = 0;

  // Call synchronous request
  // The return value depends on "code"
  // Return value 0 means that request is not implemented or is not processed
  virtual size_t SyncRequest(
      IN size_t     code,
      IN OUT void*  arg
      ) = 0;

  //=======================================================
  // During long operations user can press "Cancel" button
  // This function returns TRUE in this case
  //=======================================================
  virtual bool  IsBreak(
      IN bool bClear = false
      ) = 0;

  virtual void Destroy() = 0;
};


//=======================================================
//
// IMessage::NewProgress() flags
//
//structure [16 bits - flags][8 bits - module][8 bits - operation]

#define UMSG_FLAGS_MASK                 0xffff0000
#define UMSG_OPERATION_MASK             0x0000ffff

//=======================================================
#define UMSG_FLAGS_MAIN                 0x80000000 // This is the main subprogress in current operation sequence

#define UMSG_UNDEFINED_OPERATION        0 // for old code

//=======================================================
#define UMSG_ITE_MODULE                 0x100      // ITE module progress
#define ITE_PROGRESS_OPERATION(n)       (UMSG_ITE_MODULE|(n))

#define UMSG_ITE_SCAN_MFT               ITE_PROGRESS_OPERATION(0x01) // scan FS
#define UMSG_ITE_COLLECT_FILE_INFO      ITE_PROGRESS_OPERATION(0x02) // scan FS
#define UMSG_ITE_PROCESS_EXCLUDES       ITE_PROGRESS_OPERATION(0x03) // scan FS
#define UMSG_ITE_COPY_DATA              ITE_PROGRESS_OPERATION(0x04) // copy data by cluster
#define UMSG_ITE_COPY_DATA_BY_SECTOR    ITE_PROGRESS_OPERATION(0x05) // copy data by sectors (special volume areas or raw volume)
#define UMSG_ITE_SCAN_DIFF_SECTORS      ITE_PROGRESS_OPERATION(0x06) // diff operation
#define UMSG_ITE_WRITE_DIFF_SECTORS     ITE_PROGRESS_OPERATION(0x07) // diff operation
#define UMSG_ITE_WRITE_CHANGED_BLOCKS   ITE_PROGRESS_OPERATION(0x08) // write changed blocks
#define UMSG_ITE_CREATE_FILE_TREE       ITE_PROGRESS_OPERATION(0x09) // ITE copy operation
#define UMSG_ITE_CREATE_COPY_LIST       ITE_PROGRESS_OPERATION(0x0a) // ITE copy operation
#define UMSG_ITE_COPY_DATA_FROM_LIST    ITE_PROGRESS_OPERATION(0x0b) // ITE copy operation
#define UMSG_ITE_UNCOMPRESS_DATA        ITE_PROGRESS_OPERATION(0x0c) // ITE copy operation
#define UMSG_ITE_DEDUPLICATE_VOLUME     ITE_PROGRESS_OPERATION(0x0d) // volume deduplication
#define UMSG_ITE_RESTORE_VOLUME         ITE_PROGRESS_OPERATION(0x0e) // volume restore deduplicated
#define UMSG_ITE_DEDUPLICATE_FILES      ITE_PROGRESS_OPERATION(0x0f) // files deduplication
#define UMSG_ITE_CALC_HASH              ITE_PROGRESS_OPERATION(0x10) // calc files deduplication hash

//=======================================================
#define UMSG_UFSD_MODULE                0x200      // UFSD_SDK module progress
#define UFSD_PROGRESS_OPERATION(n)      (UMSG_UFSD_MODULE|(n))

//
// IMessage::MessageBox() flags
// (NOTE: you can pass this flag in Win32 MessageBox) as is
//

#define UMSG_MB_OK                       0x00000000
#define UMSG_MB_OKCANCEL                 0x00000001
#define UMSG_MB_ABORTRETRYIGNORE         0x00000002
#define UMSG_MB_YESNOCANCEL              0x00000003
#define UMSG_MB_YESNO                    0x00000004
#define UMSG_MB_RETRYCANCEL              0x00000005


//
// Predefined returned values of IMessage::MessageBox
//

#define UMSG_IDOK                1
#define UMSG_IDCANCEL            2
#define UMSG_IDABORT             3
#define UMSG_IDRETRY             4
#define UMSG_IDIGNORE            5
#define UMSG_IDYES               6
#define UMSG_IDNO                7

#define UMSG_OP_READ             0
#define UMSG_OP_WRITE            1

} // namespace api

#endif //__MESSAGE_H__
