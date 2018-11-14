// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_EMPTYVALUES_H_
#define MANTID_KERNEL_EMPTYVALUES_H_

/**
    This file contains functions to define empty values, i.e EMPTY_INT();

    @author Martyn Gigg, Tessella plc
*/
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/System.h"

namespace Mantid {

/// Returns what we consider an "empty" integer
DLLExport int EMPTY_INT();

/// Returns what we consider an "empty" long
DLLExport long EMPTY_LONG();

/// Returns what we consider an "empty" int64_t
DLLExport int64_t EMPTY_INT64();

/// Return what we consider to be an empty double
DLLExport double EMPTY_DBL();
} // namespace Mantid

#endif // MANTID_KERNEL_EMPTYVALUES_H_
