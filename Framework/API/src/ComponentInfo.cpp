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
 * Constructor.
 * @param componentInfo : Internal Beamline ComponentInfo
 * @param componentIds : ComponentIDs ordered by component
 * @param componentIdToIndexMap : ID -> index translation map
 * index
 */
ComponentInfo::ComponentInfo(
    const Mantid::Beamline::ComponentInfo &componentInfo,
    boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
        componentIds,
    boost::shared_ptr<const std::unordered_map<Geometry::IComponent *, size_t>>
        componentIdToIndexMap)
    : m_componentInfo(componentInfo), m_componentIds(std::move(componentIds)),
      m_compIDToIndex(std::move(componentIdToIndexMap)) {

  if (m_componentIds->size() != m_compIDToIndex->size()) {
    throw std::invalid_argument(
        "Inconsistent ID and Mapping input containers for API::ComponentInfo");
  }
  if (m_componentIds->size() != m_componentInfo.size()) {
    throw std::invalid_argument("Inconsistent ID and base "
                                "Beamline::ComponentInfo sizes for "
                                "API::ComponentInfo");
  }
}

std::vector<size_t>
ComponentInfo::detectorsInSubtree(size_t componentIndex) const {
  return m_componentInfo.detectorsInSubtree(componentIndex);
}

std::vector<size_t>
ComponentInfo::componentsInSubtree(size_t componentIndex) const {
  return m_componentInfo.componentsInSubtree(componentIndex);
}

size_t ComponentInfo::size() const { return m_componentInfo.size(); }

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  return m_compIDToIndex->at(id);
}

Kernel::V3D ComponentInfo::position(const size_t componentIndex) const {
  return Kernel::toV3D(m_componentInfo.position(componentIndex));
}

Kernel::Quat ComponentInfo::rotation(const size_t componentIndex) const {
  return Kernel::toQuat(m_componentInfo.rotation(componentIndex));
}
} // namespace API
} // namespace Mantid
