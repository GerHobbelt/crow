#pragma once
// settings for crow
// TODO(ipkn) replace with runtime config. libucl?

/* #ifdef - enables debug mode */
#define CROW_ENABLE_DEBUG

/* #ifdef - disable timestamp in logs */
//#define CROW_DISABLE_LOG_TIMESTAMP

/* #ifdef - disable prefix in logs */
//#define CROW_DISABLE_LOG_PREFIX

/* #ifdef - enables logging */
#define CROW_ENABLE_LOGGING

/* #ifdef - enforces section 5.2 and 6.1 of RFC6455 (only accepting masked messages from clients) */
//#define CROW_ENFORCE_WS_SPEC

/* #define - specifies log level */
/*
    Debug       = 0
    Info        = 1
    Warning     = 2
    Error       = 3
    Critical    = 4

    default to INFO
*/
#ifndef CROW_LOG_LEVEL
#define CROW_LOG_LEVEL 1
#endif

#ifndef CROW_STATIC_DIRECTORY
#define CROW_STATIC_DIRECTORY "static/"
#endif
#ifndef CROW_STATIC_ENDPOINT
#define CROW_STATIC_ENDPOINT "/static/<path>"
#endif

// compiler flags
#if defined(_MSVC_LANG) && _MSVC_LANG >= 201402L
#define CROW_CAN_USE_CPP14
#endif
#if __cplusplus >= 201402L
#define CROW_CAN_USE_CPP14
#endif

#if defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#define CROW_CAN_USE_CPP17
#endif
#if __cplusplus >= 201703L
#define CROW_CAN_USE_CPP17
#if defined(__GNUC__) && __GNUC__ < 8
#define CROW_FILESYSTEM_IS_EXPERIMENTAL
#endif
#endif

#if defined(_MSC_VER)
#if _MSC_VER < 1900
#define CROW_MSVC_WORKAROUND
#define constexpr const
#define noexcept throw()
#endif

//
// shut up boost::asio warning
//
// We support Windows 10 only, unless someone happens to still have Win 8 and is willing to test our goods
//                                                                                       [Ger Hobbelt]
//
#if !defined(_WIN32_WINNT_WIN10)
#define _WIN32_WINNT_WINXP                  0x0501 // Windows XP
#define _WIN32_WINNT_WS03                   0x0502 // Windows Server 2003
#define _WIN32_WINNT_WIN6                   0x0600 // Windows Vista
#define _WIN32_WINNT_VISTA                  0x0600 // Windows Vista
#define _WIN32_WINNT_WS08                   0x0600 // Windows Server 2008
#define _WIN32_WINNT_LONGHORN               0x0600 // Windows Vista
#define _WIN32_WINNT_WIN7                   0x0601 // Windows 7
#define _WIN32_WINNT_WIN8                   0x0602 // Windows 8
#define _WIN32_WINNT_WINBLUE                0x0603 // Windows 8.1
#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 // Windows 10
#define _WIN32_WINNT_WIN10                  0x0A00 // Windows 10
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif
#endif

#if defined(__GNUC__) && __GNUC__ == 8 && __GNUC_MINOR__ < 4
#if __cplusplus > 201103L
#define CROW_GCC83_WORKAROUND
#else
#error "GCC 8.1 - 8.3 has a bug that prevents Crow from compiling with C++11. Please update GCC to > 8.3 or use C++ > 11."
#endif
#endif

// ILP64 vs LLP64 vs LP64 vs LP32; https://github.com/cpredef/predef/blob/master/DataModels.md
//
// why sizeof(int) checks won't work:
// https://stackoverflow.com/questions/4079243/how-can-i-use-sizeof-in-a-preprocessor-macro
// https://stackoverflow.com/questions/5272825/detecting-64bit-compile-in-c
#include <cstdint>
#include <limits.h>

#if (INTPTR_MAX == INT64_MAX) || defined(__LP64__) || defined (__x86_64__)
// 64-bit

#if INT_MAX == LONG_MAX
// LLP64
#if defined(__LP64__)
#error "Unexpected ILP64 mode"
#endif
#if INT_MAX == LLONG_MAX
#define __ILP64__ 1
#else
#define __LLP64__ 1
#endif
#else
// LP64
#if !defined(__LP64__)
#define __LP64__ 1
#endif
#endif

#elif (INTPTR_MAX == INT32_MAX) || defined(__ILP32__)
// 32-bit

#if (INTPTR_MAX == INT_MAX)
#if !defined(__ILP32__)
#define __ILP32__		1
#endif
#else
#if defined(__ILP32__)
#error "Unexpected ILP32 mode"
#endif
#define __LP32__		1
#endif

#else
#error "LP32/ILP64/LLP64: Unknown pointer size or missing size macros!"
#endif
