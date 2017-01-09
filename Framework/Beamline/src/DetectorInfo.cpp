#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/make_cow.h"

namespace Mantid {
namespace Beamline {

DetectorInfo::DetectorInfo(const size_t numberOfDetectors)
    : m_isMonitor(Kernel::make_cow<std::vector<bool>>(numberOfDetectors)),
      m_isMasked(Kernel::make_cow<std::vector<bool>>(numberOfDetectors)) {}

DetectorInfo::DetectorInfo(const size_t numberOfDetectors,
                           const std::vector<size_t> &monitorIndices)
    : DetectorInfo(numberOfDetectors) {
  for (const auto i : monitorIndices)
    m_isMonitor.access()[i] = true;
}

/// Returns the size of the DetectorInfo, i.e., the number of detectors in the
/// instrument.
size_t DetectorInfo::size() const { return m_isMasked->size(); }

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  return (*m_isMonitor)[index];
}

/// Returns true if the detector is masked.
bool DetectorInfo::isMasked(const size_t index) const {
  return (*m_isMasked)[index];
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const size_t index, bool masked) {
  m_isMasked.access()[index] = masked;
}

} // namespace Beamline
} // namespace Mantid
