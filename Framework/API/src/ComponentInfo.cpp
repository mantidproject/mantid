#include "MantidAPI/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidBeamline/ComponentInfo.h"
#include <boost/make_shared.hpp>
#include <exception>
#include <string>
#include <boost/make_shared.hpp>

namespace Mantid {
namespace API {

/**
 * Constructor
 * @param componentInfo : Beamline wrapped ComponentInfo
 * @param componentIds : Component Ids ordered by component index
 */
ComponentInfo::ComponentInfo(const Beamline::ComponentInfo &componentInfo,
                             std::vector<Geometry::ComponentID> &&componentIds)
    : m_componentInfo(componentInfo),
      m_componentIds(
          boost::make_shared<std::vector<Geometry::ComponentID>>(componentIds)),
      m_compIDToIndex(boost::make_shared<
          std::unordered_map<Geometry::IComponent *, size_t>>()) {

  if (m_componentInfo.size() != m_componentIds->size()) {
    throw std::invalid_argument("Mismatch between size of ComponentInfo and "
                                "number of ComponentIDs provided");
  }
  for (size_t i = 0; i < m_componentInfo.size(); ++i) {
    (*m_compIDToIndex)[(*m_componentIds)[i]] = i;
  }
}

std::vector<size_t>
ComponentInfo::detectorIndices(size_t componentIndices) const {
  return m_componentInfo.detectorIndices(componentIndices);
}

size_t ComponentInfo::size() const { return m_componentInfo.size(); }

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  return m_compIDToIndex->at(id);
}

} // namespace API
} // namespace Mantid
