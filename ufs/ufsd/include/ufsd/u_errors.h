// <copyright file="u_errors.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/****************************************************
 *  File UFSD/include/errors.h
 *
 *  Error codes are listed here.
 ****************************************************/

#ifndef UFSD_ERROR_BASE
  // Severity bit, Customer bit
  //#define UFSD_ERROR_BASE  0xA0000100
  const int UFSD_ERROR_BASE = 0xA0000100;
#endif

#define MAKE_UFSD_ERROR(x)  ( UFSD_ERROR_BASE | (x) )
#define IS_UFSD_ERROR(x)    ( UFSD_ERROR_BASE == (UFSD_ERROR_BASE & (x) ) )


#define ERR_BADUFSDFILE     MAKE_UFSD_ERROR(0x0008)
#define ERR_NOUFSDINIT      MAKE_UFSD_ERROR(0x0011)
// Not enough free space
#define ERR_NOSPC           MAKE_UFSD_ERROR(0x0023)

// Wrong data from ZIP archive data may be encrypted
#define ERR_WRONGDATA           MAKE_UFSD_ERROR(0x002A)

// Can't mount 'cause journal is not empty
#define ERR_NEED_REPLAY         MAKE_UFSD_ERROR(0x002B)

#define ERR_LOG_FULL            MAKE_UFSD_ERROR(0x002C)

// User aborts operation
#define ERR_UFSD_CANCELLED      MAKE_UFSD_ERROR( 0x0032)


// Maximum hardlinks count reached
#define ERR_MAXIMUM_LINK        MAKE_UFSD_ERROR( 0x0038 )

// File size is bigger than maximum allowed
// This can happen if driver with 64-bit cluster addresses creates file
// that uses more than 2^32 clusters and then this file is opened by
// another driver with 32-bit cluster addresses (in this case not all
// clusters belong to the file can be addressed).
// Or if we try to truncate/posix_fallocate the file to the size bigger
// than is supported by FS (and current mount options/driver flags).
#define ERR_FILE_TOO_BIG        MAKE_UFSD_ERROR( 0x0039 )

//
// Use this define to simplify error checking
//
#define UFSD_SUCCESS(x) ( ERR_NOERROR == (x) )

