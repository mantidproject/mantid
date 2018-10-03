// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_BEAMLINE_DLLCONFIG_H_
#define MANTID_BEAMLINE_DLLCONFIG_H_

/*
  This file contains the DLLExport/DLLImport linkage configuration for the
  Beamline library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_BEAMLINE
#define MANTID_BEAMLINE_DLL DLLExport
#else
#define MANTID_BEAMLINE_DLL DLLImport
#endif // IN_MANTID_BEAMLINE

#endif // MANTID_BEAMLINE_DLLCONFIG_H_
