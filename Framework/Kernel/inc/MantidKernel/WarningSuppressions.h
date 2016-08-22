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
// Currently this is only defined for gcc
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
// how to use a pragma in a macro
#define PRAGMA(x) _Pragma(#x)

// convenience for getting gcc version
#define GCC_VERSION                                                            \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// things to make the macros clearer
#define GCC_DIAG_STR(s) #s
// undefine definition from Poco 1.6
#ifdef GCC_DIAG_JOINSTR
#undef GCC_DIAG_JOINSTR
#endif
#define GCC_DIAG_JOINSTR(x, y) GCC_DIAG_STR(x##y)
// undefine definition from Poco 1.6
#ifdef GCC_DIAG_DO_PRAGMA
#undef GCC_DIAG_DO_PRAGMA
#endif
#define GCC_DIAG_DO_PRAGMA(x) _Pragma(#x)
#define GCC_DIAG_PRAGMA(x) GCC_DIAG_DO_PRAGMA(GCC diagnostic x)

// the following were previously defined in Poco/Platform_POSIX.h
#ifdef GCC_DIAG_ON
#undef GCC_DIAG_ON
#endif
#ifdef GCC_DIAG_OFF
#undef GCC_DIAG_OFF
#endif
// define macros for turning the warning suppression on/off
#if GCC_VERSION >= 40600 // 4.6.0
#define GCC_DIAG_OFF(x)                                                        \
  GCC_DIAG_PRAGMA(push)                                                        \
  GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W, x))
#define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(pop)
#else
#define GCC_DIAG_OFF(x) GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W, x))
#define GCC_DIAG_ON(x) GCC_DIAG_PRAGMA(warning GCC_DIAG_JOINSTR(-W, x))
#endif

#else // anything else - does nothing
#define GCC_DIAG_OFF(x)
#define GCC_DIAG_ON(x)
#endif

// Defining this macro separately since clang-tidy tries to add spaces around
// the hyphen and we use it in a lot of test files.
// clang-format off
#if defined(GCC_VERSION) && GCC_VERSION >= 50000
#define GCC_DIAG_OFF_SUGGEST_OVERRIDE GCC_DIAG_OFF(suggest-override)
#define GCC_DIAG_ON_SUGGEST_OVERRIDE GCC_DIAG_ON(suggest-override)
#else
#define GCC_DIAG_OFF_SUGGEST_OVERRIDE
#define GCC_DIAG_ON_SUGGEST_OVERRIDE
#endif
// clang-format on

#ifdef GCC_VERSION
#define GCC_UNUSED_FUNCTION __attribute__((unused))
#else
#define GCC_UNUSED_FUNCTION
#endif

#endif /*MANTID_KERNEL_WARNINGSUPPRESSIONS_H_*/
