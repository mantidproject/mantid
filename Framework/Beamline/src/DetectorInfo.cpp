#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_cow.h"

namespace Mantid {
namespace Beamline {

DetectorInfo::DetectorInfo(const size_t numberOfDetectors)
    : m_isMasked(Kernel::make_cow<std::vector<bool>>(numberOfDetectors)) {}

/// Returns the size of the DetectorInfo, i.e., the number of detectors in the
/// instrument.
size_t DetectorInfo::size() const { return m_isMasked->size(); }

/// Returns true if the detector is a masked.
bool DetectorInfo::isMasked(const size_t index) const {
  return (*m_isMasked)[index];
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const size_t index, bool masked) {
  m_isMasked.access()[index] = masked;
}

} // namespace Beamline
} // namespace Mantid
