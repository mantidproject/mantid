#ifndef MANTID_DATAOBJECTS_FRACTIONALREBINNING_
#define MANTID_DATAOBJECTS_FRACTIONALREBINNING_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidDataObjects/DllConfig.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidDataObjects/RebinnedOutput.h"

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
FractionalRebinning helper functionality, used by the Rebin2D algorithm.

@author Harry Jeffery

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
                boost::shared_ptr<API::Progress> progress =
                    boost::shared_ptr<API::Progress>());

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
