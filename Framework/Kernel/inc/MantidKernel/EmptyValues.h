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

#include <cfloat>
#include <climits>

namespace Mantid {

/**
 * Returns what we consider an "empty" integer within a property
 * @returns An flag value
 */
constexpr int EMPTY_INT() { return INT_MAX; }

/**
 * Returns what we consider an "empty" long within a property
 * @returns An flag value
 */
constexpr long EMPTY_LONG() { return LONG_MAX; }

/**
 * Returns what we consider an "empty" int64_t within a property
 * @returns An flag value
 */
constexpr int64_t EMPTY_INT64() { return INT64_MAX; }

/**
 * Returns what we consider an "empty" double within a property
 * @returns An flag value
 */
constexpr double EMPTY_DBL() { return DBL_MAX / 2; }

} // namespace Mantid

#endif // MANTID_KERNEL_EMPTYVALUES_H_
