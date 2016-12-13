#include "MantidBeamline/DetectorInfo.h"

namespace Mantid {
namespace Beamline {

DetectorInfo::DetectorInfo(const size_t numberOfDetectors)
    : m_size(numberOfDetectors) {}

size_t DetectorInfo::size() const { return m_size; }

} // namespace Beamline
} // namespace Mantid
