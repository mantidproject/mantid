// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"

#include <cmath>

namespace Mantid {
namespace Algorithms {

/** Initializes a DetectorGridDefinition object.
 *  @param minLatitude Start of the latitude range.
 *  @param maxLatitude End of the latitude range.
 *  @param latitudePoints Number of rows.
 *  @param minLongitude Start of the longitude range.
 *  @param maxLongitude End of the longitude range.
 *  @param longitudePoints Number of columns.
 *  @throw std::runtime_error If invalid parameters are given
 */
DetectorGridDefinition::DetectorGridDefinition(const double minLatitude,
                                               const double maxLatitude,
                                               const size_t latitudePoints,
                                               const double minLongitude,
                                               const double maxLongitude,
                                               const size_t longitudePoints)
    : m_minLatitude(minLatitude), m_maxLatitude(maxLatitude),
      m_latitudePoints(latitudePoints), m_minLongitude(minLongitude),
      m_maxLongitude(maxLongitude), m_longitudePoints(longitudePoints) {
  if (latitudePoints == 0 || longitudePoints == 0 ||
      minLatitude > maxLatitude || minLongitude > maxLongitude) {
    throw std::runtime_error("Invalid detector grid definition.");
  }
  // The angular ranges might be zero in some cases preventing
  // the spawning of a real grid. We want to avoid this.
  const double tiny = 1e-5;
  const double smallShift = M_PI / 300.0;
  if (std::abs(m_minLatitude - m_maxLatitude) < tiny) {
    m_minLatitude -= smallShift;
    m_maxLatitude += smallShift;
  }
  if (std::abs(m_minLongitude - m_maxLongitude) < tiny) {
    m_minLongitude -= smallShift;
    m_maxLongitude += smallShift;
  }
  m_latitudeStep = (m_maxLatitude - m_minLatitude) /
                   static_cast<double>(m_latitudePoints - 1);
  m_longitudeStep = (m_maxLongitude - m_minLongitude) /
                    static_cast<double>(m_longitudePoints - 1);
}

/** Return the latitude of the given row.
 *  @param row Number of a row.
 *  @return A latitude.
 */
double DetectorGridDefinition::latitudeAt(const size_t row) const {
  return m_minLatitude + static_cast<double>(row) * m_latitudeStep;
}

/** Return the longitude of the given column.
 *  @param column Number of a column.
 *  @return A longitude.
 */
double DetectorGridDefinition::longitudeAt(const size_t column) const {
  return m_minLongitude + static_cast<double>(column) * m_longitudeStep;
}

/** Return the indices to detector surrounding the given point.
 *  @param latitude Latitude of a point.
 *  @param longitude Longitude of a point.
 *  @return Indices to four nearby detectors.
 */
std::array<size_t, 4>
DetectorGridDefinition::nearestNeighbourIndices(const double latitude,
                                                const double longitude) const {
  auto row = static_cast<size_t>((latitude - m_minLatitude) / m_latitudeStep);
  // Check for points at the edges or outside the grid.
  if (row == m_latitudePoints - 1) {
    --row;
  }
  auto col =
      static_cast<size_t>((longitude - m_minLongitude) / m_longitudeStep);
  if (col == m_longitudePoints - 1) {
    --col;
  }
  std::array<size_t, 4> is;
  std::get<0>(is) = col * m_latitudePoints + row;
  std::get<1>(is) = std::get<0>(is) + 1;
  std::get<2>(is) = std::get<0>(is) + m_latitudePoints;
  std::get<3>(is) = std::get<2>(is) + 1;
  return is;
}

/** Return the number of columns in the grid.
 *  @return Number of columns.
 */
size_t DetectorGridDefinition::numberColumns() const {
  return m_longitudePoints;
}

/** Return the number of rows in the grid.
 *  @return Number of rows.
 */
size_t DetectorGridDefinition::numberRows() const { return m_latitudePoints; }

} // namespace Algorithms
} // namespace Mantid
