# Paragon APFS SDK Community Edition

Paragon APFS SDK allows developers to access APFS volumes from their forensic software, applications, devices, and appliances on non-Apple platforms

## Changelog

|    Date     | Event |
|-------------|-------|
| 2017        | Create |
| June, 2019 | Publication |

## Sources

| Folder  | Content |
|---------|---------|
| linutil | sources of apfsutil and UFSD interfaces implementation |
| ufs     | UFSD SDK: interfaces, headers, Paragon APFS sources, common and 3rd-party components |

## Key Features

- Cross-platform implementation for Windows, Linux, macOS, RTOSes, and UEFI environments
- Regularly updated libraries. Every build is tested for reliability, functionality, and performance before leaving the lab
- Full read-only access on any APFS volumes:
  - Folder enumeration (both case-sensitive and case-insensitive)
  - Files reading (including cloned and compressed)
  - Get a list of file extended attributes (xattr)
  - Accessing all APFS sub-volumes
  - Reading encrypted volumes (if OpenSSL is present and detected by CMake)

## Limitations

- Read-only access to APFS volumes. A read-write version of Paragon APFS SDK will be available soon on site: https://www.paragon-software.com/
- Only first sub-volume can be accessed on 32-bit platforms
- Stable work with 16TB+ volumes is not guaranteed
- No support for hardware-encrypted APFS volumes
- No support for Big Endian platforms (MIPS, PowerPC)

## apfsutil

The utility to test and show the UFSD SDK capabilities. Usage:

```sh
$ apfsutil <test_name> [options] /dev/xxx/path/to/file/or/folder

where:
    test_name      name of the test to run (see 'User scenarios' section)

options:
   --passN=password  use password for the volume N (for encrypted volumes). APFS volumes are listed from N=1 to 100
   --trace           turn on UFSD trace
   --subvolumes      mount all sub-volumes (All sub-volumes from the container will be "mounted" in the 'Ufsd_Volumes' folder in the root)
```
For example:
```sh
$ sudo apfsutil readfile /dev/sdb1/test_folder/newfile.txt
$ sudo apfsutil enumfolder --subvolumes --pass1=qwerty --pass2=qwerty2 /dev/sdb1/Ufsd_Volumes/Untitled/test_folder
```
or Windows
```cmd
$ apfsutil enumfolder f:\test_folder
```

### Build prerequisites

- C++ compiler
- CMake
- (Optional) OpenSSL library

### How to build

#### Linux/Mac OS

```sh
$ mkdir .build
$ cd .build
$ cmake ..
$ cmake --build .
```

#### Windows

There are two ways for building apfsutil under Windows:

1. Compile with MSVC from VS2015 or later.
2. Download and install MinGW, add MinGW "bin" directory to the PATH system environment variable and run cmake with "MinGW Makefiles" generator.
```cmd
$ mkdir .build
$ cd .build
$ cmake -G "MinGW Makefiles" ..
$ cmake --build .
```
If you have both MinGW "bin" and MSYS "bin" directories in PATH, use "MSYS Makefiles" generator instead.

Warning: option --trace is available only in the debug mode. To build:
```sh
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## User scenarios

apfsutil already contains some simple user scenarios, such as

| test name  | test description |
|------------|------------------|
| enumroot   | root folder enumeration |
| enumfolder | sub-directory enumeration |
| readfile   | file reading |
| listea     | list and show all file extended attributes |
| listsubvolumes | show all sub-volumes from the container |

### Sub-volumes

apfsutil can work with all sub-volumes in an APFS container. To mount all ones, add option --subvolumes to the apfsutil arguments. Without this option, only the main (1st) volume will be mounted.
Paragon APFS SDK puts sub-volumes in the "/Ufsd_Volumes" folder. To read or enumerate files and folders on a sub-volume, its full path should be specified. For example, first of all, get a list of sub-volumes.
Note, for this user scenario the --subvolume is optional. If a required sub-volume name is known, this step can be skipped.
```sh
$ apfsutil listsubvolumes /dev/xxx
Volumes:
Untitled Volume2 MyEncryptedVolume
APFS: listsubvolumes returns 0. finished in 4 ms
```
Important: this operation is only available on the unlocked (unmounted) APFS Container!

Then enumerate the root sub-volume folder, or read any file
```sh
$ apfsutil enumfolder /dev/xxx/Ufsd_Volumes/Untitled
$ apfsutil readfile /dev/xxx/Ufsd_Volumes/Untitled/testfile.txt
```

### How to add your case

To create your own custom case, put its name (any, but not previously defined) in the list named s_Cmd (in the linutil/apfsutil.cpp) and create a command handler (function) there.

```c
static const t_CmdHandler s_Cmd[] = {
  { "enumroot"        , OnEnumRoot         },   // readdir example
  { "enumfolder"      , OnEnumFolder       },   // enumerate folder
  { "readfile"        , OnReadFile         },   // file reading
  { "listea"          , OnListEa           },   // list all extended attributes
  { "listsubvolumes"  , OnEnumSubvolumes   },   // sub-volumes enumeration
  // handlers for RW version
  { "createfile"      , OnCreateFile       },   // create file
  { "createfolder"    , OnCreateFolder     },   // create folder
  { "queryalloc"      , OnQueryAlloc       },   // list file extents (allocations)
  { "fsinfo"          , OnFsInfo           },   // get file system information
  //
  // TODO: add your handlers here
  // ...
  { NULL      , NULL },
};
```

Test handler should be of type
```c
typedef int (*HandlerFunc)(CFileSystem*, const char*);
```

Inside your specified handler all public functions from base UFSD classes CFileSystem, CFile, CDir are available (for more details see ufs\ufsd\include\ufsd\u_fsbase.h).

After rebuilding the utility selected test case will be available to start
```sh
$ sudo apfsutil my_new_case /dev/xxx/path/to/file/or/folder
```

## Read-Write access

Some operations require the full Read-Write sources (The version will be available soon on site: https://www.paragon-software.com/).
In the Community Edition these cases will return a "not implemented" error.

| test name    | test description |
|--------------|------------------|
| createfile   | file creation    |
| createfolder | folder creation  |
| queryalloc   | list file extents|
| fsinfo       | information about the file system (all sub-volumes, even encrypted) |

### fsinfo example

```sh
$ sudo apfsutil fsinfo /dev/sdb1
APFS Volume [0] 0F7AFC7E-BDF6-4546-8494-2A0C449E8D1F
  Name               : volume1 (Case-insensitive)
  Creator            : newfs_apfs (945.200.129)
  Used Clusters      : 1187894
  Reserved Clusters  : 0
  Files              : 63478
  Directories        : 1630
  Symlinks           : 5
  Special files      : 0
  Snapshots          : 0
  Encrypted          : Yes

APFS Volume [1] D45089E3-D391-4246-9490-B4EAC8D4E395
  Name               : volume2 (Case-insensitive)
  Creator            : diskmanagementd (945.200.129)
  Used Clusters      : 270
  Reserved Clusters  : 0
  Files              : 71
  Directories        : 19
  Symlinks           : 1
  Special files      : 0
  Snapshots          : 0
  Encrypted          : No

APFS Volume [2] 9D8EBA18-B110-4FCB-86FC-3E18B304628E
  Name               : volume3 (Case-insensitive)
  Creator            : diskmanagementd (945.200.129)
  Used Clusters      : 265
  Reserved Clusters  : 0
  Files              : 64
  Directories        : 18
  Symlinks           : 0
  Special files      : 0
  Snapshots          : 0
  Encrypted          : No
```

## apfsutil error codes

The utility can return following exit codes:

|   code     | description      |
|------------|------------------|
|          0 | all is OK, no error |
|         -1 | invalid command line parameters |
|         -2 | invalid or empty device name |
|         -3 | the device is mounted (linux only) |
|         -4 | no handler specified for the selected test_name |
|         -5 | misuse of the --pass option |
| 0xA0001001 | Errors from the Paragon APFS SDK |
| ...        |                                   |
| 0xA0001026 | (console output will contain an error message) |


## Licensing
The license for this project is defined in a separate document "LICENSE.txt"
Paragon APFS SDK also contains the following code:

- lzfse - Copyright (c) 2015-2016, Apple Inc. All rights reserved. https://github.com/lzfse/lzfse/blob/master/LICENSE
- zlib  - Copyright (c) 1995-2017 Jean-loup Gailly and Mark Adler
- icu   - Copyright (c) 1991-2019 Unicode, Inc. All rights reserved. https://www.unicode.org/copyright.html.
