#ifndef MANTID_GEOMETRY_DETECTORINFOITEM_H_
#define MANTID_GEOMETRY_DETECTORINFOITEM_H_

#include <utility>

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"

using Mantid::Kernel::V3D;
using Mantid::Geometry::DetectorInfo;

namespace Mantid {
namespace Geometry {

class MANTID_GEOMETRY_DLL DetectorInfoItem {

public:
  DetectorInfoItem(const DetectorInfoItem &other) = default;
  DetectorInfoItem &operator=(const DetectorInfoItem &rhs) = default;
  DetectorInfoItem(DetectorInfoItem &&other) = default;
  DetectorInfoItem &operator=(DetectorInfoItem &&rhs) = default;

  Mantid::Kernel::V3D position() const {
    return m_detectorInfo->position(m_index); 
  }

  void advance(int64_t delta) {
    m_index = delta < 0 ? std::max(static_cast<uint64_t>(0),
                                   static_cast<uint64_t>(m_index) + delta)
                        : std::min(m_detectorInfo->size(),
                                   m_index + static_cast<size_t>(delta));
  }

  void incrementIndex() {
    if (m_index < m_detectorInfo->size()) {
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
      : m_detectorInfo(&detectorInfo), m_index(index) {}

  // Non-owning pointer. A reference makes the class unable to define assignment
  // operator that we need.
  const DetectorInfo *m_detectorInfo;
  size_t m_index;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFOITEM_H_ */
