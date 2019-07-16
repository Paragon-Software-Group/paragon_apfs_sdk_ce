# Set the minimum required version of cmake for a project.
cmake_minimum_required(VERSION 2.8)

# Disallow source modifications.
set(CMAKE_DISABLE_SOURCE_CHANGES ON)

# Disallow in-source builds.
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Project name (same as package name).
set(_project_name apfsutil)

# Set project type (native or managed).
set(_project_type native)

# Add output directory for shared lib and executive files project.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    CACHE PATH "Directory where .exe and .dll files are dumped.")

# If make install is invoked or INSTALL is built, this directory is prepended
# onto all install directories.
set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/../${_project_name}.bin
    CACHE PATH "Install destination directory.")

# Specifies the available build types on multi-config generators.
set(CMAKE_CONFIGURATION_TYPES Debug RelWithDebInfo
    CACHE INTERNAL "Allowed configuration types.")

# Set a name, version, and enable languages for the entire project.
project(${_project_name} CXX)

# Project version.
set(${_project_name}_major 1)
set(${_project_name}_minor 0)
set(${_project_name}_patch 0)
set(${_project_name}_prerelease_suffix "-alpha")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DNDEBUG)
else()
    add_definitions(-DUFSD_TRACE
                    -DUFSD_TRACE_ASSERT
                    -DUFSD_TRACE_ERROR
                   )
endif()

set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_CXX_STANDARD 98)          # CMake 3.1+
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Set default toolchains.
#
if(WIN32)
    set(PLATFORM_TYPE WINDOWS)
    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_SYSTEM_PROCESSOR "AMD64")
endif()

if(MSVC)
    set(CMAKE_GENERATOR_PLATFORM x64)
    set(CMAKE_GENERATOR_TOOLSET v140_xp)
    add_definitions(-DWINVER=0x501)
    add_definitions(-D_WIN32_WINNT=0x501)
    add_definitions(-D_WIN32_IE=0x600)
    add_definitions(-DVSS_WS03)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "/W4 ${CMAKE_CXX_FLAGS}")
elseif(MINGW)
    set(CMAKE_CXX_FLAGS "-Wno-format -Wno-unknown-pragmas ${CMAKE_CXX_FLAGS}")

    include(CheckStructHasMember)
    set(_test_headers windows.h winioctl.h)
    CHECK_STRUCT_HAS_MEMBER("struct _SET_PARTITION_INFORMATION_EX" PartitionStyle "${_test_headers}" HAVE_SET_PARTITION_INFORMATION_EX LANGUAGE CXX)
    if(HAVE_SET_PARTITION_INFORMATION_EX)
        add_definitions(-DUFSD_WITH_SET_PARTITION)
    endif()
    add_definitions(-D_CONSOLE)
elseif(UNIX)
    if(APPLE)
        set(CMAKE_SYSTEM_NAME Darwin)
    else()
        set(CMAKE_SYSTEM_NAME Linux)
        set(PLATFORM_TYPE LINUX)
    endif()

    execute_process(COMMAND "uname" "-m" OUTPUT_VARIABLE HOST_ARCH)

    if("${HOST_ARCH}" MATCHES "^x86_64")
        set(CMAKE_SYSTEM_PROCESSOR "x86_64")
        set(CMAKE_CXX_FLAGS "-m64 ${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_SYSTEM_PROCESSOR "i686")
        set(CMAKE_CXX_FLAGS "-m32 ${CMAKE_CXX_FLAGS}")
    endif()
else()
    message( FATAL_ERROR "Unknown configuration" )
endif()

if(UNIX OR MINGW)
    set(CMAKE_CXX_FLAGS "-fPIC -fno-common -fmerge-all-constants -fstrict-aliasing -fno-exceptions -fno-asynchronous-unwind-tables ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "-Wall -Wstrict-aliasing -Wno-unused-local-typedefs ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS "-pipe -std=c++98 ${CMAKE_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

#
# Supported platforms for each component.
#
set(${_project_name}
    WINDOWS
    LINUX
    MACOS
   )

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ufs" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/linutil")
    set(_baseapi  ufs/api)
    set(_ufsd_sdk ufs/ufsd)
    set(_linutil  linutil)
    set(_crypt    crypt)
elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../ufs" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../linutil")
    set(_baseapi  ../../ufs/api)
    set(_ufsd_sdk ../../ufs/ufsd)
    set(_linutil  ../../linutil)
    set(_crypt    ../../linutil/apfsutil/crypt)
else()
    message( FATAL_ERROR "Cannot locate ufs and linutil directories" )
endif()

# List of header files.
set(_api_headers
    ${_baseapi}/include/api/assert.hpp
    ${_baseapi}/include/api/cipher.hpp
    ${_baseapi}/include/api/crypt.hpp
    ${_baseapi}/include/api/compress.hpp
    ${_baseapi}/include/api/errors.hpp
    ${_baseapi}/include/api/export.hpp
    ${_baseapi}/include/api/fileio.hpp
    ${_baseapi}/include/api/fsattribs.hpp
    ${_baseapi}/include/api/hash.hpp
    ${_baseapi}/include/api/log.hpp
    ${_baseapi}/include/api/memory_mgm.hpp
    ${_baseapi}/include/api/message.hpp
    ${_baseapi}/include/api/rwb.hpp
    ${_baseapi}/include/api/string.hpp
    ${_baseapi}/include/api/time.hpp
    ${_baseapi}/include/api/types.hpp
    )

set(_ufsd_headers
    ${_ufsd_sdk}/include/ufsd.h
    ${_ufsd_sdk}/include/ufsd/ufsd_export.hpp
    ${_ufsd_sdk}/include/ufsd/u_chkdsk.h
    ${_ufsd_sdk}/include/ufsd/u_defrag.h
    ${_ufsd_sdk}/include/ufsd/u_errors.h
    ${_ufsd_sdk}/include/ufsd/u_fsbase.h
    ${_ufsd_sdk}/include/ufsd/u_ioctl.h
    ${_ufsd_sdk}/include/ufsd/u_list.h
    ${_ufsd_sdk}/include/ufsd/u_mmngr.h
    ${_ufsd_sdk}/include/ufsd/u_rwblck.h
    ${_ufsd_sdk}/include/ufsd/u_types.h
    ${_ufsd_sdk}/include/ufsd/u_ufsd.h
    )

# List of source files.
set(_linutil_sources
    ${_linutil}/funcs.h
    ${_linutil}/gettext.h
    ${_linutil}/debug.cpp
    ${_linutil}/misc.cpp
    ${_linutil}/ufsdio.cpp
    ${_linutil}/ufsdlog.cpp
    ${_linutil}/ufsdmmgr.cpp
    ${_linutil}/ufsdstr.cpp
    ${_linutil}/ufsdtime.cpp
    ${_linutil}/apfsutil.cpp
    )

set(_apfs_sources
    ${_ufsd_sdk}/src/apfs/apfs.h
    ${_ufsd_sdk}/src/apfs/apfs_struct.h
    ${_ufsd_sdk}/src/apfs/apfsbplustree.h
    ${_ufsd_sdk}/src/apfs/apfscompr.h
    ${_ufsd_sdk}/src/apfs/apfsenum.h
    ${_ufsd_sdk}/src/apfs/apfshash.h
    ${_ufsd_sdk}/src/apfs/apfsinode.h
    ${_ufsd_sdk}/src/apfs/apfssuper.h
    ${_ufsd_sdk}/src/apfs/apfstable.h
    ${_ufsd_sdk}/src/apfs/apfsvolsb.h
    ${_ufsd_sdk}/src/apfs/dirapfs.h
    ${_ufsd_sdk}/src/apfs/apfs.cpp
    ${_ufsd_sdk}/src/apfs/apfsbplustree.cpp
    ${_ufsd_sdk}/src/apfs/apfscompr.cpp
    ${_ufsd_sdk}/src/apfs/apfshash.cpp
    ${_ufsd_sdk}/src/apfs/apfssuper.cpp
    ${_ufsd_sdk}/src/apfs/apfsencryption.cpp
    ${_ufsd_sdk}/src/apfs/apfsenum.cpp
    ${_ufsd_sdk}/src/apfs/apfsinode.cpp
    ${_ufsd_sdk}/src/apfs/apfstable.cpp
    ${_ufsd_sdk}/src/apfs/apfsvolsb.cpp
    ${_ufsd_sdk}/src/apfs/apfsxattr.cpp
    ${_ufsd_sdk}/src/apfs/dirapfs.cpp
    )

set(_common_sources
    ${_ufsd_sdk}/src/common/crc32.cpp
    ${_ufsd_sdk}/src/common/fsbase.cpp
    ${_ufsd_sdk}/src/common/uavl.cpp
    )

set(_h_sources
    ${_ufsd_sdk}/src/h/assert.h
    ${_ufsd_sdk}/src/h/crc32.h
    ${_ufsd_sdk}/src/h/ucommon.h
    ${_ufsd_sdk}/src/h/uerrors.h
    ${_ufsd_sdk}/src/h/uint64.h
    ${_ufsd_sdk}/src/h/uavl.h
    ${_ufsd_sdk}/src/h/uswap.h
    ${_ufsd_sdk}/src/h/utrace.h
    ${_ufsd_sdk}/src/h/versions.h
    ${_ufsd_sdk}/src/h/verstr.h
    )

set(_lzfse_sources
    ${_ufsd_sdk}/src/lzfse/lzfse_struct.h
    ${_ufsd_sdk}/src/lzfse/ulzfse.h
    ${_ufsd_sdk}/src/lzfse/ulzfse.cpp
    )

set(_unixfs_sources
    ${_ufsd_sdk}/src/unixfs/unixblock.cpp
    ${_ufsd_sdk}/src/unixfs/unixdir.cpp
    ${_ufsd_sdk}/src/unixfs/unixenum.cpp
    ${_ufsd_sdk}/src/unixfs/unixfile.cpp
    ${_ufsd_sdk}/src/unixfs/unixfs.cpp
    ${_ufsd_sdk}/src/unixfs/unixinode.cpp
    ${_ufsd_sdk}/src/unixfs/unixsuperblock.cpp
    ${_ufsd_sdk}/src/unixfs/unixblock.h
    ${_ufsd_sdk}/src/unixfs/unixdir.h
    ${_ufsd_sdk}/src/unixfs/unixenum.h
    ${_ufsd_sdk}/src/unixfs/unixfile.h
    ${_ufsd_sdk}/src/unixfs/unixfs.h
    ${_ufsd_sdk}/src/unixfs/unixinode.h
    ${_ufsd_sdk}/src/unixfs/unixsuperblock.h
    )

set(_zlib_sources
    ${_ufsd_sdk}/src/zlib/crc32.h
    ${_ufsd_sdk}/src/zlib/inffast.h
    ${_ufsd_sdk}/src/zlib/inffixed.h
    ${_ufsd_sdk}/src/zlib/inflate.h
    ${_ufsd_sdk}/src/zlib/inftrees.h
    ${_ufsd_sdk}/src/zlib/uzlib.h
    ${_ufsd_sdk}/src/zlib/zconf.h
    ${_ufsd_sdk}/src/zlib/zlib.h
    ${_ufsd_sdk}/src/zlib/zutil.h
    ${_ufsd_sdk}/src/zlib/adler32.cpp
    ${_ufsd_sdk}/src/zlib/crc32.cpp
    ${_ufsd_sdk}/src/zlib/infback.cpp
    ${_ufsd_sdk}/src/zlib/inffast.cpp
    ${_ufsd_sdk}/src/zlib/inflate.cpp
    ${_ufsd_sdk}/src/zlib/inftrees.cpp
    ${_ufsd_sdk}/src/zlib/uncompr.cpp
    ${_ufsd_sdk}/src/zlib/uzlib.cpp
    ${_ufsd_sdk}/src/zlib/zutil.cpp
    )

find_package(OpenSSL)
if(OPENSSL_FOUND)
    add_definitions(-DUFSD_WITH_OPENSSL)
    include_directories(${OPENSSL_INCLUDE_DIR})
    set(_crypto_sources
        ${_crypt}/cipher/aescipher.hpp
        ${_crypt}/cipher/cipherfactory.hpp
        ${_crypt}/hash/hash.hpp
        ${_crypt}/cipher/aescipher.cpp
        ${_crypt}/cipher/cipherfactory.cpp
        ${_crypt}/hash/hash.cpp
        )
endif()

if( EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_ufsd_sdk}/src/apfs/rw"
AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/${_ufsd_sdk}/src/unixfs/rw"
AND NOT DEFINED ENV{APFSUTIL_FORCE_RO}
AND NOT(APFSUTIL_FORCE_RO))
    message( STATUS "APFS RW included" )

    set(_apfsrw_sources
        ${_ufsd_sdk}/src/apfs/rw/apfsbitmap.h
        ${_ufsd_sdk}/src/apfs/rw/apfs.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfsbitmap.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfsbplustree.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfsenum.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfsinode.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfssuper.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfstable.cpp
        ${_ufsd_sdk}/src/apfs/rw/apfsvolsb.cpp
        ${_ufsd_sdk}/src/apfs/rw/dirapfs.cpp
        )

    set(_unixfsrw_sources
        ${_ufsd_sdk}/src/unixfs/rw/unixbitmap.h
        ${_ufsd_sdk}/src/unixfs/rw/unixbitmap.cpp
        ${_ufsd_sdk}/src/unixfs/rw/unixctl.cpp
        ${_ufsd_sdk}/src/unixfs/rw/unixdir.cpp
        ${_ufsd_sdk}/src/unixfs/rw/unixfile.cpp
        ${_ufsd_sdk}/src/unixfs/rw/unixfs.cpp
        ${_ufsd_sdk}/src/unixfs/rw/unixinode.cpp
        ${_ufsd_sdk}/src/unixfs/rw/unixsuperblock.cpp
        )

    list(APPEND _common_sources
        ${_ufsd_sdk}/src/h/bitfunc.h
        ${_ufsd_sdk}/src/h/ubitmap.h
        ${_ufsd_sdk}/src/h/urun.h
        ${_ufsd_sdk}/src/common/bitfunc.cpp
        ${_ufsd_sdk}/src/common/ubitmap.cpp
        ${_ufsd_sdk}/src/common/urun.cpp
        )

    add_definitions(-DUFSD_USE_WNDBITMAP -DUFSD_USE_SPBITMAP)
else()
    message( STATUS "APFS ReadOnly!" )
    add_definitions(-DUFSD_APFS_RO)
endif()


# Flags for sources in the current directory and below.
add_definitions(-DUFSD_APFSUTIL
                -DUFSD_LIB_STATIC
                -DUFSD_USE_SIMPLE_CUFSD
                -DSTATIC_MEMORY_MANAGER
                -DUFSD_USE_STATIC_TRACE
                )

include_directories(${_baseapi}/include ${_ufsd_sdk}/include ${_crypt}/cipher ${_crypt}/hash)

add_executable(${_project_name} ${_linutil_sources}
                                ${_api_headers}
                                ${_ufsd_headers}
                                ${_apfs_sources}
                                ${_apfsrw_sources}
                                ${_common_sources}
                                ${_h_sources}
                                ${_lzfse_sources}
                                ${_unixfs_sources}
                                ${_unixfsrw_sources}
                                ${_zlib_sources}
                                ${_crypto_sources}
              )

if(OPENSSL_FOUND)
    target_link_libraries(${_project_name} ${OPENSSL_LIBRARIES})
endif()

if(MSVC)
    source_group("api"                FILES ${_api_headers})
    source_group("ufsd\\include"      FILES ${_ufsd_headers})
    source_group("ufsd\\src\\apfs"    FILES ${_apfs_sources})
    source_group("ufsd\\src\\common"  FILES ${_common_sources})
    source_group("ufsd\\src\\h"       FILES ${_h_sources})
    source_group("ufsd\\src\\lzfse"   FILES ${_lzfse_sources})
    source_group("ufsd\\src\\unixfs"  FILES ${_unixfs_sources})
    source_group("ufsd\\src\\zlib"    FILES ${_zlib_sources})
    source_group("linutil"            FILES ${_linutil_sources})

    if(_crypto_sources)
      source_group("linutil\\crypt"   FILES ${_crypto_sources})
    endif()

    if(_apfsrw_sources)
        source_group("ufsd\\src\\apfs\\rw" FILES ${_apfsrw_sources})
    endif()

    if(_unixfsrw_sources)
        source_group("ufsd\\src\\unixfs\\rw" FILES ${_unixfsrw_sources})
    endif()
endif()
