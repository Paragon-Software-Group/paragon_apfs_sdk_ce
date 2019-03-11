// <copyright file="ufsd.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
#ifndef _INCLUDE_UFSD_SDK
#define _INCLUDE_UFSD_SDK

// Don't
// #include <api.hpp>

// Include only what we need
#include <api/export.hpp>
#include <api/types.hpp>
#include <api/errors.hpp>
#include <api/memory_mgm.hpp>
#include <api/string.hpp>
#include <api/log.hpp>
#include <api/time.hpp>
#include <api/rwb.hpp>
#include <api/fileio.hpp>
#include <api/message.hpp>
#include <api/cipher.hpp>
#include <api/hash.hpp>


#include "ufsd/u_mmngr.h"
#include "ufsd/u_types.h"
#include "ufsd/u_errors.h"
#include "ufsd/u_list.h"
#include "ufsd/u_rwblck.h"
#include "ufsd/u_fsbase.h"
#include "ufsd/u_chkdsk.h"
#include "ufsd/u_defrag.h"
#include "ufsd/u_ioctl.h"
#include "ufsd/u_ufsd.h"

#endif //_INCLUDE_UFSD_SDK
