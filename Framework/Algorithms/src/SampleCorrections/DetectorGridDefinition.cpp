#include "MantidAlgorithms/SampleCorrections/DetectorGridDefinition.h"

#include <cmath>

namespace Mantid {
namespace Algorithms {

DetectorGridDefinition::DetectorGridDefinition(const double minLatitude, const double maxLatitude,
                       const size_t latitudePoints, const double minLongitude,
                       const double maxLongitude, const size_t longitudePoints)
  : m_minLatitude(minLatitude), m_maxLatitude(maxLatitude), m_latitudePoints(latitudePoints),
    m_minLongitude(minLongitude), m_maxLongitude(maxLongitude), m_longitudePoints(longitudePoints) {
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
  m_latitudeStep = (maxLatitude - minLatitude) / static_cast<double>(latitudePoints - 1);
  m_longitudeStep = (maxLongitude - minLongitude) / static_cast<double>(longitudePoints - 1);
}

double DetectorGridDefinition::latitudeAt(const size_t row) const {
  return m_minLatitude + static_cast<double>(row) * m_latitudeStep;
}

double DetectorGridDefinition::longitudeAt(const size_t column) const {
  return m_minLongitude + static_cast<double>(column) * m_longitudeStep;
}

std::array<size_t, 4> DetectorGridDefinition::nearestNeighbourIndices(const double latitude, const double longitude) const {
  size_t row = static_cast<size_t>((latitude - m_minLatitude) / m_latitudeStep);
  if (row == m_latitudePoints - 1) {
    --row;
  }
  size_t col = static_cast<size_t>((longitude - m_minLongitude) / m_longitudeStep);
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

size_t DetectorGridDefinition::numberColumns() const {
  return m_longitudePoints;
}

size_t DetectorGridDefinition::numberRows() const {
  return m_latitudePoints;
}

} // namespace Algorithms
} // namespace Mantid
