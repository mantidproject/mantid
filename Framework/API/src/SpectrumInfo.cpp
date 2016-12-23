#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/make_shared.hpp>
#include <algorithm>

namespace Mantid {
namespace API {

SpectrumInfo::SpectrumInfo(const ExperimentInfo &experimentInfo)
    : m_experimentInfo(experimentInfo),
      m_detectorInfo(experimentInfo.detectorInfo()),
      m_lastDetector(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

SpectrumInfo::SpectrumInfo(ExperimentInfo &experimentInfo)
    : m_experimentInfo(experimentInfo),
      m_mutableDetectorInfo(&experimentInfo.mutableDetectorInfo()),
      m_detectorInfo(*m_mutableDetectorInfo),
      m_lastDetector(PARALLEL_GET_MAX_THREADS),
      m_lastIndex(PARALLEL_GET_MAX_THREADS, -1) {}

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
  double l2{0.0};
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    l2 += m_detectorInfo.l2(detIndex);
  }
  return l2 / static_cast<double>(dets.size());
}

/** Returns the scattering angle 2 theta (angle w.r.t. to beam direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::twoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  double twoTheta{0.0};
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    twoTheta += m_detectorInfo.twoTheta(detIndex);
  }
  return twoTheta / static_cast<double>(dets.size());
}

/** Returns the signed scattering angle 2 theta (angle w.r.t. to beam
 * direction).
 *
 * Throws an exception if the spectrum is a monitor.
 */
double SpectrumInfo::signedTwoTheta(const size_t index) const {
  if (isMonitor(index))
    throw std::logic_error(
        "Two theta (scattering angle) is not defined for monitors.");

  double signedTwoTheta{0.0};
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    signedTwoTheta += m_detectorInfo.signedTwoTheta(detIndex);
  }
  return signedTwoTheta / static_cast<double>(dets.size());
}

/// Returns the position of the spectrum with given index.
Kernel::V3D SpectrumInfo::position(const size_t index) const {
  Kernel::V3D newPos;
  const auto &dets = getDetectorVector(index);
  for (const auto &det : dets) {
    const auto &detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    newPos += m_detectorInfo.position(detIndex);
  }
  return newPos / static_cast<double>(dets.size());
}

/// Returns true if the spectrum is associated with detectors in the instrument.
bool SpectrumInfo::hasDetectors(const size_t index) const {
  // Workspaces can contain invalid detector IDs. Those IDs will be silently
  // ignored here until this is fixed.
  const auto &validDetectorIDs = m_detectorInfo.detectorIDs();
  for (const auto &id : m_experimentInfo.detectorIDsInGroup(index)) {
    const auto &it = std::lower_bound(validDetectorIDs.cbegin(),
                                      validDetectorIDs.cend(), id);
    if (it != validDetectorIDs.cend() && *it == id) {
      return true;
    }
  }
  return false;
}

/// Returns true if the spectrum is associated with exactly one detector.
bool SpectrumInfo::hasUniqueDetector(const size_t index) const {
  size_t count = 0;
  // Workspaces can contain invalid detector IDs. Those IDs will be silently
  // ignored here until this is fixed.
  const auto &validDetectorIDs = m_detectorInfo.detectorIDs();
  for (const auto &id : m_experimentInfo.detectorIDsInGroup(index)) {
    const auto &it = std::lower_bound(validDetectorIDs.cbegin(),
                                      validDetectorIDs.cend(), id);
    if (it != validDetectorIDs.cend() && *it == id) {
      ++count;
    }
  }
  return count == 1;
}

/** Set the mask flag of the spectrum with given index. Not thread safe.
 *
 * Currently this simply sets the mask flags for the underlying detectors. */
void SpectrumInfo::setMasked(const size_t index, bool masked) {
  for (const auto &det : getDetectorVector(index)) {
    const auto detIndex = m_detectorInfo.indexOf(det->getID());
    m_detectorInfo.setCachedDetector(detIndex, det);
    m_mutableDetectorInfo->setMasked(detIndex, masked);
  }
}

/// Return a const reference to the detector or detector group of the spectrum
/// with given index.
const Geometry::IDetector &SpectrumInfo::detector(const size_t index) const {
  return getDetector(index);
}

/// Returns the source position.
Kernel::V3D SpectrumInfo::sourcePosition() const {
  return m_detectorInfo.sourcePosition();
}

/// Returns the sample position.
Kernel::V3D SpectrumInfo::samplePosition() const {
  return m_detectorInfo.samplePosition();
}

/// Returns L1 (distance from source to sample).
double SpectrumInfo::l1() const { return m_detectorInfo.l1(); }

const Geometry::IDetector &SpectrumInfo::getDetector(const size_t index) const {
  size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
  if (m_lastIndex[thread] == index)
    return *m_lastDetector[thread];

  m_lastIndex[thread] = index;

  // Note: This function body has big overlap with the method
  // MatrixWorkspace::getDetector(). The plan is to eventually remove the
  // latter, once SpectrumInfo is in widespread use.
  const auto &dets = m_experimentInfo.detectorIDsInGroup(index);
  const size_t ndets = dets.size();
  if (ndets == 1) {
    // If only 1 detector for the spectrum number, just return it
    const auto detIndex = m_detectorInfo.indexOf(*dets.begin());
    m_lastDetector[thread] = m_detectorInfo.getDetectorPtr(detIndex);
  } else if (ndets == 0) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getDetector(): No "
                                           "detectors for this workspace "
                                           "index.",
                                           "");
  } else {
    // Else need to construct a DetectorGroup and use that
    std::vector<boost::shared_ptr<const Geometry::IDetector>> det_ptrs;
    for (const auto &id : dets) {
      try {
        const auto detIndex = m_detectorInfo.indexOf(id);
        det_ptrs.push_back(m_detectorInfo.getDetectorPtr(detIndex));
      } catch (std::out_of_range &) {
        // Workspaces can contain invalid detector IDs. Those IDs will be
        // silently ignored here until this is fixed. Some valid IDs will exist
        // if hasDetectors or hasUniqueDetectors has returned true, but there
        // could still be invalid IDs.
      }
    }
    m_lastDetector[thread] =
        boost::make_shared<Geometry::DetectorGroup>(det_ptrs, false);
  }

  return *m_lastDetector[thread];
}

std::vector<Geometry::IDetector_const_sptr>
SpectrumInfo::getDetectorVector(const size_t index) const {
  const auto &det = getDetector(index);
  const auto &ndet = det.nDets();
  if (ndet > 1) {
    const auto group = dynamic_cast<const Geometry::DetectorGroup *>(&det);
    return group->getDetectors();
  } else {
    size_t thread = static_cast<size_t>(PARALLEL_THREAD_NUMBER);
    return {m_lastDetector[thread]};
  }
}

} // namespace API
} // namespace Mantid
