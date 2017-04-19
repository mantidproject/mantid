#include "MantidAPI/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidBeamline/ComponentInfo.h"
#include <boost/make_shared.hpp>
#include <mutex>

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
          std::unordered_map<Geometry::IComponent *, size_t>>()),
      m_initIndexMapping(false) {
  /*
   * Ideally we would check here that componentIds.size() ==
   * m_componentInfo.size().
   * Currently that check would break too much in Mantid.
   */
}

std::vector<size_t>
ComponentInfo::detectorIndices(size_t componentIndex) const {
  return m_componentInfo.detectorIndices(componentIndex);
}

const std::vector<Geometry::IComponent *> &ComponentInfo::componentIds() const {
  return *m_componentIds;
}

size_t ComponentInfo::size() const { return m_componentInfo.size(); }

size_t ComponentInfo::indexOf(Geometry::IComponent *id) const {
  if (!m_initIndexMapping.load()) {
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto nComponents = m_componentInfo.size();
    m_compIDToIndex->reserve(nComponents);
    for (size_t i = 0; i < nComponents; ++i) {
      (*m_compIDToIndex)[(*m_componentIds)[i]] = i;
    }
    m_initIndexMapping.store(true);
  }
  return m_compIDToIndex->at(id);
}

} // namespace API
} // namespace Mantid
