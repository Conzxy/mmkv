// SPDX-LICENSE-IDENTIFIER: Apache-2.0
#ifndef MMKV_UTIL_COMPILER_MACRO_H__
#define MMKV_UTIL_COMPILER_MACRO_H__

#if defined(__GNUC__) || defined(__clang__) || defined(__GNUG__)
#  define MMKV_EXPORT_ATTR     __attribute__((visibility("default")))
#  define MMKV_IMPORT_ATTR     __attribute__((visibility("default")))
#  define MMKV_DEPRECATED_ATTR __attribute__((__deprecated__))
#  define MMKV_NO_EXPORT       __attribute__((visibility("hidden")))
#elif defined(_MSC_VER)
#  define MMKV_EXPORT_ATTR     __declspec(dllexport)
#  define MMKV_IMPORT_ATTR     __declspec(dllimport)
#  define MMKV_DEPRECATED_ATTR __declspec(deprecated)
#  define MMKV_NO_EXPORT
#endif //! defined(__GNUC__) || defined(__clang__) || defined(__GNUG__)

// For GCC,
// There is no need to specify MMKV_BUILDING_DSO since MMKV_API no
// change. For archive, you no need to specify MMKV_STATIC_DEFINE since gcc
// will ignore it. (However, you had better specify it, readelf is different, it
// is a LOCAL symbol).
//
// For MSVC, You should specify MMKV_STATID_DEFINE for using and
// building archive. Similar, you should specify MMKV_BUILDIG_DSO for building
// DSO, but don't specify anything for using. i.e. using DSO is default

#ifdef MMKV_LINK_SHARED
#  define MMKV_LINK_CORE_SHARED
#  define MMKV_LINK_NET_SHARED
#endif

#ifdef MMKV_LINK_CORE_SHARED
#  define MMKV_API    MMKV_IMPORT_ATTR
#  define MMKV_NO_API MMKV_NO_EXPORT
#elif defined(MMKV_BUILD_CORE_SHARED)
#  define MMKV_API    MMKV_EXPORT_ATTR
#  define MMKV_NO_API MMKV_NO_EXPORT
#else
#  define MMKV_API
#  define MMKV_NO_API
#endif //! MMKV_LINK_CORE_SHARED

#if __cplusplus >= 201402L
#  define MMKV_DEPRECATED(msg) [[deprecated(msg)]]
#elif defined(__GUNC__) || defined(__clang__)
#  define MMKV_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#  define MMKV_DEPRECATED(msg) __declspec(deprecated(msg))
#endif

#define MMKV_DEPRECATED_API(msg)    MMKV_DEPRECATED(msg) MMKV_API
#define MMKV_DEPRECATED_NO_API(msg) MMKV_DEPRECATED(msg) MMKV_NO_API

#if defined(__GNUC__) || defined(__clang__)
#  define MMKV___THREAD_DEFINED 1
#else
#  define MMKV___THREAD_DEFINED 0
#endif //! defined(__GNUC__) || defined(__clang__)

#endif //! MMKV_UTIL_COMPILER_MACRO_H__
