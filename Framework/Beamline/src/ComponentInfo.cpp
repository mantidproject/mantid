#include "MantidBeamline/ComponentInfo.h"

namespace Mantid {
namespace Beamline {

ComponentInfo::ComponentInfo(std::vector<std::vector<size_t>> &detectorIndexes)
    : m_detectorIndexes(detectorIndexes) {}

size_t ComponentInfo::size() const { return m_detectorIndexes.size(); }

} // namespace Beamline
} // namespace Mantid
