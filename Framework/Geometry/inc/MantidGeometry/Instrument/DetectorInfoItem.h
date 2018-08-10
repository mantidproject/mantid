#ifndef MANTID_GEOMETRY_DETECTORINFOITEM_H_
#define MANTID_GEOMETRY_DETECTORINFOITEM_H_

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

using Mantid::Geometry::DetectorInfo;
using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;

namespace Mantid {
namespace Geometry {

/** DetectorInfoItem

DetectorInfoItem is only created by DetectorInfoIterator and allows users of
the DetectorInfoIterator object access to data from DetectorInfo. The available
methods include:
  - isMonitor()
  - isMaksed()
  - twoTheta()
  - position()
  - rotation()

@author Bhuvan Bezawada, STFC
@date 2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_GEOMETRY_DLL DetectorInfoItem {

public:
  // Provide copy and move constructors
  DetectorInfoItem(const DetectorInfoItem &other) = default;
  DetectorInfoItem &operator=(const DetectorInfoItem &rhs) = default;
  DetectorInfoItem(DetectorInfoItem &&other) = default;
  DetectorInfoItem &operator=(DetectorInfoItem &&rhs) = default;

  bool isMonitor() const { return m_detectorInfo->isMonitor(m_index); }

  bool isMasked() const { return m_detectorInfo->isMasked(m_index); }

  double twoTheta() const { return m_detectorInfo->twoTheta(m_index); }

  Mantid::Kernel::V3D position() const {
    return m_detectorInfo->position(m_index);
  }

  Mantid::Kernel::Quat rotation() const {
    return m_detectorInfo->rotation(m_index);
  }

  void advance(int64_t delta) {
    m_index = delta < 0 ? std::max(static_cast<uint64_t>(0),
                                   static_cast<uint64_t>(m_index) + delta)
                        : std::min(m_detectorInfo->size(),
                                   m_index + static_cast<size_t>(delta));
  }

  // This could cause a segmentation fault if a user goes past the end of the
  // iterator and tries to index into the n+1 th element (which would not
  // exist). Adding range checks to all the above methods may slow down
  // performance though.
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

  // Non-owning pointer. A reference makes the class unable to define an
  // assignment operator that we need.
  const DetectorInfo *m_detectorInfo;
  size_t m_index;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFOITEM_H_ */
