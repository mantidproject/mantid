#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace API {

DetectorInfo::DetectorInfo(const Geometry::Instrument &instrument)
    : m_instrument(instrument), m_detectorIDs(instrument.getDetectorIDs(
                                    false /* do not skip monitors */)),
      m_detectors(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

/// Returns the position of the detector with given index.
Kernel::V3D DetectorInfo::position(const size_t index) const {
  return getDetector(index).getPos();
}

const std::vector<detid_t> DetectorInfo::detectorIDs() const {
  return m_detectorIDs;
}

const Geometry::IDetector &DetectorInfo::getDetector(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] != index) {
    m_lastIndex[thread] = index;
    m_detectors[thread] = m_instrument.getDetector(m_detectorIDs[index]);
  }

  return *m_detectors[thread];
}

void DetectorInfo::setCachedDetector(
    size_t index, boost::shared_ptr<const Geometry::IDetector> detector) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  m_lastIndex[thread] = index;
  m_detectors[thread] = detector;
}

} // namespace API
} // namespace Mantid
