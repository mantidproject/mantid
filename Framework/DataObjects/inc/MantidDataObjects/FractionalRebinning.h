// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_FRACTIONALREBINNING_
#define MANTID_DATAOBJECTS_FRACTIONALREBINNING_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidGeometry/Math/Quadrilateral.h"

namespace Mantid {
//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------
namespace API {
class Progress;
}
namespace Geometry {
class ConvexPolygon;
}

namespace DataObjects {

/**
 * FractionalRebinning helper functionality, used by the
 * Rebin2D algorithm.
 */

namespace FractionalRebinning {

/// Find the intersect region on the output grid
MANTID_DATAOBJECTS_DLL bool
getIntersectionRegion(const std::vector<double> &xAxis,
                      const std::vector<double> &verticalAxis,
                      const Geometry::Quadrilateral &inputQ, size_t &qstart,
                      size_t &qend, size_t &x_start, size_t &x_end);

/// Compute sqrt of errors and put back in bin width division if necessary
MANTID_DATAOBJECTS_DLL void
normaliseOutput(API::MatrixWorkspace_sptr outputWS,
                API::MatrixWorkspace_const_sptr inputWS,
                API::Progress *progress = nullptr);

/// Rebin the input quadrilateral to to output grid
MANTID_DATAOBJECTS_DLL void
rebinToOutput(const Geometry::Quadrilateral &inputQ,
              const API::MatrixWorkspace_const_sptr &inputWS, const size_t i,
              const size_t j, API::MatrixWorkspace &outputWS,
              const std::vector<double> &verticalAxis);

/// Rebin the input quadrilateral to to output grid
MANTID_DATAOBJECTS_DLL void rebinToFractionalOutput(
    const Geometry::Quadrilateral &inputQ,
    const API::MatrixWorkspace_const_sptr &inputWS, const size_t i,
    const size_t j, DataObjects::RebinnedOutput &outputWS,
    const std::vector<double> &verticalAxis,
    const DataObjects::RebinnedOutput_const_sptr &inputRB = nullptr);

} // namespace FractionalRebinning

} // namespace DataObjects
} // namespace Mantid

#endif
