#include "MantidAPI/DetectorSearcher.h"
#include "MantidKernel/ConfigService.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

#include <tuple>

using Mantid::Kernel::V3D;
using Mantid::Geometry::InstrumentRayTracer;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::ReferenceFrame;
using namespace Mantid::API;

DetectorSearcher::DetectorSearcher(Geometry::Instrument_const_sptr instrument,
                                   const API::DetectorInfo &detInfo)
    : m_usingFullRayTrace(instrument->containsRectDetectors() ==
                          Geometry::Instrument::ContainsState::Full),
      m_crystallography_convention(Kernel::ConfigService::Instance().getString(
                                       "Q.convention") == "Crystallography"),
      m_detInfo(detInfo), m_instrument(instrument) {
  // choose the search strategy to use
  if (!m_usingFullRayTrace) {
    createDetectorCache();
  } else {
    m_rayTracer = Kernel::make_unique<InstrumentRayTracer>(instrument);
  }
}

void DetectorSearcher::createDetectorCache() {
  std::vector<Eigen::Array3d, Eigen::aligned_allocator<Eigen::Array3d>> points;
  points.reserve(m_detInfo.size());
  m_indexMap.reserve(m_detInfo.size());

  for (size_t pointNo = 0; pointNo < m_detInfo.size(); ++pointNo) {
    if (m_detInfo.isMonitor(pointNo))
      continue; // skip monitor
    if (m_detInfo.isMasked(pointNo))
      continue; // detector is masked so don't use

    // calculate a Q vector for each detector
    // This follows a method similar to that used in IntegrateEllipsoids
    const auto &det = m_detInfo.detector(pointNo);
    const auto tt1 = det.getTwoTheta(V3D(0, 0, 0), V3D(0, 0, 1)); // two theta
    const auto ph1 = det.getPhi();                                // phi
    auto E1 =
        V3D(-std::sin(tt1) * std::cos(ph1), -std::sin(tt1) * std::sin(ph1),
            1. - std::cos(tt1)); // end of trajectory
    E1 = E1 * (1. / E1.norm());  // normalize

    Eigen::Array3d point(E1[0], E1[1], E1[2]);

    // Ignore nonsensical points
    if (point.hasNaN())
      continue;

    points.push_back(point);
    m_indexMap.push_back(pointNo);
  }

  // create KDtree of cached detector Q vectors
  m_detectorCacheSearch =
      Kernel::make_unique<Kernel::NearestNeighbours<3>>(points);
}

const std::tuple<bool, size_t>
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

const std::tuple<bool, size_t>
DetectorSearcher::searchUsingInstrumentRayTracing(const V3D &q) {
  const auto direction = convertQtoDirection(q);
  m_rayTracer->traceFromSample(direction);
  const auto det = m_rayTracer->getDetectorResult();

  if (!det)
    return std::make_tuple(false, 0);

  const auto detIndex = m_detInfo.indexOf(det->getID());
  return std::make_tuple(true, detIndex);
}

const std::tuple<bool, size_t>
DetectorSearcher::searchUsingNearestNeighbours(const V3D &q) {
  const auto detectorDir = convertQtoDirection(q);
  // find where this Q vector should intersect with "extended" space
  const auto neighbours =
      m_detectorCacheSearch->findNearest(Eigen::Array3d(q[0], q[1], q[2]), 5);
  if (neighbours.size() == 0)
    return std::make_tuple(false, 0);

  const auto result = checkInteceptWithNeighbours(detectorDir, neighbours);
  const auto hitDetector = std::get<0>(result);
  const auto index = std::get<1>(result);

  if (hitDetector)
    return std::make_tuple(true, m_indexMap[index]);

  // Tube Gap Parameter specifically applies to CORELLI
  if (!hitDetector && m_instrument->hasParameter("tube-gap")) {
    std::vector<double> gaps =
        m_instrument->getNumberParameter("tube-gap", true);
    if (!gaps.empty()) {
      const auto gap = static_cast<double>(gaps.front());
      // try adding and subtracting tube-gap in 3 q dimensions to see if you can
      // find detectors on each side of tube gap
      for (int i = 0; i < 3; i++) {
        auto gapDir = V3D(0., 0., 0.);
        gapDir[i] = gap;

        auto beam1 = detectorDir + gapDir;
        const auto result1 = checkInteceptWithNeighbours(beam1, neighbours);
        const auto hit1 = std::get<0>(result1);

        auto beam2 = detectorDir - gapDir;
        const auto result2 = checkInteceptWithNeighbours(beam2, neighbours);
        const auto hit2 = std::get<0>(result2);

        if (hit1 && hit2) {
          // Set the detector to one of the neighboring pixels
          return std::make_tuple(true, m_indexMap[std::get<1>(result1)]);
        }
      }
    }
  }

  return std::make_tuple(false, 0);
}

const std::tuple<bool, size_t> DetectorSearcher::checkInteceptWithNeighbours(
    const V3D &direction,
    const Kernel::NearestNeighbours<3>::NearestNeighbourResults &neighbours)
    const {
  Geometry::Track track(m_detInfo.samplePosition(), direction);
  // Find which of the neighbours we actually intersect with
  for (const auto neighbour : neighbours) {
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

V3D DetectorSearcher::convertQtoDirection(const V3D &q) const {
  const auto norm_q = q.norm();
  const auto refFrame = m_instrument->getReferenceFrame();
  const V3D refBeamDir = refFrame->vecPointingAlongBeam();

  auto qSign = (m_crystallography_convention) ? -1.0 : 1.0;
  const double qBeam = q.scalar_prod(refBeamDir) * qSign;
  double one_over_wl = (norm_q * norm_q) / (2.0 * qBeam);

  qSign = (m_crystallography_convention) ? 1.0 : -1.0;
  auto detectorDir = q * qSign;
  detectorDir[refFrame->pointingAlongBeam()] = one_over_wl - qBeam;
  detectorDir.normalize();
  return detectorDir;
}
