#ifndef MANTID_GEOMETRY_DETECTORINFOITEM_H_
#define MANTID_GEOMETRY_DETECTORINFOITEM_H_

#include <utility>

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"

using Mantid::Kernel::V3D;

namespace Mantid {
namespace Geometry {

class MANTID_GEOMETRY_DLL DetectorInfoItem {

public:
  DetectorInfoItem(const DetectorInfoItem &copy)
      : m_detectorInfo(copy.m_detectorInfo), m_index(copy.m_index) {}

  DetectorInfoItem &operator=(const DetectorInfoItem &rhs) {
    this->m_index = rhs.m_index;
    return *this;
  }

  size_t position() const { return m_index; }

  void advance(int64_t delta) {
    if (delta < 0) {
      m_index = std::max(static_cast<uint64_t>(0),
                         static_cast<uint64_t>(m_index) + delta);
    } else {
      m_index =
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

private:
  friend class DetectorInfoIterator;

  // Private constructor, can only be created by DetectorInfoIterator
  DetectorInfoItem(const DetectorInfo &detectorInfo, const size_t index)
      : m_detectorInfo(detectorInfo), m_index(index) {}

  // DetectorInfoItem& operator=(const DetectorInfoItem &) = delete;

  // Variables to hold data
  const DetectorInfo &m_detectorInfo;
  size_t m_index = 0;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFOITEM_H_ */
