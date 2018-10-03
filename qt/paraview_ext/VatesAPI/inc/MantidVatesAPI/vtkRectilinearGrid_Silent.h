// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VTKRECTILINEARGRID_SILENT_H
#define VTKRECTILINEARGRID_SILENT_H

#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <vtkRectilinearGrid.h>
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
#endif

#endif // VTKRECTILINEARGRID_SILENT_H
