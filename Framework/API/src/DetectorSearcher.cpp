// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/DetectorSearcher.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/NearestNeighbours.h"

#include <tuple>

using Mantid::Geometry::InstrumentRayTracer;
using Mantid::Kernel::V3D;
using namespace Mantid;
using namespace Mantid::API;

double getQSign() {
  const auto convention =
      Kernel::ConfigService::Instance().getString("Q.convention");
  return (convention == "Crystallography") ? -1.0 : 1.0;
}

/** Create a new DetectorSearcher for the given instrument
 *
 * The search strategy will be determined in the constructor based on the
 * given instrument geometry
 *
 * @param instrument :: the instrument to find detectors in
 * @param detInfo :: the Geometry::DetectorInfo object for this instrument
 */
DetectorSearcher::DetectorSearcher(Geometry::Instrument_const_sptr instrument,
                                   const Geometry::DetectorInfo &detInfo)
    : m_usingFullRayTrace(instrument->containsRectDetectors() ==
                          Geometry::Instrument::ContainsState::Full),
      m_crystallography_convention(getQSign()), m_detInfo(detInfo),
      m_instrument(instrument) {

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
    m_rayTracer = Kernel::make_unique<InstrumentRayTracer>(instrument);
  }
}

/** Create a NearestNeighbours search tree for the current instrument
 */
void DetectorSearcher::createDetectorCache() {
  std::vector<Eigen::Vector3d> points;
  points.reserve(m_detInfo.size());
  m_indexMap.reserve(m_detInfo.size());

  const auto frame = m_instrument->getReferenceFrame();
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

    points.push_back(point);
    m_indexMap.push_back(pointNo);
  }

  // create KDtree of cached detector Q vectors
  m_detectorCacheSearch =
      Kernel::make_unique<Kernel::NearestNeighbours<3>>(points);
}

/** Find the index of a detector given a vector in Qlab space
 *
 * If no detector is found the first parameter of the returned tuple is false
 *
 * @param q :: the Qlab vector to find a detector for
 * @return tuple with data <detector found, detector index>
 */
DetectorSearcher::DetectorSearchResult
DetectorSearcher::findDetectorIndex(const V3D &q) {
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
DetectorSearcher::DetectorSearchResult
DetectorSearcher::searchUsingInstrumentRayTracing(const V3D &q) {
  const auto direction = convertQtoDirection(q);
  m_rayTracer->traceFromSample(direction);
  const auto det = m_rayTracer->getDetectorResult();

  if (!det)
    return std::make_tuple(false, 0);

  const auto detIndex = m_detInfo.indexOf(det->getID());

  if (m_detInfo.isMasked(detIndex) || m_detInfo.isMonitor(detIndex))
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
DetectorSearcher::DetectorSearchResult
DetectorSearcher::searchUsingNearestNeighbours(const V3D &q) {
  const auto detectorDir = convertQtoDirection(q);
  // find where this Q vector should intersect with "extended" space
  const auto neighbours =
      m_detectorCacheSearch->findNearest(Eigen::Vector3d(q[0], q[1], q[2]), 5);
  if (neighbours.empty())
    return std::make_tuple(false, 0);

  const auto result = checkInteceptWithNeighbours(detectorDir, neighbours);
  const auto hitDetector = std::get<0>(result);
  const auto index = std::get<1>(result);

  if (hitDetector)
    return std::make_tuple(true, m_indexMap[index]);

  // Tube Gap Parameter specifically applies to tube instruments
  if (!hitDetector && m_instrument->hasParameter("tube-gap")) {
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
DetectorSearcher::DetectorSearchResult DetectorSearcher::handleTubeGap(
    const V3D &detectorDir,
    const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours) {
  std::vector<double> gaps = m_instrument->getNumberParameter("tube-gap", true);
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
    const V3D &direction,
    const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours)
    const {
  Geometry::Track track(m_detInfo.samplePosition(), direction);
  // Find which of the neighbours we actually intersect with
  for (const auto &neighbour : neighbours) {
    const auto index = std::get<1>(neighbour);
    const auto &det = m_detInfo.detector(m_indexMap[index]);

    Mantid::Geometry::BoundingBox bb;
    if (!bb.doesLineIntersect(track))
      continue;

    const auto hitDetector = det.interceptSurface(track) > 0;
    if (hitDetector)
      return std::make_tuple(hitDetector, index);

    track.reset(m_detInfo.samplePosition(), direction);
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
  const auto refFrame = m_instrument->getReferenceFrame();
  const V3D refBeamDir = refFrame->vecPointingAlongBeam();

  const double qBeam = q.scalar_prod(refBeamDir) * m_crystallography_convention;
  double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);

  auto detectorDir = q * -m_crystallography_convention;
  detectorDir[refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
  detectorDir.normalize();
  return detectorDir;
}
