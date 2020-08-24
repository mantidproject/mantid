// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"

#ifdef IN_MANTID_VATES_SIMPLEGUI_QTWIDGETS
#define EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS DLLExport
#else
#define EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS DLLImport
#endif // IN_MANTID_VATES_SIMPLEGUI_QTWIDGETS
