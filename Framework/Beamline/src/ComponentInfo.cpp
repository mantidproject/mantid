#include "MantidBeamline/ComponentInfo.h"

namespace Mantid {
namespace Beamline {

ComponentInfo::ComponentInfo(std::vector<std::vector<size_t>> &detectorIndexes)
    : m_detectorIndexes(detectorIndexes) {}

std::vector<size_t>
ComponentInfo::detectorIndexes(size_t componentIndex) const {
  return m_detectorIndexes[componentIndex];
}

size_t ComponentInfo::size() const { return m_detectorIndexes.size(); }

} // namespace Beamline
} // namespace Mantid
