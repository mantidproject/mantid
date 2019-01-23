// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**

  This file includes the muParser.h header file and avoids a conflict with the
  DLLExport macro
  that we both have defined.
*/
#ifdef _WIN32
#ifdef DLLExport
#undef DLLExport // Avoid warning about redefinition
#endif
#include <muParser.h>
#undef DLLExport
#define DLLExport __declspec(dllexport) // Our version.
#else
#include <muParser.h>
#endif
