// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/DetectorSearcher.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/NearestNeighbours.h"

#include <queue>
#include <tuple>

using Mantid::Geometry::InstrumentRayTracer;
using Mantid::Kernel::V3D;
using namespace Mantid;
using namespace Mantid::API;

namespace {
Kernel::Logger g_log("DetectorSearcher");

/** Check whether all of the detectors in the instrument are in rectangular
 * banks. Mirrors Instrument::containsRectDetectors, ignoring the source,
 * sample, monitors, choppers, slits and supermirrors.
 *
 * @param compInfo :: the Geometry::ComponentInfo object for the instrument
 * @param detInfo :: the Geometry::DetectorInfo object for the instrument
 * @return true if only rectangular banks were found
 */
bool containsOnlyRectDetectors(const Geometry::ComponentInfo &compInfo, const Geometry::DetectorInfo &detInfo) {
  using Beamline::ComponentType;
  std::queue<size_t> compQueue;
  for (auto const child : compInfo.children(compInfo.root()))
    compQueue.push(child);

  bool foundRect = false;
  while (!compQueue.empty()) {
    size_t const index = compQueue.front();
    compQueue.pop();

    if ((compInfo.hasSource() && index == compInfo.source()) || (compInfo.hasSample() && index == compInfo.sample()))
      continue;
    if (compInfo.isDetector(index) && detInfo.isMonitor(index))
      continue;
    // skip choppers, slits and supermirrors - HACK!
    auto const &name = compInfo.name(index);
    if (name == "chopper-position" || name.substr(0, 4) == "slit" || name == "supermirror")
      continue;

    auto const type = compInfo.componentType(index);
    if (type == ComponentType::Rectangular) {
      foundRect = true;
    } else if (type == ComponentType::Unstructured || type == ComponentType::Grid ||
               type == ComponentType::Structured || type == ComponentType::OutlineComposite) {
      // An assembly, so check its children
      for (auto const child : compInfo.children(index))
        compQueue.push(child);
    } else {
      // Found a non-rectangular detector or component
      return false;
    }
  }
  return foundRect;
}
} // namespace

double getQSign() {
  const auto convention = Kernel::ConfigService::Instance().getString("Q.convention");
  return (convention == "Crystallography") ? -1.0 : 1.0;
}

/** Create a new DetectorSearcher for the given instrument
 *
 * The search strategy will be determined in the constructor based on the
 * given instrument geometry
 *
 * @param compInfo :: the Geometry::ComponentInfo object for this instrument
 * @param detInfo :: the Geometry::DetectorInfo object for this instrument
 */
DetectorSearcher::DetectorSearcher(const Geometry::ComponentInfo &compInfo, const Geometry::DetectorInfo &detInfo)
    : m_usingFullRayTrace(containsOnlyRectDetectors(compInfo, detInfo)), m_crystallography_convention(getQSign()),
      m_compInfo(compInfo), m_detInfo(detInfo) {

  /* Choose the search strategy to use
   * If the instrument uses rectangular detectors (e.g. TOPAZ) then it is faster
   * to run a full ray trace starting from the top of the instrument. This is
   * due to the speed up of looking up a single pixel in the rectangular
   * detector.
   *
   * If the instrument does not use rectangular detectors (e.g. WISH, CORELLI)
   * then it is faster to use a nearest neighbour search to find the closest
   * pixels, then check them for intersection.
   * */
  if (!m_usingFullRayTrace) {
    createDetectorCache();
  } else {
    m_rayTracer = std::make_unique<InstrumentRayTracer>(compInfo, detInfo);
  }
}

/** Create a NearestNeighbours search tree for the current instrument
 */
void DetectorSearcher::createDetectorCache() {
  std::vector<Eigen::Vector3d> points;
  points.reserve(m_detInfo.size());
  m_indexMap.reserve(m_detInfo.size());

  const auto frame = m_compInfo.referenceFrame();
  auto beam = frame->vecPointingAlongBeam();
  auto up = frame->vecPointingUp();

  for (size_t pointNo = 0; pointNo < m_detInfo.size(); ++pointNo) {
    if (m_detInfo.isMonitor(pointNo) || m_detInfo.isMasked(pointNo))
      continue; // detector is a monitor or masked so don't use

    // Calculate a unit Q vector for each detector
    // This follows a method similar to that used in IntegrateEllipsoids
    const auto pos = normalize(m_detInfo.position(pointNo));
    auto E1 = (pos - beam) * -m_crystallography_convention;
    const auto norm = E1.norm();
    if (norm == 0.) {
      E1 = V3D(up) * -m_crystallography_convention;
    } else {
      E1 /= norm;
    }

    Eigen::Vector3d point(E1[0], E1[1], E1[2]);

    // Ignore nonsensical points
    if (point.hasNaN() || up.coLinear(beam, pos))
      continue;

    points.emplace_back(point);
    m_indexMap.emplace_back(pointNo);
  }

  // create KDtree of cached detector Q vectors
  m_detectorCacheSearch = std::make_unique<Kernel::NearestNeighbours<3>>(points);
}

/** Find the index of a detector given a vector in Qlab space
 *
 * If no detector is found the first parameter of the returned tuple is false
 *
 * @param q :: the Qlab vector to find a detector for
 * @return tuple with data <detector found, detector index>
 */
DetectorSearcher::DetectorSearchResult DetectorSearcher::findDetectorIndex(const V3D &q) {
  // quick check to see if this Q is valid
  if (q.nullVector())
    return std::make_tuple(false, 0);

  // search using best strategy for current instrument
  if (m_usingFullRayTrace) {
    return searchUsingInstrumentRayTracing(q);
  } else {
    return searchUsingNearestNeighbours(q);
  }
}

/** Find the index of a detector given a vector in Qlab space using a ray
 * tracing search strategy
 *
 * If no detector is found the first parameter of the returned tuple is false
 *
 * @param q :: the Qlab vector to find a detector for
 * @return tuple with data <detector found, detector index>
 */
DetectorSearcher::DetectorSearchResult DetectorSearcher::searchUsingInstrumentRayTracing(const V3D &q) {
  const auto direction = convertQtoDirection(q);
  m_rayTracer->traceFromSample(direction);
  auto const detIndex = m_rayTracer->getDetectorResult();

  if (detIndex == Geometry::ComponentInfo::invalidIndex || m_detInfo.isMasked(detIndex))
    return std::make_tuple(false, 0);

  return std::make_tuple(true, detIndex);
}

/** Find the index of a detector given a vector in Qlab space using a nearest
 * neighbours search strategy
 *
 * If no detector is found the first parameter of the returned tuple is false
 *
 * @param q :: the Qlab vector to find a detector for
 * @return tuple with data <detector found, detector index>
 */
DetectorSearcher::DetectorSearchResult DetectorSearcher::searchUsingNearestNeighbours(const V3D &q) {
  const auto detectorDir = convertQtoDirection(q);
  // find where this Q vector should intersect with "extended" space
  // NOTE: increase the Neighbors from 11 to 21 to cover a wide extended space
  const auto neighbours = m_detectorCacheSearch->findNearest(Eigen::Vector3d(q[0], q[1], q[2]), 21);

  // check if neighboring is empty
  if (neighbours.empty()) {
    g_log.information() << "empty neighbor list for given Q\n";
    return std::make_tuple(false, 0);
  }

  const auto result = checkInteceptWithNeighbours(detectorDir, neighbours);
  const auto hitDetector = std::get<0>(result);
  const auto index = std::get<1>(result);

  if (hitDetector) {
    return std::make_tuple(true, m_indexMap[index]);
  } else if (m_compInfo.hasParameter(m_compInfo.root(), "tube-gap")) {
    // Tube Gap Parameter specifically applies to tube instruments
    return handleTubeGap(detectorDir, neighbours);
  }

  return std::make_tuple(false, 0);
}

/** Handle the tube-gap parameter in tube based instruments.
 *
 * This will check for interceptions with the nearest neighbours by "wiggling"
 * the predicted detector direction slightly.
 *
 * @param detectorDir :: the predicted direction towards a detector
 * @param neighbours :: the NearestNeighbour results to check interception with
 * @return a detector search result with whether a detector was hit
 */
DetectorSearcher::DetectorSearchResult
DetectorSearcher::handleTubeGap(const V3D &detectorDir,
                                const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours) {
  std::vector<double> gaps = m_compInfo.getNumberParameter(m_compInfo.root(), "tube-gap", true);
  if (!gaps.empty()) {
    const auto gap = static_cast<double>(gaps.front());
    // try adding and subtracting tube-gap in 3 q dimensions to see if you can
    // find detectors on each side of tube gap
    for (int i = 0; i < 3; i++) {
      auto gapDir = V3D(0., 0., 0.);
      gapDir[i] = gap;

      auto beam1 = normalize(detectorDir + gapDir);
      const auto result1 = checkInteceptWithNeighbours(beam1, neighbours);
      const auto hit1 = std::get<0>(result1);

      const auto beam2 = normalize(detectorDir - gapDir);
      const auto result2 = checkInteceptWithNeighbours(beam2, neighbours);
      const auto hit2 = std::get<0>(result2);

      if (hit1 && hit2) {
        // Set the detector to one of the neighboring pixels
        return std::make_tuple(true, m_indexMap[std::get<1>(result1)]);
      }
    }
  }

  return std::make_tuple(false, 0);
}

/** Check whether the given direction in real space intersects with any of the
 * k nearest neighbours
 *
 * @param direction :: real space direction vector
 * @param neighbours :: vector of nearest neighbours to check
 * @return tuple of <detector hit, index of correct index in m_IndexMap>
 */
std::tuple<bool, size_t> DetectorSearcher::checkInteceptWithNeighbours(
    const V3D &direction, const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours) const {
  Geometry::Track track(m_detInfo.samplePosition(), direction);
  // Find which of the neighbours we actually intersect with
  for (const auto &neighbour : neighbours) {
    const auto index = std::get<1>(neighbour);
    const auto detIndex = m_indexMap[index];

    Mantid::Geometry::BoundingBox bb;
    if (!bb.doesLineIntersect(track))
      continue;

    if (!m_compInfo.hasValidShape(detIndex))
      throw Kernel::Exception::NullPointerException("DetectorSearcher::checkInteceptWithNeighbours", "shape");

    // Express the track in the detector shape's frame: remove the detector position and rotation
    auto unRotate = m_detInfo.rotation(detIndex);
    unRotate.inverse();
    auto trackStart = track.startPoint() - m_detInfo.position(detIndex);
    unRotate.rotate(trackStart);
    auto trackDirection = track.direction();
    unRotate.rotate(trackDirection);
    Geometry::Track probeTrack(trackStart, trackDirection);

    const auto hitDetector = m_compInfo.shape(detIndex).interceptSurface(probeTrack) > 0;
    if (hitDetector)
      return std::make_tuple(hitDetector, index);
  }

  return std::make_tuple(false, 0);
}

/** Helper method to convert a vector in Qlab to a direction in detector space
 *
 * @param q :: a Qlab vector
 * @return a direction in detector space
 */
V3D DetectorSearcher::convertQtoDirection(const V3D &q) const {
  const auto norm_q = q.norm();
  const auto refFrame = m_compInfo.referenceFrame();
  const V3D refBeamDir = refFrame->vecPointingAlongBeam();

  const double qBeam = q.scalar_prod(refBeamDir) * m_crystallography_convention;
  double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);

  auto detectorDir = q * -m_crystallography_convention;
  detectorDir[refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
  detectorDir.normalize();
  return detectorDir;
}
