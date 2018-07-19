#ifndef MANTID_KERNEL_WARNINGSUPPRESSIONS_H_
#define MANTID_KERNEL_WARNINGSUPPRESSIONS_H_

/*  A system-wide file to contain, e.g., useful system-dependent macros
    for suppressing compiler warnings.

    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
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

// Currently this is only defined for gcc
#if defined(GCC_VERSION) || defined(CLANG_VERSION)
// how to use a pragma in a macro
#define PRAGMA(x) _Pragma(#x)

// things to make the macros clearer
#define DIAG_STR(s) #s
// undefine definition from Poco 1.6
#ifdef DIAG_JOINSTR
#undef DIAG_JOINSTR
#endif
#define DIAG_JOINSTR(x, y) DIAG_STR(x##y)
// undefine definition from Poco 1.6
#ifdef DIAG_DO_PRAGMA
#undef DIAG_DO_PRAGMA
#endif
#define DIAG_DO_PRAGMA(x) _Pragma(#x)

#if defined(GCC_VERSION)
#define DIAG_PRAGMA(x) DIAG_DO_PRAGMA(GCC diagnostic x)
#else
#define DIAG_PRAGMA(x) DIAG_DO_PRAGMA(clang diagnostic x)
#endif

// the following were previously defined in Poco/Platform_POSIX.h
#ifdef DIAG_ON
#undef DIAG_ON
#endif
#ifdef DIAG_OFF
#undef DIAG_OFF
#endif

#if defined(GCC_VERSION)
// define macros for turning the warning suppression on/off for GCC
// note that we turn off unknown warnings here as well so that we can use the
// same macro for GCC and clang.
// clang-format off
#if GCC_VERSION >= 40600 // GCC 4.6.0
#define DIAG_OFF(x)                                                          \
  DIAG_PRAGMA(push)                                                          \
  DIAG_PRAGMA(ignored DIAG_JOINSTR(-W, unknown-warning))                     \
  DIAG_PRAGMA(ignored DIAG_JOINSTR(-W, x))
#define DIAG_ON(x) DIAG_PRAGMA(pop)
#else
#define DIAG_OFF(x)                                                          \
  DIAG_PRAGMA(ignored DIAG_JOINSTR(-W, unknown-warning))                     \
  DIAG_PRAGMA(ignored DIAG_JOINSTR(-W, x))
#define DIAG_ON(x)                                                           \
  DIAG_PRAGMA(warning DIAG_JOINSTR(-W, unknown-warning))                     \
  DIAG_PRAGMA(warning DIAG_JOINSTR(-W, x))
#endif
// clang-format on

#elif defined(CLANG_VERSION) && CLANG_VERSION >= 306
// define macros for turning the warning suppression on/off for clang
// note that we turn off unknown warnings here as well so that we can use the
// same macro for GCC and clang.
// clang-format off
#define DIAG_OFF(x)                                                         \
  DIAG_PRAGMA(push)                                                         \
  DIAG_PRAGMA(ignored DIAG_JOINSTR(-W, unknown-warning-option))             \
  DIAG_PRAGMA(ignored DIAG_JOINSTR(-W, x))
#define DIAG_ON(x) DIAG_PRAGMA(pop)
// clang-format on
#else
// neither clang or GCC
#define DIAG_OFF(x)
#define DIAG_ON(x)
#endif

#endif

// Defining this macro separately since clang-tidy tries to add spaces around
// the hyphen and we use it in a lot of test files.
// clang-format off
#if defined(__cplusplus) && defined(GCC_VERSION) && GCC_VERSION >= 50000
#define DIAG_OFF_SUGGEST_OVERRIDE DIAG_OFF(suggest-override)
#define DIAG_ON_SUGGEST_OVERRIDE DIAG_ON(suggest-override)
#elif defined(__cplusplus) && defined(CLANG_VERSION) && CLANG_VERSION >= 306
#define DIAG_OFF_SUGGEST_OVERRIDE DIAG_OFF(inconsistent-missing-override)
#define DIAG_ON_SUGGEST_OVERRIDE DIAG_ON(inconsistent-missing-override)
#else
#define DIAG_OFF_SUGGEST_OVERRIDE
#define DIAG_ON_SUGGEST_OVERRIDE
#endif
// clang-format on

#if defined(GCC_VERSION) || defined(CLANG_VERSION)
#define UNUSED_FUNCTION __attribute__((unused))
#else
#define UNUSED_FUNCTION
#endif

#endif /*MANTID_KERNEL_WARNINGSUPPRESSIONS_H_*/
