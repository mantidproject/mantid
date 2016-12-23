#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"

namespace Mantid {
namespace API {

/** Construct DetectorInfo based on an Instrument.
 *
 * The Instrument reference `instrument` must be the parameterized instrument
 * obtained from a workspace. The pointer to the ParameterMap `pmap` can be
 * null. If it is not null, it must refer to the same map as wrapped in
 * `instrument`. Non-const methods of DetectorInfo may only be called if `pmap`
 * is not null. */
DetectorInfo::DetectorInfo(
    Beamline::DetectorInfo &detectorInfo,
    boost::shared_ptr<const Geometry::Instrument> instrument,
    Geometry::ParameterMap *pmap)
    : m_detectorInfo(detectorInfo), m_pmap(pmap), m_instrument(instrument),
      m_lastDetector(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {
  // Note: This does not seem possible currently (the instrument objects is
  // always allocated, even if it is empty), so this will not fail.
  if (!m_instrument)
    throw std::runtime_error("Workspace does not contain an instrument!");

  m_detectorIDs = instrument->getDetectorIDs(false /* do not skip monitors */);
  for (size_t i = 0; i < m_detectorIDs.size(); ++i)
    m_detIDToIndex[m_detectorIDs[i]] = i;
}

/// Assigns the contents of the non-wrapping part of `rhs` to this.
DetectorInfo &DetectorInfo::operator=(const DetectorInfo &rhs) {
  if (detectorIDs() != rhs.detectorIDs())
    throw std::runtime_error("DetectorInfo::operator=: Detector IDs in "
                             "assignment do not match. Assignment not "
                             "possible");
  // Do NOT assign anything in the "wrapping" part of DetectorInfo. We simply
  // assign the underlying Beamline::DetectorInfo.
  m_detectorInfo = rhs.m_detectorInfo;
  return *this;
}

/// Returns the size of the DetectorInfo, i.e., the number of detectors in the
/// instrument.
size_t DetectorInfo::size() const { return m_detectorIDs.size(); }

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  return getDetector(index).isMonitor();
}

/// Returns true if the detector is a masked.
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
      m_instrument->getReferenceFrame()->vecPointingUp();
  return getDetector(index)
      .getSignedTwoTheta(samplePos, beamLine, instrumentUpAxis);
}

/// Returns the position of the detector with given index.
Kernel::V3D DetectorInfo::position(const size_t index) const {
  return getDetector(index).getPos();
}

/// Returns the rotation of the detector with given index.
Kernel::Quat DetectorInfo::rotation(const size_t index) const {
  return getDetector(index).getRotation();
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const size_t index, bool masked) {
  // Note that masking via the ParameterMap is actually thread safe, but it will
  // not be with upcoming internal changes.
  m_pmap->addBool(&getDetector(index), "masked", masked);
}

/** Sets all mask flags to false (unmasked). Not thread safe.
 *
 * This method was introduced to help with refactoring and may be removed in the
 *future. */
void DetectorInfo::clearMaskFlags() { m_pmap->clearParametersByName("masked"); }

/// Set the absolute position of the detector with given index. Not thread safe.
void DetectorInfo::setPosition(const size_t index,
                               const Kernel::V3D &position) {
  const auto &det = getDetector(index);
  using namespace Geometry::ComponentHelper;
  TransformType positionType = Absolute;
  moveComponent(det, *m_pmap, position, positionType);
}

/// Set the absolute rotation of the detector with given index. Not thread safe.
void DetectorInfo::setRotation(const size_t index,
                               const Kernel::Quat &rotation) {
  const auto &det = getDetector(index);
  using namespace Geometry::ComponentHelper;
  TransformType rotationType = Absolute;
  rotateComponent(det, *m_pmap, rotation, rotationType);
}

/** Set the absolute position of the component `comp`. Not thread safe.
 *
 * This may or may not be a detector. Even if it is not a detector it will
 * typically still influence detector positions. */
void DetectorInfo::setPosition(const Geometry::IComponent &comp,
                               const Kernel::V3D &pos) {
  using namespace Geometry::ComponentHelper;
  TransformType positionType = Absolute;
  moveComponent(comp, *m_pmap, pos, positionType);

  if (!dynamic_cast<const Geometry::Detector *>(&comp)) {
    // If comp is a detector cached positions stay valid. In all other cases
    // (higher level in instrument tree, or other leaf component such as sample
    // or source) we flush all cached positions.
    if (m_source)
      m_sourcePos = m_source->getPos();
    if (m_sample)
      m_samplePos = m_sample->getPos();
    // Detector positions are currently not cached, the cached pointers to
    // detectors stay valid. Once we store positions in DetectorInfo we need to
    // update detector positions here.
  }
}

/** Set the absolute rotation of the component `comp`. Not thread safe.
 *
 * This may or may not be a detector. Even if it is not a detector it will
 * typically still influence detector positions rotations. */
void DetectorInfo::setRotation(const Geometry::IComponent &comp,
                               const Kernel::Quat &rot) {
  using namespace Geometry::ComponentHelper;
  TransformType rotationType = Absolute;
  rotateComponent(comp, *m_pmap, rot, rotationType);

  if (!dynamic_cast<const Geometry::Detector *>(&comp)) {
    // If comp is a detector cached positions and rotations stay valid. In all
    // other cases/ (higher level in instrument tree, or other leaf component
    // such as sample or source) we flush all cached positions and rotations.
    if (m_source)
      m_sourcePos = m_source->getPos();
    if (m_sample)
      m_samplePos = m_sample->getPos();
    // Detector positions and rotations are currently not cached, the cached
    // pointers to detectors stay valid. Once we store positions and rotations
    // in DetectorInfo we need to update detector positions and rotations here.
  }
}

/// Return a const reference to the detector with given index.
const Geometry::IDetector &DetectorInfo::detector(const size_t index) const {
  return getDetector(index);
}

/// Returns the source position.
Kernel::V3D DetectorInfo::sourcePosition() const {
  cacheSource();
  return m_sourcePos;
}

/// Returns the sample position.
Kernel::V3D DetectorInfo::samplePosition() const {
  cacheSample();
  return m_samplePos;
}

/// Returns L1 (distance from source to sample).
double DetectorInfo::l1() const {
  cacheSource();
  cacheSample();
  std::call_once(m_L1Cached, &DetectorInfo::cacheL1, this);
  return m_L1;
}

/// Returns a sorted vector of all detector IDs.
const std::vector<detid_t> &DetectorInfo::detectorIDs() const {
  return m_detectorIDs;
}

const Geometry::IDetector &DetectorInfo::getDetector(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] != index) {
    m_lastIndex[thread] = index;
    m_lastDetector[thread] = m_instrument->getDetector(m_detectorIDs[index]);
  }

  return *m_lastDetector[thread];
}

/// Helper used by SpectrumInfo.
boost::shared_ptr<const Geometry::IDetector>
DetectorInfo::getDetectorPtr(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  static_cast<void>(getDetector(index));
  return m_lastDetector[thread];
}

/// Sets the cached detector. This is an optimization used by SpectrumInfo.
void DetectorInfo::setCachedDetector(
    size_t index, boost::shared_ptr<const Geometry::IDetector> detector) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  m_lastIndex[thread] = index;
  m_lastDetector[thread] = detector;
}

/// Returns a reference to the source component. The value is cached, so calling
/// it repeatedly is cheap.
const Geometry::IComponent &DetectorInfo::getSource() const {
  cacheSource();
  return *m_source;
}

/// Returns a reference to the sample component. The value is cached, so calling
/// it repeatedly is cheap.
const Geometry::IComponent &DetectorInfo::getSample() const {
  cacheSample();
  return *m_sample;
}

void DetectorInfo::cacheSource() const {
  std::call_once(m_sourceCached, &DetectorInfo::doCacheSource, this);
  if (!m_sourceGood)
    throw std::runtime_error("Instrument does not contain source!");
}

void DetectorInfo::cacheSample() const {
  std::call_once(m_sampleCached, &DetectorInfo::doCacheSample, this);
  if (!m_sampleGood)
    throw std::runtime_error("Instrument does not contain sample!");
}

void DetectorInfo::doCacheSource() const {
  m_source = m_instrument->getSource();
  // Workaround: GCC has trouble with exceptions thrown from with std::call_once
  // (example from cppreference does not work). Instead we set a flag and throw
  // in a wrapper function.
  if (!m_source)
    return;
  m_sourceGood = true;
  m_sourcePos = m_source->getPos();
}

void DetectorInfo::doCacheSample() const {
  m_sample = m_instrument->getSample();
  // Workaround: GCC has trouble with exceptions thrown from with std::call_once
  // (example from cppreference does not work). Instead we set a flag and throw
  // in a wrapper function.
  if (!m_sample)
    return;
  m_sampleGood = true;
  m_samplePos = m_sample->getPos();
}

void DetectorInfo::cacheL1() const { m_L1 = m_source->getDistance(*m_sample); }

} // namespace API
} // namespace Mantid
