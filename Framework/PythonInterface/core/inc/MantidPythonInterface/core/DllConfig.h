// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_CORE_DLLCONFIG_H_
#define MANTID_PYTHONINTERFACE_CORE_DLLCONFIG_H_
/*
  This file contains the DLLExport/DLLImport linkage configuration for the
  PythonInterfaceCore library
*/
#include "MantidKernel/System.h"

#ifdef IN_MANTID_PYTHONINTERFACE_CORE
#define MANTID_PYTHONINTERFACE_CORE_DLL DLLExport
#else
#define MANTID_PYTHONINTERFACE_CORE_DLL DLLImport
#endif // IN_MANTID_PYTHONINTERFACE_CORE

#endif // MANTID_PYTHONINTERFACE_CORE_DLLCONFIG_H_
