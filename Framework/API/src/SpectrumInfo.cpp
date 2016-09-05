#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace API {

SpectrumInfo::SpectrumInfo(const MatrixWorkspace &workspace)
    : m_workspace(workspace), m_instrument(workspace.getInstrument()),
      m_L1(-1.0), m_detectors(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {
  // Note: This does not seem possible currently (the instrument objects is
  // always allocated, even if it is empty), so this will not fail.
  if (!m_instrument)
    throw std::runtime_error("Workspace " + workspace.getName() +
                             " does not contain an instrument!");
}

// Defined as default in source for forward declaration with std::unique_ptr.
SpectrumInfo::~SpectrumInfo() = default;

/// Returns true if the detector(s) associated with the spectrum are monitors.
bool SpectrumInfo::isMonitor(const size_t index) const {
  return getDetector(index).isMonitor();
}

/// Returns true if the detector(s) associated with the spectrum are masked.
bool SpectrumInfo::isMasked(const size_t index) const {
  return getDetector(index).isMasked();
}

/** Returns L2 (distance from sample to spectrum).
 *
 * For monitors this is defined such that L1+L2 = source-detector distance,
 * i.e., for a monitor in the beamline between source and sample L2 is negative.
 */
double SpectrumInfo::l2(const size_t index) const {
  if (!isMonitor(index))
    return getDetector(index).getDistance(getSample());
  else
    return getDetector(index).getDistance(getSource()) - l1();
}

/// Returns 2 theta (angle w.r.t. to beam direction).
double SpectrumInfo::twoTheta(const size_t index) const {
  // Note: This function has big overlap with the method
  // MatrixWorkspace::detectorTwoTheta(). The plan is to eventually remove the
  // latter, once SpectrumInfo is in widespread use.
  const Kernel::V3D samplePos = getSamplePos();
  const Kernel::V3D beamLine = samplePos - getSourcePos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  return getDetector(index).getTwoTheta(samplePos, beamLine);
}

/// Returns signed 2 theta (signed angle w.r.t. to beam direction).
double SpectrumInfo::signedTwoTheta(const size_t index) const {
  // Note: This function has big overlap with the method
  // MatrixWorkspace::detectorSignedTwoTheta(). The plan is to eventually remove
  // the latter, once SpectrumInfo is in widespread use.
  const Kernel::V3D samplePos = getSamplePos();
  const Kernel::V3D beamLine = samplePos - getSourcePos();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const Kernel::V3D &instrumentUpAxis =
      m_instrument->getReferenceFrame()->vecPointingUp();
  return getDetector(index)
      .getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
}

Kernel::V3D SpectrumInfo::position(const size_t index) const {
  return getDetector(index).getPos();
}

bool SpectrumInfo::hasDetectors(const size_t index) const {
  return m_workspace.getSpectrum(index).getDetectorIDs().size() > 0;
}

bool SpectrumInfo::hasUniqueDetector(const size_t index) const {
  return m_workspace.getSpectrum(index).getDetectorIDs().size() == 1;
}

/// Returns L1 (distance from source to sample).
double SpectrumInfo::l1() const {
  std::call_once(m_L1Cached, &SpectrumInfo::cacheL1, this);
  return m_L1;
}

const Geometry::IDetector &SpectrumInfo::getDetector(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] == index)
    return *m_detectors[thread];

  m_lastIndex[thread] = index;

  // Note: This function body has big overlap with the method
  // MatrixWorkspace::getDetector(). The plan is to eventually remove the
  // latter, once SpectrumInfo is in widespread use.
  const auto &dets = m_workspace.getSpectrum(index).getDetectorIDs();
  const size_t ndets = dets.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    m_detectors[thread] = m_instrument->getDetector(*dets.begin());
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           "");
  } else {
    // Else need to construct a DetectorGroup and use that
    auto dets_ptr = m_instrument->getDetectors(dets);
    m_detectors[thread] = Geometry::IDetector_const_sptr(
        new Geometry::DetectorGroup(dets_ptr, false));
  }

  return *m_detectors[thread];
}

/// Returns a reference to the source component. The value is cached, so calling
/// it repeatedly is cheap.
const Geometry::IComponent &SpectrumInfo::getSource() const {
  std::call_once(m_sourceCached, &SpectrumInfo::cacheSource, this);
  return *m_source;
}

/// Returns a reference to the sample component. The value is cached, so calling
/// it repeatedly is cheap.
const Geometry::IComponent &SpectrumInfo::getSample() const {
  std::call_once(m_sampleCached, &SpectrumInfo::cacheSample, this);
  return *m_sample;
}

/// Returns the source position. The value is cached, so calling it repeatedly
/// is cheap.
Kernel::V3D SpectrumInfo::getSourcePos() const {
  std::call_once(m_sourceCached, &SpectrumInfo::cacheSource, this);
  return m_sourcePos;
}

/// Returns the sample position. The value is cached, so calling it repeatedly
/// is cheap.
Kernel::V3D SpectrumInfo::getSamplePos() const {
  std::call_once(m_sampleCached, &SpectrumInfo::cacheSample, this);
  return m_samplePos;
}

void SpectrumInfo::cacheSource() const {
  m_source = m_instrument->getSource();
  if (!m_source)
    throw std::runtime_error("Instrument in workspace " +
                             m_workspace.getName() +
                             " does not contain source!");
  m_sourcePos = m_source->getPos();
}

void SpectrumInfo::cacheSample() const {
  m_sample = m_instrument->getSample();
  if (!m_sample)
    throw std::runtime_error("Instrument in workspace " +
                             m_workspace.getName() +
                             "  does not contain sample!");
  m_samplePos = m_sample->getPos();
}

void SpectrumInfo::cacheL1() const {
  std::call_once(m_sourceCached, &SpectrumInfo::cacheSource, this);
  std::call_once(m_sampleCached, &SpectrumInfo::cacheSample, this);
  m_L1 = m_source->getDistance(*m_sample);
}

} // namespace API
} // namespace Mantid
