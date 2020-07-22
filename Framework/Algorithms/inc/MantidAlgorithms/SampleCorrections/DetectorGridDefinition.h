// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"

#include <array>
#include <cstddef>

namespace Mantid {
namespace Algorithms {

/** DetectorGridDefinition is a helper class for building the sparse
  instrument in MonteCarloAbsorption.
*/
class MANTID_ALGORITHMS_DLL DetectorGridDefinition {
public:
  DetectorGridDefinition(const double minLatitude, const double maxLatitude,
                         const size_t latitudePoints, const double minLongitude,
                         const double maxLongitude, const size_t longitudeStep);

  double latitudeAt(const size_t row) const;
  double longitudeAt(const size_t column) const;
  std::array<size_t, 4> nearestNeighbourIndices(const double latitude,
                                                const double longitude) const;
  size_t numberColumns() const;
  size_t numberRows() const;

private:
  double m_minLatitude;
  double m_maxLatitude;
  size_t m_latitudePoints;
  double m_latitudeStep;
  double m_minLongitude;
  double m_maxLongitude;
  size_t m_longitudePoints;
  double m_longitudeStep;
};

} // namespace Algorithms
} // namespace Mantid
