// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_DLLCONFIG_H_
#define MANTID_HISTOGRAMDATA_DLLCONFIG_H_

/*
  This file contains the DLLExport/DLLImport linkage configuration for the
  Histogram library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_HISTOGRAMDATA
#define MANTID_HISTOGRAMDATA_DLL DLLExport
#else
#define MANTID_HISTOGRAMDATA_DLL DLLImport
#endif // IN_MANTID_HISTOGRAMDATA

#endif // MANTID_HISTOGRAMDATA_DLLCONFIG_H_
