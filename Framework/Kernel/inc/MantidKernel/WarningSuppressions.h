// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_WARNINGSUPPRESSIONS_H_
#define MANTID_KERNEL_WARNINGSUPPRESSIONS_H_

/*  A system-wide file to contain, e.g., useful system-dependent macros
    for suppressing compiler warnings.
*/

/**
 * This is a collection of macros for turning compiler warnings off
 * in a controlled manner. The work is based on
 * http://dbp-consulting.com/tutorials/SuppressingGCCWarnings.html
 */
#if defined(__clang__)
#define CLANG_VERSION ((__clang_major__ * 100) + __clang_minor__)
#endif

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER)) && !defined(__clang__)
#define GCC_VERSION                                                            \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#if defined(GCC_VERSION) || defined(CLANG_VERSION)
// things to make the macros clearer
#define GNU_DIAG_MAKE_WARNING(x) "-W" x
#define GNU_DIAG_DO_PRAGMA(x) _Pragma(#x)

#if defined(GCC_VERSION)
#define GNU_DIAG_PRAGMA(x) GNU_DIAG_DO_PRAGMA(GCC diagnostic x)
#else
#define GNU_DIAG_PRAGMA(x) GNU_DIAG_DO_PRAGMA(clang diagnostic x)
#endif

// the following were previously defined in Poco/Platform_POSIX.h
#ifdef GNU_DIAG_ON
#undef GNU_DIAG_ON
#endif
#ifdef GNU_DIAG_OFF
#undef GNU_DIAG_OFF
#endif

#if defined(GCC_VERSION)
// define macros for turning the warning suppression on/off for GCC
// note that we turn off unknown warnings here as well so that we can use the
// same macro for GCC and clang.
// clang-format off
#define GNU_DIAG_OFF(x)                                                          \
  GNU_DIAG_PRAGMA(push)                                                          \
  GNU_DIAG_PRAGMA(ignored GNU_DIAG_MAKE_WARNING("pragmas"))                      \
  GNU_DIAG_PRAGMA(ignored GNU_DIAG_MAKE_WARNING(x))
#define GNU_DIAG_ON(x) GNU_DIAG_PRAGMA(pop)

// clang-format on

#elif defined(CLANG_VERSION) && CLANG_VERSION >= 306
// define macros for turning the warning suppression on/off for clang
// note that we turn off unknown warnings here as well so that we can use the
// same macro for GCC and clang.
// clang-format off
#define GNU_DIAG_OFF(x)                                                         \
  GNU_DIAG_PRAGMA(push)                                                         \
  GNU_DIAG_PRAGMA(ignored GNU_DIAG_MAKE_WARNING("unknown-pragmas"))             \
  GNU_DIAG_PRAGMA(ignored GNU_DIAG_MAKE_WARNING("unknown-warning-option"))      \
  GNU_DIAG_PRAGMA(ignored GNU_DIAG_MAKE_WARNING(x))
#define GNU_DIAG_ON(x) GNU_DIAG_PRAGMA(pop)
// clang-format on
#endif
#else
// neither clang or GCC
#define GNU_DIAG_OFF(x)
#define GNU_DIAG_ON(x)
#endif

// Similar macros for MSVC
#if defined(_MSC_VER)
// clang-format off
#define MSVC_DIAG_OFF(id)                                                        \
  __pragma(warning(push))                                                        \
  __pragma(warning(disable : id))
#define MSVC_DIAG_ON(id) __pragma(warning(pop))
// clang-format on
#else
#define MSVC_DIAG_OFF(x)
#define MSVC_DIAG_ON(x)
#endif

// Defining this macro separately since clang-tidy tries to add spaces around
// the hyphen and we use it in a lot of test files.
// clang-format off
#if defined(__cplusplus) && defined(GCC_VERSION) && GCC_VERSION >= 50000
#define GNU_DIAG_OFF_SUGGEST_OVERRIDE GNU_DIAG_OFF("suggest-override")
#define GNU_DIAG_ON_SUGGEST_OVERRIDE GNU_DIAG_ON("suggest-override")
#elif defined(__cplusplus) && defined(CLANG_VERSION) && CLANG_VERSION >= 306
#define GNU_DIAG_OFF_SUGGEST_OVERRIDE GNU_DIAG_OFF("inconsistent-missing-override")
#define GNU_DIAG_ON_SUGGEST_OVERRIDE GNU_DIAG_ON("inconsistent-missing-override")
#else
#define GNU_DIAG_OFF_SUGGEST_OVERRIDE
#define GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif
// clang-format on

#if defined(GCC_VERSION) || defined(CLANG_VERSION)
#define GNU_UNUSED_FUNCTION __attribute__((unused))
#else
#define GNU_UNUSED_FUNCTION
#endif

#endif /*MANTID_KERNEL_WARNINGSUPPRESSIONS_H_*/
