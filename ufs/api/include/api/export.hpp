// <copyright file="export.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#pragma once

#ifdef ufs_api_SHARED_DEFINE
#   ifndef UFS_API_EXPORT
#       ifdef ufs_api_EXPORTS
#           if defined _WIN32 
#               define UFS_API_EXPORT __declspec(dllexport)
#           elif defined __GNUC__ || defined __clang__
#               define UFS_API_EXPORT __attribute__((visibility("default")))
#           endif
#       else
#           if defined _WIN32 
#               define UFS_API_EXPORT __declspec(dllimport)
#           elif defined __GNUC__ || defined __clang__
#               define UFS_API_EXPORT __attribute__((visibility("default")))
#           endif
#       endif
#   endif
#   ifndef UFS_API_NO_EXPORT
#       if defined _WIN32 
#           define UFS_API_NO_EXPORT
#       elif defined __GNUC__ || defined __clang__
#           define UFS_API_NO_EXPORT __attribute__((visibility("hidden")))
#       endif
#   endif
#else 
#   define UFS_API_EXPORT
#   define UFS_API_NO_EXPORT
#endif

#ifndef UFS_API_DEPRECATED
#   if defined _WIN32 
#       define UFS_API_DEPRECATED __declspec(deprecated)
#   elif defined __GNUC__ || defined __clang__
#       define UFS_API_DEPRECATED __attribute__ ((__deprecated__))
#   endif
#endif

#ifndef UFS_API_DEPRECATED_EXPORT
#  define UFS_API_DEPRECATED_EXPORT UFS_API_EXPORT UFS_API_DEPRECATED
#endif

#ifndef UFS_API_DEPRECATED_NO_EXPORT
#  define UFS_API_DEPRECATED_NO_EXPORT UFS_API_NO_EXPORT UFS_API_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define UFS_API_NO_DEPRECATED
#endif
