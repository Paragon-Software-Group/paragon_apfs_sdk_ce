// <copyright file="hash.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>


#pragma once

#include <api/memory_mgm.hpp>
#include <api/hash.hpp>

namespace hash
{

/// Creates hash provider factory
int CreateHashFactory(
    api::IHashFactory **ppCompress
);

} //namespace hash
