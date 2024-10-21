// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/**
    This file contains typedefs for entities relating to multi-dimensional
    geometry.

    @author Martyn Gigg, Tessella plc
*/

#include <limits>

namespace Mantid {

/** Typedef for the data type to use for coordinate axes in MD objects such
 * as MDBox, MDEventWorkspace, etc.
 *
 * This could be a float or a double, depending on requirements.
 * We can change this in order to compare
 * performance/memory/accuracy requirements.
 */
using coord_t = float;

/// Define indicating that the coord_t type is a float (not double)
// #undef COORDT_IS_FLOAT
#define COORDT_IS_FLOAT

/** Typedef for the signal recorded in a MDBox, etc.
 * Note: MDEvents use 'float' internally to save memory
 */
using signal_t = double;

/** Macro TMDE to make declaring template functions
 * faster. Put this macro before function declarations.
 * Use:
 *
 * TMDE(void ClassName)\::methodName()
 * {
 *    // function body here
 * }
 *
 * @tparam MDE :: the MDLeanEvent/MDEvent type; at first, this will always be
 *                MDLeanEvent<nd>
 * @tparam nd :: the number of dimensions in the center coords. Passing this
 *               as a template argument should speed up some code.
 */
#define TMDE(decl) template <typename MDE, size_t nd> decl<MDE, nd>

/** Macro to make declaring template classes faster.
 * Use:
 * TMDE_CLASS
 * class ClassName : ...
 */
#define TMDE_CLASS template <typename MDE, size_t nd>

#define UNDEF_SIZET std::numeric_limits<size_t>::max()
#define UNDEF_COORDT std::numeric_limits<coord_t>::quiet_NaN()
#define UNDEF_UINT64 std::numeric_limits<uint64_t>::max()
} // namespace Mantid
