#ifndef MANTID_GEOMETRY_DETECTORINFOITEM_H_
#define MANTID_GEOMETRY_DETECTORINFOITEM_H_

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

#include <utility>

namespace Mantid {
namespace Geometry {
namespace Iterators {

class DetectorInfoItem {

private:
  friend class DetectorInfoIterator;

  // Variables to hold data
  const DetectorInfo &m_detectorInfo;
  size_t m_index;

  // Private constructor, can only be created by DetectorInfoIterator
  DetectorInfoItem(const DetectorInfo &detectorInfo, const size_t index)
      : m_detectorInfo(detectorInfo), m_index(index) {}

public:
  void advance(int64_t delta) {
    if (delta < 0) {
      std::max(static_cast<uint64_t>(0),
               static_cast<uint64_t>(m_index) + delta);
    } else {
      std::min(m_detectorInfo.size(), m_index + static_cast<size_t>(delta));
    }
  }

  void incrementIndex() {
    if (m_index < m_detectorInfo.size()) {
      ++m_index;
    }
  }

  void decrementIndex() {
    if (m_index > 0) {
      --m_index;
    }
  }

  size_t getIndex() const { return m_index; }

  void setIndex(const size_t index) { m_index = index; }
};

} // namespace Iterators
} // namespace Geometry
} // namespace Mantid

#endif
