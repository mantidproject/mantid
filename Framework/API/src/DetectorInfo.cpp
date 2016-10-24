#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace API {

DetectorInfo::DetectorInfo(const Geometry::Instrument &instrument)
    : m_instrument(instrument), m_detectorIDs(instrument.getDetectorIDs(
                                    false /* do not skip monitors */)),
      m_detectors(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

/// Returns true if the detector associated with the detector is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  return getDetector(index).isMonitor();
}

/// Returns true if the detector associated with the detector is a masked.
bool DetectorInfo::isMasked(const size_t index) const {
  return getDetector(index).isMasked();
}

/** Returns L2 (distance from sample to spectrum).
 *
 * For monitors this is defined such that L1+L2 = source-detector distance,
 * i.e., for a monitor in the beamline between source and sample L2 is negative.
 */
double DetectorInfo::l2(const size_t index) const {
  if (!isMonitor(index))
    return getDetector(index).getDistance(getSample());
  else
    return getDetector(index).getDistance(getSource()) - l1();
}

/// Returns 2 theta (scattering angle w.r.t. to beam direction).
double DetectorInfo::twoTheta(const size_t index) const {
  const Kernel::V3D samplePos = samplePosition();
  const Kernel::V3D beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  return getDetector(index).getTwoTheta(samplePos, beamLine);
}

/// Returns signed 2 theta (signed scattering angle w.r.t. to beam direction).
double DetectorInfo::signedTwoTheta(const size_t index) const {
  const Kernel::V3D samplePos = samplePosition();
  const Kernel::V3D beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const Kernel::V3D &instrumentUpAxis =
      m_instrument.getReferenceFrame()->vecPointingUp();
  return getDetector(index)
      .getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
}

/// Returns the position of the detector with given index.
Kernel::V3D DetectorInfo::position(const size_t index) const {
  return getDetector(index).getPos();
}

/// Returns the source position.
Kernel::V3D DetectorInfo::sourcePosition() const {
  std::call_once(m_sourceCached, &DetectorInfo::cacheSource, this);
  return m_sourcePos;
}

/// Returns the sample position.
Kernel::V3D DetectorInfo::samplePosition() const {
  std::call_once(m_sampleCached, &DetectorInfo::cacheSample, this);
  return m_samplePos;
}

/// Returns L1 (distance from source to sample).
double DetectorInfo::l1() const {
  std::call_once(m_L1Cached, &DetectorInfo::cacheL1, this);
  return m_L1;
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

/// Returns a reference to the source component. The value is cached, so calling
/// it repeatedly is cheap.
const Geometry::IComponent &DetectorInfo::getSource() const {
  std::call_once(m_sourceCached, &DetectorInfo::cacheSource, this);
  return *m_source;
}

/// Returns a reference to the sample component. The value is cached, so calling
/// it repeatedly is cheap.
const Geometry::IComponent &DetectorInfo::getSample() const {
  std::call_once(m_sampleCached, &DetectorInfo::cacheSample, this);
  return *m_sample;
}

void DetectorInfo::cacheSource() const {
  m_source = m_instrument.getSource();
  if (!m_source)
    throw std::runtime_error("Instrument does not contain source!");
  m_sourcePos = m_source->getPos();
}

void DetectorInfo::cacheSample() const {
  m_sample = m_instrument.getSample();
  if (!m_sample)
    throw std::runtime_error("Instrument does not contain sample!");
  m_samplePos = m_sample->getPos();
}

void DetectorInfo::cacheL1() const {
  std::call_once(m_sourceCached, &DetectorInfo::cacheSource, this);
  std::call_once(m_sampleCached, &DetectorInfo::cacheSample, this);
  m_L1 = m_source->getDistance(*m_sample);
}

} // namespace API
} // namespace Mantid
