// <copyright file="crc32.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
////////////////////////////////////////////////////////////////
//
// This file contains implementation of function's for caclculating crc
//

namespace UFSD {

unsigned short crc16(unsigned short crc, const void* pBuffer, size_t BufSize);

unsigned int crc32c(unsigned int crc, const void* pBuffer, size_t BufSize);
};
