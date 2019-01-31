// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_DETECTORINFOITEM_H_
#define MANTID_GEOMETRY_DETECTORINFOITEM_H_

#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

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
*/
template <typename T> class DetectorInfoItem {

public:
  // Methods that can be accessed via the iterator
  bool isMonitor() const { return m_detectorInfo->isMonitor(m_index); }

  bool isMasked() const { return m_detectorInfo->isMasked(m_index); }

  void setMasked(bool masked) {
    static_assert(!std::is_const<T>::value, "Operation disabled on const T");
    return m_detectorInfo->setMasked(m_index, masked);
  }

  double twoTheta() const { return m_detectorInfo->twoTheta(m_index); }

  Mantid::Kernel::V3D position() const {
    return m_detectorInfo->position(m_index);
  }

  Mantid::Kernel::Quat rotation() const {
    return m_detectorInfo->rotation(m_index);
  }

  double l2() const { return m_detectorInfo->l2(m_index); }

  size_t index() const { return m_index; }

  DetectorInfoItem(T &detectorInfo, const size_t index)
      : m_detectorInfo(&detectorInfo), m_index(index) {}

  // Non-owning pointer. A reference makes the class unable to define an
  // assignment operator that we need.
  T *m_detectorInfo;
  size_t m_index;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFOITEM_H_ */
