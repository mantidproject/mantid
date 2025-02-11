// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/*******************************************************************************
 *                      READ THIS!! (AND THEN READ IT AGAIN)
 *
 *
 * PLEASE DON'T EDIT THIS FILE UNLESS ADDING SOMETHING THAT ABSOLUTELY
 * MUST BE SEEN BY **EVERY** FILE IN MANTID
 *
 *
 *******************************************************************************/

/*  A system-wide file to contain, e.g., useful system-dependent macros
 *
 *  @author Russell Taylor, Tessella Support Services plc
 */

/**
 * Definitions of the DLLImport compiler directives for MSVC
 */
#ifdef _WIN32
// Export/Import declarations
#define DLLExport __declspec(dllexport)
#define DLLImport __declspec(dllimport)
#define EXTERN_IMPORT extern
#elif defined(__GNUC__) && !defined(__clang__)
#define DLLExport __attribute__((visibility("default")))
#define DLLImport
#define EXTERN_IMPORT extern
#else
#define DLLExport
#define DLLImport
#define EXTERN_IMPORT
#endif

/**
 * Function arguments are sometimes unused in certain implmentations
 * but are required for documentation purposes.
 * This is a macro to silence compiler warnings about the subject
 */
#ifndef UNUSED_ARG
#define UNUSED_ARG(x) (void)x;
#endif

/**
 * A Macro to mark a function as deprecated.
 */
#if (defined(__GNUC__) || defined(__clang__))
#define DEPRECATED(func) func __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif

/**
 *  For size_t and ptrdiff_t
 */
#include <cstddef>
#include <cstdint>
