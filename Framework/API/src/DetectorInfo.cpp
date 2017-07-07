#include "MantidAPI/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidBeamline/DetectorInfo.h"
#include "MantidKernel/EigenConversionHelpers.h"
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
 * is not null. Detector ID -> index map provided as constructor argument.
 *
 * */
DetectorInfo::DetectorInfo(
    Beamline::DetectorInfo &detectorInfo,
    boost::shared_ptr<const Geometry::Instrument> instrument,
    boost::shared_ptr<std::vector<detid_t>> detectorIds,
    Geometry::ParameterMap *pmap,
    boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
        detIdToIndexMap)
    : m_detectorInfo(detectorInfo), m_pmap(pmap), m_instrument(instrument),
      m_detectorIDs(detectorIds), m_detIDToIndex(detIdToIndexMap),
      m_lastDetector(PARALLEL_GET_MAX_THREADS),
      m_lastAssemblyDetectorIndices(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {

  // Note: This does not seem possible currently (the instrument objects is
  // always allocated, even if it is empty), so this will not fail.
  if (!m_instrument)
    throw std::invalid_argument(
        "DetectorInfo::DetectorInfo Workspace does not contain an instrument!");

  if (m_detectorIDs->size() != m_detIDToIndex->size()) {
    throw std::invalid_argument(
        "DetectorInfo::DetectorInfo: ID and ID->index map do not match");
  }
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

/** Returns true if the content of this is equivalent to the content of other.
 *
 * Here "equivalent" implies equality of all member, except for positions and
 * rotations, which are treated specially:
 * - Positions that differ by less than 1 nm = 1e-9 m are considered equivalent.
 * - Rotations that imply relative position changes of less than 1 nm = 1e-9 m
 *   with a rotation center that is 1000 m away are considered equivalent.
 * Note that in both cases the actual limit may be lower, but it is guarenteed
 * that any LARGER differences are NOT considered equivalent. */
bool DetectorInfo::isEquivalent(const DetectorInfo &other) const {
  return m_detectorInfo.isEquivalent(other.m_detectorInfo);
}

/// Returns the size of the DetectorInfo, i.e., the number of detectors in the
/// instrument.
size_t DetectorInfo::size() const { return m_detectorIDs->size(); }

/// Returns true if the beamline has scanning detectors.
bool DetectorInfo::isScanning() const { return m_detectorInfo.isScanning(); }

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const {
  return m_detectorInfo.isMonitor(index);
}

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const std::pair<size_t, size_t> &index) const {
  return m_detectorInfo.isMonitor(index);
}

/// Returns true if the detector is masked.
bool DetectorInfo::isMasked(const size_t index) const {
  return m_detectorInfo.isMasked(index);
}

/// Returns true if the detector is masked.
bool DetectorInfo::isMasked(const std::pair<size_t, size_t> &index) const {
  return m_detectorInfo.isMasked(index);
}

/** Returns L2 (distance from sample to spectrum).
 *
 * For monitors this is defined such that L1+L2 = source-detector distance,
 * i.e., for a monitor in the beamline between source and sample L2 is negative.
 */
double DetectorInfo::l2(const size_t index) const {
  if (!isMonitor(index))
    return position(index).distance(samplePosition());
  else
    return position(index).distance(sourcePosition()) - l1();
}

/** Returns L2 (distance from sample to spectrum).
 *
 * For monitors this is defined such that L1+L2 = source-detector distance,
 * i.e., for a monitor in the beamline between source and sample L2 is negative.
 */
double DetectorInfo::l2(const std::pair<size_t, size_t> &index) const {
  if (!isMonitor(index))
    return position(index).distance(samplePosition());
  else
    return position(index).distance(sourcePosition()) - l1();
}

/// Returns 2 theta (scattering angle w.r.t. to beam direction).
double DetectorInfo::twoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  const auto sampleDetVec = position(index) - samplePos;
  return sampleDetVec.angle(beamLine);
}

/// Returns 2 theta (scattering angle w.r.t. to beam direction).
double DetectorInfo::twoTheta(const std::pair<size_t, size_t> &index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }

  const auto sampleDetVec = position(index) - samplePos;
  return sampleDetVec.angle(beamLine);
}

/// Returns signed 2 theta (signed scattering angle w.r.t. to beam direction).
double DetectorInfo::signedTwoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const auto &instrumentUpAxis =
      m_instrument->getReferenceFrame()->vecPointingUp();

  const auto sampleDetVec = position(index) - samplePos;
  double angle = sampleDetVec.angle(beamLine);

  const auto cross = beamLine.cross_prod(sampleDetVec);
  const auto normToSurface = beamLine.cross_prod(instrumentUpAxis);
  if (normToSurface.scalar_prod(cross) < 0) {
    angle *= -1;
  }
  return angle;
}

/// Returns signed 2 theta (signed scattering angle w.r.t. to beam direction).
double
DetectorInfo::signedTwoTheta(const std::pair<size_t, size_t> &index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Source and sample are at same position!");
  }
  // Get the instrument up axis.
  const auto &instrumentUpAxis =
      m_instrument->getReferenceFrame()->vecPointingUp();

  const auto sampleDetVec = position(index) - samplePos;
  double angle = sampleDetVec.angle(beamLine);

  const auto cross = beamLine.cross_prod(sampleDetVec);
  const auto normToSurface = beamLine.cross_prod(instrumentUpAxis);
  if (normToSurface.scalar_prod(cross) < 0) {
    angle *= -1;
  }
  return angle;
}

/// Returns the position of the detector with given index.
Kernel::V3D DetectorInfo::position(const size_t index) const {
  return Kernel::toV3D(m_detectorInfo.position(index));
}

/// Returns the position of the detector with given index.
Kernel::V3D
DetectorInfo::position(const std::pair<size_t, size_t> &index) const {
  return Kernel::toV3D(m_detectorInfo.position(index));
}

/// Returns the rotation of the detector with given index.
Kernel::Quat DetectorInfo::rotation(const size_t index) const {
  return Kernel::toQuat(m_detectorInfo.rotation(index));
}

/// Returns the rotation of the detector with given index.
Kernel::Quat
DetectorInfo::rotation(const std::pair<size_t, size_t> &index) const {
  return Kernel::toQuat(m_detectorInfo.rotation(index));
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const size_t index, bool masked) {
  m_detectorInfo.setMasked(index, masked);
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const std::pair<size_t, size_t> &index,
                             bool masked) {
  m_detectorInfo.setMasked(index, masked);
}

/** Sets all mask flags to false (unmasked). Not thread safe.
 *
 * This method was introduced to help with refactoring and may be removed in the
 *future. */
void DetectorInfo::clearMaskFlags() {
  for (size_t i = 0; i < size(); ++i)
    m_detectorInfo.setMasked(i, false);
}

/// Set the absolute position of the detector with given index. Not thread safe.
void DetectorInfo::setPosition(const size_t index,
                               const Kernel::V3D &position) {
  m_detectorInfo.setPosition(index, Kernel::toVector3d(position));
}

/// Set the absolute position of the detector with given index. Not thread safe.
void DetectorInfo::setPosition(const std::pair<size_t, size_t> &index,
                               const Kernel::V3D &position) {
  m_detectorInfo.setPosition(index, Kernel::toVector3d(position));
}

/// Set the absolute rotation of the detector with given index. Not thread safe.
void DetectorInfo::setRotation(const size_t index,
                               const Kernel::Quat &rotation) {
  m_detectorInfo.setRotation(index, Kernel::toQuaterniond(rotation));
}

/// Set the absolute rotation of the detector with given index. Not thread safe.
void DetectorInfo::setRotation(const std::pair<size_t, size_t> &index,
                               const Kernel::Quat &rotation) {
  m_detectorInfo.setRotation(index, Kernel::toQuaterniond(rotation));
}

/** Set the absolute position of the component `comp`.
 *
 * This may or may not be a detector. Even if it is not a detector it will
 * typically still influence detector positions. Note that this method will be
 * removed once ComponentInfo::setPosition is available. */
void DetectorInfo::setPosition(const Geometry::IComponent &comp,
                               const Kernel::V3D &pos) {
  if (const auto *det = dynamic_cast<const Geometry::Detector *>(&comp)) {
    const auto index = indexOf(det->getID());
    setPosition(index, pos);
  } else {
    const auto &detIndices = getAssemblyDetectorIndices(comp);
    if ((!detIndices.empty()) && m_detectorInfo.isScanning())
      throw std::runtime_error("Cannot move parent component containing "
                               "detectors since the beamline has "
                               "time-dependent (moving) detectors.");
    // This will go badly wrong if the parameter map in the component is not
    // identical to ours, but there does not seem to be a way to check?
    const auto oldPos = comp.getPos();
    using namespace Geometry::ComponentHelper;
    TransformType positionType = Absolute;
    moveComponent(comp, *m_pmap, pos, positionType);
    // If comp is a detector cached positions stay valid. In all other cases
    // (higher level in instrument tree, or other leaf component such as sample
    // or source) we flush all cached positions.
    if (detIndices.size() == 0 || detIndices.size() == size()) {
      // Update only if comp is not a bank (not detectors) or the full
      // instrument (all detectors). The should make this thread-safe for
      // practical purposes such as moving each bank in a separate thread.
      if (m_source)
        m_sourcePos = m_source->getPos();
      if (m_sample)
        m_samplePos = m_sample->getPos();
      if (m_source && m_sample)
        m_L1 = m_sourcePos.distance(m_samplePos);
    }
    // Detector positions are currently not cached, the cached pointers to
    // detectors stay valid. Once we store positions in DetectorInfo we need to
    // update detector positions here.
    const auto delta = toVector3d(pos - oldPos);
    for (const auto index : detIndices) {
      m_detectorInfo.setPosition(index, m_detectorInfo.position(index) + delta);
    }
  }
}

/** Set the absolute rotation of the component `comp`.
 *
 * This may or may not be a detector. Even if it is not a detector it will
 * typically still influence detector positions rotations. Note that this method
 * will be removed once ComponentInfo::setRotation is available. */
void DetectorInfo::setRotation(const Geometry::IComponent &comp,
                               const Kernel::Quat &rot) {
  if (const auto *det = dynamic_cast<const Geometry::Detector *>(&comp)) {
    const auto index = indexOf(det->getID());
    setRotation(index, rot);
  } else {
    const auto &detIndices = getAssemblyDetectorIndices(comp);
    if ((!detIndices.empty()) && m_detectorInfo.isScanning())
      throw std::runtime_error("Cannot move parent component containing "
                               "detectors since the beamline has "
                               "time-dependent (moving) detectors.");
    // This will go badly wrong if the parameter map in the component is not
    // identical to ours, but there does not seem to be a way to check?
    const auto pos = toVector3d(comp.getPos());
    auto invOldRot = comp.getRotation();
    invOldRot.inverse();
    const auto delta = toQuaterniond(rot * invOldRot).normalized();
    using namespace Geometry::ComponentHelper;
    TransformType rotationType = Absolute;
    rotateComponent(comp, *m_pmap, rot, rotationType);
    // If comp is a detector cached positions and rotations stay valid. In all
    // other cases (higher level in instrument tree, or other leaf component
    // such as sample or source) we flush all cached positions and rotations.
    if (detIndices.size() == 0 || detIndices.size() == size()) {
      // Update only if comp is not a bank (not detectors) or the full
      // instrument (all detectors). The should make this thread-safe for
      // practical purposes such as moving each bank in a separate thread.
      if (m_source)
        m_sourcePos = m_source->getPos();
      if (m_sample)
        m_samplePos = m_sample->getPos();
      if (m_source && m_sample)
        m_L1 = m_sourcePos.distance(m_samplePos);
    }
    // Detector positions and rotations are currently not cached, the cached
    // pointers to detectors stay valid. Once we store positions and rotations
    // in DetectorInfo we need to update detector positions and rotations here.
    auto transformation = Eigen::Matrix3d(delta);
    for (const auto index : detIndices) {
      m_detectorInfo.setRotation(index, delta * m_detectorInfo.rotation(index));
      auto relativePos =
          transformation * (m_detectorInfo.position(index) - pos);
      m_detectorInfo.setPosition(index, relativePos + pos);
    }
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
  return *m_detectorIDs;
}

/// Returns the scan count of the detector with given detector index.
size_t DetectorInfo::scanCount(const size_t index) const {
  return m_detectorInfo.scanCount(index);
}

/** Returns the scan interval of the detector with given index.
 *
 * The interval start and end values would typically correspond to nanoseconds
 * since 1990, as in Kernel::DateAndTime. */
std::pair<Kernel::DateAndTime, Kernel::DateAndTime>
DetectorInfo::scanInterval(const std::pair<size_t, size_t> &index) const {
  const auto &interval = m_detectorInfo.scanInterval(index);
  return {interval.first, interval.second};
}

/** Set the scan interval of the detector with given detector index.
 *
 * The interval start and end values would typically correspond to nanoseconds
 * since 1990, as in Kernel::DateAndTime. Note that it is currently not possible
 * to modify scan intervals for a DetectorInfo with time-dependent detectors,
 * i.e., time intervals must be set with this method before merging individual
 * scans. */
void DetectorInfo::setScanInterval(
    const size_t index,
    const std::pair<Kernel::DateAndTime, Kernel::DateAndTime> &interval) {
  m_detectorInfo.setScanInterval(index, {interval.first.totalNanoseconds(),
                                         interval.second.totalNanoseconds()});
}

/** Merges the contents of other into this.
 *
 * Scan intervals in both other and this must be set. Intervals must be
 * identical or non-overlapping. If they are identical all other parameters (for
 * that index) must match. */
void DetectorInfo::merge(const DetectorInfo &other) {
  m_detectorInfo.merge(other.m_detectorInfo);
}

const Geometry::IDetector &DetectorInfo::getDetector(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] != index) {
    m_lastIndex[thread] = index;
    m_lastDetector[thread] = m_instrument->getDetector((*m_detectorIDs)[index]);
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

/** Returns a reference to a vector of detector indices in a CompAssembly.
 *
 * Note that this method will be removed once ComponentInfo provides this
 * functionality. */
const std::vector<size_t> &DetectorInfo::getAssemblyDetectorIndices(
    const Geometry::IComponent &comp) const {
  const auto *base = comp.getBaseComponent();
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastAssemblyDetectorIndices[thread].first != base) {
    m_lastAssemblyDetectorIndices[thread].first = base;
    m_lastAssemblyDetectorIndices[thread].second.clear();
    std::vector<Geometry::IDetector_const_sptr> dets;
    m_instrument->getDetectorsInBank(dets, *base);
    for (const auto &det : dets) {
      const auto index = indexOf(det->getID());
      m_lastAssemblyDetectorIndices[thread].second.push_back(index);
    }
  }
  return m_lastAssemblyDetectorIndices[thread].second;
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

boost::shared_ptr<const std::unordered_map<detid_t, size_t>>
DetectorInfo::detIdToIndexMap() const {
  return m_detIDToIndex;
}
} // namespace API
} // namespace Mantid
