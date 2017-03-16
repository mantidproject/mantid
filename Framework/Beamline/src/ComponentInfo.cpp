#include "MantidBeamline/ComponentInfo.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Beamline {

ComponentInfo::ComponentInfo()
    : m_detectorIndices(
          boost::make_shared<std::vector<std::vector<size_t>>>(0)) {}

ComponentInfo::ComponentInfo(
    const std::vector<std::vector<size_t>> &detectorIndexes)
    : m_detectorIndices(boost::make_shared<std::vector<std::vector<size_t>>>(
          detectorIndexes)) {}

std::vector<size_t>
ComponentInfo::detectorIndices(size_t componentIndex) const {
  return (*m_detectorIndices)[componentIndex];
}

size_t ComponentInfo::size() const { return m_detectorIndices->size(); }

} // namespace Beamline
} // namespace Mantid
