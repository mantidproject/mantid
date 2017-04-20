#include "MantidAPI/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidBeamline/ComponentInfo.h"
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
    const Mantid::Beamline::ComponentInfo &componentInfo,
    boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
        componentIds)
    : m_componentInfo(componentInfo), m_componentIds(std::move(componentIds)),
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

ComponentInfo::ComponentInfo(
    const Mantid::Beamline::ComponentInfo &componentInfo,
    boost::shared_ptr<const std::vector<Mantid::Geometry::IComponent *>>
        componentIds,
    boost::shared_ptr<std::unordered_map<Geometry::IComponent *, size_t>>
        compIdToIndexMap)
    : m_componentInfo(componentInfo), m_componentIds(std::move(componentIds)),
      m_compIDToIndex(compIdToIndexMap) {}

std::vector<size_t>
ComponentInfo::detectorIndices(size_t componentIndex) const {
  return m_componentInfo.detectorIndices(componentIndex);
}

const std::vector<Geometry::IComponent *> &ComponentInfo::componentIds() const {
  return *m_componentIds;
}

size_t ComponentInfo::size() const { return m_componentInfo.size(); }

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  return m_compIDToIndex->at(id);
}

boost::shared_ptr<std::unordered_map<Geometry::IComponent *, size_t>>
ComponentInfo::compIdToIndexMapping() const {
  return m_compIDToIndex;
}

bool ComponentInfo::operator==(const ComponentInfo &other) const {
  return this->m_componentInfo == other.m_componentInfo &&
         m_compIDToIndex == other.m_compIDToIndex;
}

bool ComponentInfo::operator!=(const ComponentInfo &other) const {
  return !this->operator==(other);
}

} // namespace API
} // namespace Mantid
