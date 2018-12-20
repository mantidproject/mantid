#ifndef MANTID_ALGORITHMS_SPARSEINSTRUMENT_H_
#define MANTID_ALGORITHMS_SPARSEINSTRUMENT_H_

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include <array>
#include <tuple>
#include <utility>

namespace Mantid {
namespace Algorithms {
class DetectorGridDefinition;
}
namespace Geometry {
class ReferenceFrame;
}
namespace HistogramData {
class Histogram;
}
namespace Kernel {
class V3D;
}
namespace Algorithms {
namespace SparseInstrument {
/**
  Defines functions and utilities to create and deal with sparse instruments.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

DLLExport std::tuple<double, double, double, double>
extremeAngles(const API::MatrixWorkspace &ws);
DLLExport std::pair<double, double>
geographicalAngles(const Kernel::V3D &p,
                   const Geometry::ReferenceFrame &refFrame);
DLLExport std::tuple<double, double>
extremeWavelengths(const API::MatrixWorkspace &ws);
DLLExport HistogramData::Histogram
modelHistogram(const API::MatrixWorkspace &modelWS,
               const size_t wavelengthPoints);
DLLExport API::MatrixWorkspace_uptr
createSparseWS(const API::MatrixWorkspace &modelWS,
               const Algorithms::DetectorGridDefinition &grid,
               const size_t wavelengthPoints);
DLLExport double greatCircleDistance(const double lat1, const double long1,
                                     const double lat2, const double long2);
DLLExport std::array<double, 4>
inverseDistanceWeights(const std::array<double, 4> &distances);
DLLExport HistogramData::Histogram
interpolateFromDetectorGrid(const double lat, const double lon,
                            const API::MatrixWorkspace &ws,
                            const std::array<size_t, 4> &indices);
DLLExport std::unique_ptr<const Algorithms::DetectorGridDefinition>
createDetectorGridDefinition(const API::MatrixWorkspace &modelWS,
                             const size_t rows, const size_t columns);
} // namespace SparseInstrument
} // namespace Algorithms
} // namespace Mantid

#endif // MANTID_ALGORITHMS_SPARSEWORKSPACECREATION_H_
