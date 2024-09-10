// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <array>
#include <optional>
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
*/

MANTID_ALGORITHMS_DLL std::tuple<double, double, double, double> extremeAngles(const API::MatrixWorkspace &ws);
MANTID_ALGORITHMS_DLL std::pair<double, double> geographicalAngles(const Kernel::V3D &p,
                                                                   const Geometry::ReferenceFrame &refFrame);
MANTID_ALGORITHMS_DLL std::tuple<double, double> extremeWavelengths(const API::MatrixWorkspace &ws);
MANTID_ALGORITHMS_DLL HistogramData::Histogram modelHistogram(const API::MatrixWorkspace &modelWS,
                                                              const size_t wavelengthPoints);
MANTID_ALGORITHMS_DLL API::MatrixWorkspace_uptr createSparseWS(const API::MatrixWorkspace &modelWS,
                                                               const Algorithms::DetectorGridDefinition &grid,
                                                               const size_t wavelengthPoints);
MANTID_ALGORITHMS_DLL double greatCircleDistance(const double lat1, const double long1, const double lat2,
                                                 const double long2);
MANTID_ALGORITHMS_DLL std::array<double, 4> inverseDistanceWeights(const std::array<double, 4> &distances);
MANTID_ALGORITHMS_DLL HistogramData::Histogram interpolateFromDetectorGrid(const double lat, const double lon,
                                                                           const API::MatrixWorkspace &ws,
                                                                           const std::array<size_t, 4> &indices);
MANTID_ALGORITHMS_DLL HistogramData::Histogram
bilinearInterpolateFromDetectorGrid(const double lat, const double lon, const API::MatrixWorkspace &ws,
                                    const std::vector<std::vector<std::optional<size_t>>> &indices);
MANTID_ALGORITHMS_DLL std::unique_ptr<const Algorithms::DetectorGridDefinition>
createDetectorGridDefinition(const API::MatrixWorkspace &modelWS, const size_t rows, const size_t columns);
} // namespace SparseInstrument
} // namespace Algorithms
} // namespace Mantid
