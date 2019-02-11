// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_COMPONENTINFOITEM_H_
#define MANTID_GEOMETRY_COMPONENTINFOITEM_H_

#include <MantidKernel/Quat.h>
#include <MantidKernel/V3D.h>
#include <vector>

namespace Mantid {
namespace Geometry {

/** ComponentInfoItem
 * Return type for ComponentInfoIterators. Provides AOS type access to
 * ComponentInfo
 */
template <typename T> class ComponentInfoItem {
public:
  ComponentInfoItem(T &componentInfo, const size_t index)
      : m_componentInfo(&componentInfo), m_index(index) {}

  bool isDetector() const { return m_componentInfo->isDetector(m_index); }
  std::vector<size_t> detectorsInSubtree() const {
    return m_componentInfo->detectorsInSubtree(m_index);
  }
  std::vector<size_t> componentsInSubtree() const {
    return m_componentInfo->componentsInSubtree(m_index);
  }
  const std::vector<size_t> &children() const {
    return m_componentInfo->children(m_index);
  }
  Kernel::V3D position() const { return m_componentInfo->position(m_index); }
  Kernel::Quat rotation() const { return m_componentInfo->rotation(m_index); }
  size_t parent() const { return m_componentInfo->parent(m_index); }
  bool hasParent() const { return m_componentInfo->hasParent(m_index); }
  Kernel::V3D scaleFactor() const {
    return m_componentInfo->scaleFactor(m_index);
  }
  std::string name() const { return m_componentInfo->name(m_index); }
  size_t index() const { return m_index; }

  // Non-owning pointer. A reference makes the class unable to define an
  // assignment operator that we need.
  T *m_componentInfo;
  size_t m_index;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTINFOITEM_H_ */
