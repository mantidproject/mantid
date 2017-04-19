#include "MantidAPI/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include <boost/make_shared.hpp>
#include <exception>
#include <string>
#include <boost/make_shared.hpp>
#include <sstream>

namespace Mantid {
namespace API {

/**
 * Constructor
 * @brief ComponentInfo::ComponentInfo
 * @param componentInfo
 * @param componentIds : ComponentIDs ordered by component
 * index
 */
ComponentInfo::ComponentInfo(
    Mantid::Beamline::ComponentInfo &componentInfo,
    std::vector<Mantid::Geometry::IComponent *> componentIds)
    : m_componentInfo(componentInfo),
      m_componentIds(boost::make_shared<std::vector<Geometry::ComponentID>>(
          std::move(componentIds))),
      m_compIDToIndex(boost::make_shared<
          std::unordered_map<Geometry::IComponent *, size_t>>()) {
  /*
   * Ideally we would check here that componentIds.size() ==
   * m_componentInfo.size().
   * Currently that check would break too much in Mantid.
   */

  for (size_t i = 0; i < m_componentInfo.size(); ++i) {
    (*m_compIDToIndex)[(*m_componentIds)[i]] = i;
  }
}

std::vector<size_t>
ComponentInfo::detectorIndices(size_t componentIndex) const {
  return m_componentInfo.detectorIndices(componentIndex);
}

std::vector<Geometry::IComponent *> ComponentInfo::componentIds() const {
  return *m_componentIds;
}

size_t ComponentInfo::size() const { return m_componentInfo.size(); }

Kernel::V3D ComponentInfo::position(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo.position(componentIndex));
}

Kernel::Quat ComponentInfo::rotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo.rotation(componentIndex));
}

void ComponentInfo::setPosition(const size_t componentIndex,
                                const Kernel::V3D &position) {
  m_componentInfo.setPosition(componentIndex, Kernel::toVector3d(position));
}

void ComponentInfo::setRotation(const size_t componentIndex,
                                const Kernel::Quat &rotation) {
  m_componentInfo.setRotation(componentIndex, Kernel::toQuaterniond(rotation));
}

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  return m_compIDToIndex->at(id);
}

} // namespace API
} // namespace Mantid
