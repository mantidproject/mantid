// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MPLCPP_DLLCONFIG_H_
#define MANTIDQT_MPLCPP_DLLCONFIG_H_

#include "MantidKernel/System.h"

#ifdef IN_MANTIDQT_MPLCPP
#define MANTID_MPLCPP_DLL DLLExport
#else
#define MANTID_MPLCPP_DLL DLLImport
#endif /* IN_MANTIDQT_MPLCPP */

#endif // MANTIDQT_MPLCPP_DLLCONFIG_H_
