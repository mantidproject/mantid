// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidBeamline/DetectorInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Geometry {
/** Construct DetectorInfo based on an Instrument.
 *
 * The Instrument reference `instrument` must be the parameterized instrument
 * obtained from a workspace. Detector ID -> index map provided as constructor
 * argument. */
DetectorInfo::DetectorInfo(std::unique_ptr<Beamline::DetectorInfo> detectorInfo,
                           std::shared_ptr<const Geometry::Instrument> instrument,
                           std::shared_ptr<const std::vector<detid_t>> detectorIds,
                           std::shared_ptr<const std::unordered_map<detid_t, size_t>> detIdToIndexMap)
    : m_detectorInfo(std::move(detectorInfo)), m_instrument(std::move(instrument)),
      m_detectorIDs(std::move(detectorIds)), m_detIDToIndex(std::move(detIdToIndexMap)),
      m_lastDetector(PARALLEL_GET_MAX_THREADS), m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {

  // Note: This does not seem possible currently (the instrument objects is
  // always allocated, even if it is empty), so this will not fail.
  if (!m_instrument)
    throw std::invalid_argument("DetectorInfo::DetectorInfo Workspace does not contain an instrument!");

  if (m_detectorIDs->size() != m_detIDToIndex->size()) {
    throw std::invalid_argument("DetectorInfo::DetectorInfo: ID and ID->index map do not match");
  }
}

/** Copy constructor. Use with EXTREME CARE.
 *
 * Public copy should not be used since proper links between DetectorInfo and
 * ComponentInfo must be set up. */
DetectorInfo::DetectorInfo(const DetectorInfo &other)
    : m_detectorInfo(std::make_unique<Beamline::DetectorInfo>(*other.m_detectorInfo)), m_instrument(other.m_instrument),
      m_detectorIDs(other.m_detectorIDs), m_detIDToIndex(other.m_detIDToIndex),
      m_lastDetector(PARALLEL_GET_MAX_THREADS), m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

/// Assigns the contents of the non-wrapping part of `rhs` to this.
DetectorInfo &DetectorInfo::operator=(const DetectorInfo &rhs) {
  if (detectorIDs() != rhs.detectorIDs())
    throw std::runtime_error("DetectorInfo::operator=: Detector IDs in "
                             "assignment do not match. Assignment not "
                             "possible");
  // Do NOT assign anything in the "wrapping" part of DetectorInfo. We simply
  // assign the underlying Beamline::DetectorInfo.
  *m_detectorInfo = *rhs.m_detectorInfo;
  return *this;
}

// Defined as default in source for forward declaration with std::unique_ptr.
DetectorInfo::~DetectorInfo() = default;

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
  return m_detectorInfo->isEquivalent(*other.m_detectorInfo);
}

/// Returns the size of the DetectorInfo, i.e., the number of detectors in the
/// instrument.
size_t DetectorInfo::size() const { return m_detectorIDs->size(); }

/// Returns true if the beamline has scanning detectors.
bool DetectorInfo::isScanning() const { return m_detectorInfo->isScanning(); }

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const size_t index) const { return m_detectorInfo->isMonitor(index); }

/// Returns true if the detector is a monitor.
bool DetectorInfo::isMonitor(const std::pair<size_t, size_t> &index) const { return m_detectorInfo->isMonitor(index); }

/// Returns true if the detector is masked.
bool DetectorInfo::isMasked(const size_t index) const { return m_detectorInfo->isMasked(index); }

/// Returns true if the detector is masked.
bool DetectorInfo::isMasked(const std::pair<size_t, size_t> &index) const { return m_detectorInfo->isMasked(index); }

/// Returns true if there are masked detectors
bool DetectorInfo::hasMaskedDetectors() const { return m_detectorInfo->hasMaskedDetectors(); }

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
    throw std::logic_error("Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }

  const auto sampleDetVec = position(index) - samplePos;
  return sampleDetVec.angle(beamLine);
}

/// Returns 2 theta (scattering angle w.r.t. to beam direction).
double DetectorInfo::twoTheta(const std::pair<size_t, size_t> &index) const {
  if (isMonitor(index))
    throw std::logic_error("Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }

  const auto sampleDetVec = position(index) - samplePos;
  return sampleDetVec.angle(beamLine);
}

/// Returns signed 2 theta (signed scattering angle w.r.t. to beam direction).
double DetectorInfo::signedTwoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error("Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }
  // Get the axis defining the sign
  const auto &instrumentUpAxis = m_instrument->getReferenceFrame()->vecThetaSign();

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
double DetectorInfo::signedTwoTheta(const std::pair<size_t, size_t> &index) const {
  if (isMonitor(index))
    throw std::logic_error("Two theta (scattering angle) is not defined for monitors.");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }
  // Get the axis defining the sign
  const auto &instrumentUpAxis = m_instrument->getReferenceFrame()->vecThetaSign();

  const auto sampleDetVec = position(index) - samplePos;
  double angle = sampleDetVec.angle(beamLine);

  const auto cross = beamLine.cross_prod(sampleDetVec);
  const auto normToSurface = beamLine.cross_prod(instrumentUpAxis);
  if (normToSurface.scalar_prod(cross) < 0) {
    angle *= -1;
  }
  return angle;
}

double DetectorInfo::azimuthal(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error("Azimuthal angle is not defined for monitors");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }

  const auto sampleDetVec = position(index) - samplePos;
  const auto beamLineNormalized = Kernel::normalize(beamLine);

  // generate the vertical axis
  const auto origHorizontal = m_instrument->getReferenceFrame()->vecPointingHorizontal();
  const auto vertical = beamLineNormalized.cross_prod(origHorizontal);
  if (vertical.scalar_prod(m_instrument->getReferenceFrame()->vecPointingUp()) <= 0.)
    throw std::runtime_error("Failed to create up axis orthogonal to the beam direction");

  // generate the horizontal axis perpendicular to the other two
  const auto horizontal = vertical.cross_prod(beamLineNormalized);
  if (origHorizontal.scalar_prod(horizontal) <= 0.)
    throw std::runtime_error("Failed to create horizontal axis orthogonal to the beam direction");

  const double dotHorizontal = sampleDetVec.scalar_prod(horizontal);
  const double dotVertical = sampleDetVec.scalar_prod(vertical);

  return atan2(dotVertical, dotHorizontal);
}

double DetectorInfo::azimuthal(const std::pair<size_t, size_t> &index) const {
  if (isMonitor(index))
    throw std::logic_error("Azimuthal angle is not defined for monitors");

  const auto samplePos = samplePosition();
  const auto beamLine = samplePos - sourcePosition();

  if (beamLine.nullVector()) {
    throw Kernel::Exception::InstrumentDefinitionError("Source and sample are at same position!");
  }

  const auto sampleDetVec = position(index) - samplePos;
  const auto beamLineNormalized = Kernel::normalize(beamLine);

  // generate the vertical axis
  const auto origHorizontal = m_instrument->getReferenceFrame()->vecPointingHorizontal();
  const auto vertical = beamLineNormalized.cross_prod(origHorizontal);
  if (vertical.scalar_prod(m_instrument->getReferenceFrame()->vecPointingUp()) <= 0.)
    throw std::runtime_error("Failed to create up axis orthogonal to the beam direction");

  // generate the horizontal axis perpendicular to the other two
  const auto horizontal = vertical.cross_prod(beamLineNormalized);
  if (origHorizontal.scalar_prod(horizontal) <= 0.)
    throw std::runtime_error("Failed to create horizontal axis orthogonal to the beam direction");

  const double dotHorizontal = sampleDetVec.scalar_prod(horizontal);
  const double dotVertical = sampleDetVec.scalar_prod(vertical);

  return atan2(dotVertical, dotHorizontal);
}

std::tuple<double, double, double> DetectorInfo::diffractometerConstants(const size_t index,
                                                                         std::vector<detid_t> &calibratedDets,
                                                                         std::vector<detid_t> &uncalibratedDets) const {
  auto det = m_instrument->getDetector((*m_detectorIDs)[index]);
  auto pmap = m_instrument->getParameterMap();
  auto par = pmap->get(det.get(), "DIFC");
  if (par) {
    double difc = par->value<double>();
    calibratedDets.push_back((*m_detectorIDs)[index]);
    double difa = 0., tzero = 0.;
    par = pmap->get(det.get(), "DIFA");
    if (par)
      difa = par->value<double>();
    par = pmap->get(det.get(), "TZERO");
    if (par)
      tzero = par->value<double>();
    return {difa, difc, tzero};
  } else {
    // if calibrated difc not available, revert to uncalibrated difc with other
    // two constants=0
    uncalibratedDets.push_back((*m_detectorIDs)[index]);
    double difc = difcUncalibrated(index);
    return {0., difc, 0.};
  }
}

double DetectorInfo::difcUncalibrated(const size_t index) const {
  return 1. / Kernel::Units::tofToDSpacingFactor(l1(), l2(index), twoTheta(index), 0.);
}

std::pair<double, double> DetectorInfo::geographicalAngles(const size_t index) const {
  const auto samplePos = samplePosition();
  const auto sampleDetVec = position(index) - samplePos;
  const double upCoord = sampleDetVec[m_instrument->getReferenceFrame()->pointingUp()];
  const double beamCoord = sampleDetVec[m_instrument->getReferenceFrame()->pointingAlongBeam()];
  const double leftoverCoord = sampleDetVec[m_instrument->getReferenceFrame()->pointingHorizontal()];
  const double lat = std::atan2(upCoord, std::hypot(leftoverCoord, beamCoord));
  const double lon = std::atan2(leftoverCoord, beamCoord);
  return std::pair<double, double>(lat, lon);
}

std::pair<double, double> DetectorInfo::geographicalAngles(const std::pair<size_t, size_t> &index) const {
  const auto samplePos = samplePosition();
  const auto sampleDetVec = position(index) - samplePos;
  const double upCoord = sampleDetVec[m_instrument->getReferenceFrame()->pointingUp()];
  const double beamCoord = sampleDetVec[m_instrument->getReferenceFrame()->pointingAlongBeam()];
  const double leftoverCoord = sampleDetVec[m_instrument->getReferenceFrame()->pointingHorizontal()];
  const double lat = std::atan2(upCoord, std::hypot(leftoverCoord, beamCoord));
  const double lon = std::atan2(leftoverCoord, beamCoord);
  return std::pair<double, double>(lat, lon);
}

/// Returns the position of the detector with given index.
Kernel::V3D DetectorInfo::position(const size_t index) const { return Kernel::toV3D(m_detectorInfo->position(index)); }

/// Returns the position of the detector with given index.
Kernel::V3D DetectorInfo::position(const std::pair<size_t, size_t> &index) const {
  return Kernel::toV3D(m_detectorInfo->position(index));
}

/// Returns the rotation of the detector with given index.
Kernel::Quat DetectorInfo::rotation(const size_t index) const {
  return Kernel::toQuat(m_detectorInfo->rotation(index));
}

/// Returns the rotation of the detector with given index.
Kernel::Quat DetectorInfo::rotation(const std::pair<size_t, size_t> &index) const {
  return Kernel::toQuat(m_detectorInfo->rotation(index));
}

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const size_t index, bool masked) { m_detectorInfo->setMasked(index, masked); }

/// Set the mask flag of the detector with given index. Not thread safe.
void DetectorInfo::setMasked(const std::pair<size_t, size_t> &index, bool masked) {
  m_detectorInfo->setMasked(index, masked);
}

/** Sets all mask flags to false (unmasked). Not thread safe.
 *
 * This method was introduced to help with refactoring and may be removed in the
 *future. */
void DetectorInfo::clearMaskFlags() {
  for (size_t i = 0; i < size(); ++i)
    m_detectorInfo->setMasked(i, false);
}

/// Set the absolute position of the detector with given index. Not thread safe.
void DetectorInfo::setPosition(const size_t index, const Kernel::V3D &position) {

  clearPositionDependentParameters(index);
  m_detectorInfo->setPosition(index, Kernel::toVector3d(position));
}

/// Set the absolute position of the detector with given index. Not thread safe.
void DetectorInfo::setPosition(const std::pair<size_t, size_t> &index, const Kernel::V3D &position) {
  clearPositionDependentParameters(index.first);
  m_detectorInfo->setPosition(index, Kernel::toVector3d(position));
}

// Clear any parameters whose value is only valid for specific positions
// Currently diffractometer constants
void DetectorInfo::clearPositionDependentParameters(const size_t index) {
  auto det = m_instrument->getDetector((*m_detectorIDs)[index]);
  auto pmap = m_instrument->getParameterMap();
  pmap->clearParametersByName("DIFA", det.get());
  pmap->clearParametersByName("DIFC", det.get());
  pmap->clearParametersByName("TZERO", det.get());
}

/// Set the absolute rotation of the detector with given index. Not thread safe.
void DetectorInfo::setRotation(const size_t index, const Kernel::Quat &rotation) {
  m_detectorInfo->setRotation(index, Kernel::toQuaterniond(rotation));
}

/// Set the absolute rotation of the detector with given index. Not thread safe.
void DetectorInfo::setRotation(const std::pair<size_t, size_t> &index, const Kernel::Quat &rotation) {
  m_detectorInfo->setRotation(index, Kernel::toQuaterniond(rotation));
}

/// Return a const reference to the detector with given index.
const Geometry::IDetector &DetectorInfo::detector(const size_t index) const { return getDetector(index); }

/// Returns the source position.
Kernel::V3D DetectorInfo::sourcePosition() const { return Kernel::toV3D(m_detectorInfo->sourcePosition()); }

/// Returns the sample position.
Kernel::V3D DetectorInfo::samplePosition() const { return Kernel::toV3D(m_detectorInfo->samplePosition()); }

/// Returns L1 (distance from source to sample).
double DetectorInfo::l1() const { return m_detectorInfo->l1(); }

/// Returns a sorted vector of all detector IDs.
const std::vector<detid_t> &DetectorInfo::detectorIDs() const { return *m_detectorIDs; }

std::size_t DetectorInfo::indexOf(const detid_t id) const {
  try {
    return m_detIDToIndex->at(id);
  } catch (const std::out_of_range &) {
    // customize the error message
    std::stringstream msg;
    msg << "Failed to find detector with id=" << id;
    throw std::out_of_range(msg.str());
  }
}

/// Returns the scan count of the detector with given detector index.
size_t DetectorInfo::scanCount() const { return m_detectorInfo->scanCount(); }

/** Returns the scan interval of the detector with given index.
 *
 * The interval start and end values would typically correspond to nanoseconds
 * since 1990, as in Types::Core::DateAndTime. */
const std::vector<std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>> DetectorInfo::scanIntervals() const {
  const auto &intervals = m_detectorInfo->scanIntervals();
  return {intervals.begin(), intervals.end()};
}

const DetectorInfoConstIt DetectorInfo::cbegin() const { return DetectorInfoConstIt(*this, 0, size()); }

const DetectorInfoConstIt DetectorInfo::cend() const { return DetectorInfoConstIt(*this, size(), size()); }

const Geometry::IDetector &DetectorInfo::getDetector(const size_t index) const {
  auto thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] != index) {
    m_lastIndex[thread] = index;
    m_lastDetector[thread] = m_instrument->getDetector((*m_detectorIDs)[index]);
  }

  return *m_lastDetector[thread];
}

/// Helper used by SpectrumInfo.
std::shared_ptr<const Geometry::IDetector> DetectorInfo::getDetectorPtr(const size_t index) const {
  auto thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  static_cast<void>(getDetector(index));
  return m_lastDetector[thread];
}

// Begin method for iterator
DetectorInfoIt DetectorInfo::begin() { return DetectorInfoIt(*this, 0, size()); }

// End method for iterator
DetectorInfoIt DetectorInfo::end() { return DetectorInfoIt(*this, size(), size()); }

} // namespace Geometry
} // namespace Mantid
