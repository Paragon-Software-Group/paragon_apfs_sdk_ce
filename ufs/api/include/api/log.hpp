// <copyright file="log.hpp" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>

#ifndef __API_LOG_H__
#define __API_LOG_H__

#pragma once

#include <api/types.hpp>

namespace api {

// error natures
#define ERROR_NATURE_PSG        0x00    // internal error
#define ERROR_NATURE_OS         0x01    // operating system error
#define ERROR_NATURE_ESX        0x02    // ESX error

// logging levels
#define LOG_LEVEL_ERROR         0x01    // errors
#define LOG_LEVEL_WARNING       0x02    // incorrect events
#define LOG_LEVEL_MESSAGE       0x04    // main top level log information
#define LOG_LEVEL_INFO          0x08    // additional log information (the default logging level)
#define LOG_LEVEL_TRACE         0x10    // trace
#define LOG_LEVEL_DEBUG1        0x20    // debug information 1
#define LOG_LEVEL_DEBUG2        0x40    // debug information 2
#define LOG_LEVEL_DEBUG3        0x80    // debug information 3
// custom levels 0x100 and over
//...

// log masks
#define LOG_MASK_WARNING        (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING)
#define LOG_MASK_MESSAGE        (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING|LOG_LEVEL_MESSAGE)
#define LOG_MASK_INFO           (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING|LOG_LEVEL_MESSAGE|LOG_LEVEL_INFO)
#define LOG_MASK_TRACE          (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING|LOG_LEVEL_MESSAGE|LOG_LEVEL_INFO|LOG_LEVEL_TRACE)
#define LOG_MASK_DEBUG1         (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING|LOG_LEVEL_MESSAGE|LOG_LEVEL_INFO|LOG_LEVEL_TRACE|LOG_LEVEL_DEBUG1)
#define LOG_MASK_DEBUG2         (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING|LOG_LEVEL_MESSAGE|LOG_LEVEL_INFO|LOG_LEVEL_TRACE|LOG_LEVEL_DEBUG1|LOG_LEVEL_DEBUG2)
#define LOG_MASK_DEBUG3         (LOG_LEVEL_ERROR|LOG_LEVEL_WARNING|LOG_LEVEL_MESSAGE|LOG_LEVEL_INFO|LOG_LEVEL_TRACE|LOG_LEVEL_DEBUG1|LOG_LEVEL_DEBUG2|LOG_LEVEL_DEBUG3)
#define LOG_MASK_ALL            0xff
#define LOG_MASK_FULL           0xffffffff

#define LOG_MASK_DEFAULT        LOG_MASK_INFO

//!!! for using SLOG_ERROR put the following s_pFileName declaration in beginning of file
//static const char s_pFileName [] = __FILE__ ",$Revision: 331860 $";

//#define NO_LOGGING

#if !defined _MSC_VER || _MSC_VER >= 1400
#ifndef NO_LOGGING
  #define LOG_WARNING(log, msg, ...)          do { if(log) (log)->Trace(LOG_LEVEL_WARNING,  0, (msg), ##__VA_ARGS__); } while((void)0,0)
  #define LOG_MESSAGE(log, msg, ...)          do { if(log) (log)->Trace(LOG_LEVEL_MESSAGE,  2, (msg), ##__VA_ARGS__); } while((void)0,0)
  #define LOG_INFO(log, msg, ...)             do { if(log) (log)->Trace(LOG_LEVEL_INFO,     4, (msg), ##__VA_ARGS__); } while((void)0,0)
  #define LOG_TRACE(log, msg, ...)            do { if(log) (log)->Trace(LOG_LEVEL_TRACE,    6, (msg), ##__VA_ARGS__); } while((void)0,0)

  #define LOG_DEBUG1(log, msg, ...)           do { if(log) (log)->Trace(LOG_LEVEL_DEBUG1,   8, (msg), ##__VA_ARGS__); } while((void)0,0)
  #define LOG_DEBUG2(log, msg, ...)           do { if(log) (log)->Trace(LOG_LEVEL_DEBUG2,  10, (msg), ##__VA_ARGS__); } while((void)0,0)
  #define LOG_DEBUG3(log, msg, ...)           do { if(log) (log)->Trace(LOG_LEVEL_DEBUG3,  12, (msg), ##__VA_ARGS__); } while((void)0,0)

  #define LOG_ERROR(log, code, ...)           do { if(log) (log)->Error((code), __FILE__, __LINE__, ##__VA_ARGS__); } while((void)0,0)
  #define SLOG_ERROR(log, code, ...)          do { if(log) (log)->Error((code), s_pFileName, __LINE__, ##__VA_ARGS__); } while((void)0,0)

  #define LOG_DUMP(log, level, buf, len, ...) do { if(log) (log)->Dump((level), 6, (buf), (len), ##__VA_ARGS__); } while((void)0,0)
#else
  #define LOG_WARNING(log, msg, ...)          (void)(log)
  #define LOG_MESSAGE(log, msg, ...)          (void)(log)
  #define LOG_INFO(log, msg, ...)             (void)(log)
  #define LOG_TRACE(log, msg, ...)            (void)(log)

  #define LOG_DEBUG1(log, msg, ...)           (void)(log)
  #define LOG_DEBUG2(log, msg, ...)           (void)(log)
  #define LOG_DEBUG3(log, msg, ...)           (void)(log)

  #define LOG_ERROR(log, code, ...)           (void)(log)
  #define SLOG_ERROR(log, code, ...)          (void)(log)

  #define LOG_DUMP(log, level, buf, len, ...) (void)(log)
#endif
#endif

class BASE_ABSTRACT_CLASS IBaseLog
{
public:

    // tracing
    virtual bool        CanTrace(
        unsigned int        LogLevel
    ) = 0;

    virtual int         Trace(
        unsigned int        LogLevel,       //0-meance LOG_LEVEL_INFO
        int                 PosIndent,
        const char*         Message,
        ...
    ) = 0;

    virtual int         Dump(
        unsigned int        LogLevel,
        int                 PosIndent,
        const void*         Buffer,
        size_t              Length
    ) = 0;

    virtual int         Dump(
        unsigned int        LogLevel,
        int                 PosIndent,
        const void*         Buffer,
        size_t              Length,
        const char*         Message,
        ...
    ) = 0;

    // error handling
    virtual int         Error(
        int                 Code,
        const char*         FileName,
        size_t              LineNumber
    ) = 0;

    virtual int         Error(
        int                 Code,
        const char*         FileName,
        size_t              LineNumber,
        const char*         Message,
        ...
    ) = 0;

    //RootError() is trigger function
    //For reseting ClearRootError() must be called
    //else any next call of RootError() will return error
    virtual int         RootError(
        int                 Code,
        unsigned int        Nature,         //ERROR_NATURE_ XXX
        const char*         Description,
        const char*         FileName,
        size_t              LineNumber
    ) = 0;

    virtual int         RootError(
        int                 Code,
        unsigned int        Nature,         //ERROR_NATURE_ XXX
        const char*         Description,
        const char*         FileName,
        size_t              LineNumber,
        const char*         Message,
        ...
    ) = 0;

    //RootError must be accessed at the any Log hierarchy
    virtual int         GetRootError(
        int*                RootError,      //saved root error code
        unsigned int*       Nature          //ERROR_NATURE_ XXX
    ) = 0;

    virtual int         GetRootDescription(
        char*               Buff,
        size_t              BuffSize,
        size_t*             DescriptionSize // real size of description
    ) = 0;

    //attach params to root error, while processing occured error upward in callstack 
    virtual int AddRootErrorParam(
        const char* moduleName,
        const char* param,
        const char* value
    ) = 0;

    virtual int GetRootErrorParams(
        char *buf,
        size_t len,
        size_t *n
    ) = 0;

    virtual void        ClearRootError() = 0;
};

class BASE_ABSTRACT_CLASS ILog : public IBaseLog
{
public:

    //get current or default log level
    virtual unsigned int GetLogMask(
        bool                fDefault
    ) = 0;

    virtual int         GetIndent() = 0;

    virtual int         CreateLog(
        const char*         ModuleName,
        ILog**              Log
    ) = 0;

    virtual int         CreateLog(
        const char*         ModuleName,
        unsigned int        LogMask,
        int                 PosIndent,
        ILog**              Log
    ) = 0;

    virtual int         AddRef() = 0;
    virtual int         Release() = 0;
};

} // namespace api

#endif //__API_LOG_H__
