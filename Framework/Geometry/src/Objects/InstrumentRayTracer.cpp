// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
#include "MantidGeometry/Objects/InstrumentRayTracer.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <deque>
#include <iterator>

namespace Mantid::Geometry {

using Kernel::V3D;

//-------------------------------------------------------------
// Public member functions
//-------------------------------------------------------------

/**
 * Constructor specifying the instrument involved in the tracing, which must
 * have a defined source.
 * @param componentInfo :: The ComponentInfo of the instrument to perform the
 * ray tracings on. It must outlive this object.
 * @param detectorInfo :: The DetectorInfo of the same instrument. It must
 * outlive this object.
 */
InstrumentRayTracer::InstrumentRayTracer(ComponentInfo const &componentInfo, DetectorInfo const &detectorInfo)
    : m_componentInfo(componentInfo), m_detectorInfo(detectorInfo) {
  if (!m_componentInfo.hasSource()) {
    std::string errorMsg = "Cannot create InstrumentRayTracer, instrument has "
                           "no defined source.\n";
    throw std::invalid_argument(errorMsg);
  }
}

/**
 * Trace a given track from the instrument source in the given direction. For
 * performance reasons the
 * results are accumulated within the object and can be returned using
 * getResults.
 * @param dir :: A directional vector. The starting point is defined by
 * the instrument source.
 */
void InstrumentRayTracer::trace(const V3D &dir) const {
  // Define the track with the source position and the given direction.
  m_resultsTrack.reset(m_componentInfo.sourcePosition(), dir);
  // The intersection results are accumulated within the ray object
  fireRay(m_resultsTrack);
}

/**
 * Trace a given track from the sample position in the given direction. For
 * performance reasons the
 * results are accumulated within the object and can be returned using
 * getResults.
 * @param dir :: A directional vector. The starting point is defined by
 * the instrument source.
 */
void InstrumentRayTracer::traceFromSample(const V3D &dir) const {
  // Define the track with the sample position and the given direction.
  m_resultsTrack.reset(m_componentInfo.samplePosition(), dir);
  // The intersection results are accumulated within the ray object
  fireRay(m_resultsTrack);
}

/**
 * Return the results of any trace() calls since the last call the getResults.
 * @returns A collection of links defining intersection information
 */
Links InstrumentRayTracer::getResults() const {
  Links results(m_resultsTrack.cbegin(), m_resultsTrack.cend());
  m_resultsTrack.clearIntersectionResults();
  return results;
}

//----------------------------------------------------------------------------------
/** Gets the results of the trace, then returns the index of the first detector
 * (that is NOT a monitor) found in the results.
 * @return the index of the detector in detectorInfo(), or
 * ComponentInfo::invalidIndex if not found
 */
size_t InstrumentRayTracer::getDetectorResult() const {
  Links const results = this->getResults();
  auto const &compInfo = m_componentInfo;
  auto const &detInfo = m_detectorInfo;

  for (auto const &result : results) {
    size_t const index = compInfo.indexOf(result.componentID);
    if (compInfo.isDetector(index) && !detInfo.isMonitor(index))
      return index;
  }
  return ComponentInfo::invalidIndex;
}

/// @return the ComponentInfo for the traced instrument
const ComponentInfo &InstrumentRayTracer::componentInfo() const { return m_componentInfo; }

/// @return the DetectorInfo for the traced instrument
const DetectorInfo &InstrumentRayTracer::detectorInfo() const { return m_detectorInfo; }

//-------------------------------------------------------------
// Private member functions
//-------------------------------------------------------------

/**
 * Fire the test ray at the instrument and perform a breadth-first search of the
 * ComponentInfo tree to find the components that were intersected.
 * @param testRay :: An input/output parameter that defines the track and
 * accumulates the
 *        intersection results
 */
void InstrumentRayTracer::fireRay(Track &testRay) const {
  auto const &compInfo = m_componentInfo;
  // Go through the instrument tree and see if we get any hits by
  // (a) first testing the bounding box and if we're inside that then
  // (b) test the lower components.
  std::deque<size_t> nodeQueue;

  // Start at the root of the tree
  nodeQueue.emplace_back(compInfo.root());

  while (!nodeQueue.empty()) {
    size_t const node = nodeQueue.front();
    nodeQueue.pop_front();
    BoundingBox bbox;
    auto it = m_boxCache.find(node);
    if (it != m_boxCache.end()) {
      bbox = it->second;
    } else {
      bbox = compInfo.boundingBox(node);
      std::lock_guard<std::mutex> lock(m_mutex);
      m_boxCache[node] = bbox;
    }

    // Quick test. If this suceeds moved on to test the children
    if (!bbox.doesLineIntersect(testRay))
      continue;
    if (compInfo.isGridDetector(node)) {
      testIntersectionWithGrid(node, testRay);
      continue;
    }
    for (auto const child : compInfo.children(node)) {
      if (!compInfo.children(child).empty())
        nodeQueue.emplace_back(child);
      else if (compInfo.hasValidShape(child))
        testIntersectionWithShape(child, testRay);
    }
  }
}

/**
 * Intersect the track with the shape of a leaf component, adding a link to the
 * track for each intersection.
 * @param index :: The component index of the leaf component
 * @param testRay :: The track to test, which accumulates the results
 */
void InstrumentRayTracer::testIntersectionWithShape(size_t const index, Track &testRay) const {
  auto const &compInfo = m_componentInfo;
  auto const &shape = compInfo.shape(index);
  V3D const pos = compInfo.position(index);
  Kernel::Quat const rot = compInfo.rotation(index);
  Kernel::Quat unRotate = rot;
  unRotate.inverse();
  V3D const scale = compInfo.scaleFactor(index);

  // Transform the track into the frame of the shape. As in
  // ObjComponent::interceptSurface, scaling is not factored out here.
  V3D trkStart = testRay.startPoint() - pos;
  unRotate.rotate(trkStart);
  V3D trkDirection = testRay.direction();
  unRotate.rotate(trkDirection);

  Track probeTrack(trkStart, trkDirection);
  shape.interceptSurface(probeTrack);

  // ComponentID is only used as a handle, so this is never written through
  auto *const compID = const_cast<IComponent *>(compInfo.componentID(index));
  for (auto const &link : probeTrack) {
    V3D in = link.entryPoint;
    rot.rotate(in);
    in *= scale;
    in += pos;
    V3D out = link.exitPoint;
    rot.rotate(out);
    out *= scale;
    out += pos;
    testRay.addLink(in, out, out.distance(testRay.startPoint()), shape, compID);
  }
}

/**
 * Intersect the track with the first layer of a Rectangular or Grid bank by
 * treating it as a plane, adding a link to the track for the pixel hit.
 * @param index :: The component index of the bank
 * @param testRay :: The track to test, which accumulates the results
 */
void InstrumentRayTracer::testIntersectionWithGrid(size_t const index, Track &testRay) const {
  auto const &compInfo = m_componentInfo;
  auto const grid = compInfo.pixelGridComponent(index);

  /// Base point (x,y,z) = position of pixel 0,0
  V3D const basePoint = compInfo.position(compInfo.detectorIndexAtXYZ(index, 0, 0, 0));

  /// Vertical (y-axis) basis vector of the detector
  V3D const vertical = compInfo.position(compInfo.detectorIndexAtXYZ(index, 0, grid.nY - 1, 0)) - basePoint;

  /// Horizontal (x-axis) basis vector of the detector
  V3D const horizontal = compInfo.position(compInfo.detectorIndexAtXYZ(index, grid.nX - 1, 0, 0)) - basePoint;

  // The beam direction
  V3D const beam = testRay.direction();

  // From: http://en.wikipedia.org/wiki/Line-plane_intersection (taken on May 4,
  // 2011),
  // We build a matrix to solve the linear equation:
  Kernel::Matrix<double> mat(3, 3);
  mat.setColumn(0, beam * -1.0);
  mat.setColumn(1, horizontal);
  mat.setColumn(2, vertical);
  mat.Invert();

  // Multiply by the inverted matrix to find t,u,v
  V3D const tuv = mat * (testRay.startPoint() - basePoint);

  // Intersection point
  V3D const intersec = beam * tuv[0];

  // t = coordinate along the line
  // u,v = coordinates along horizontal, vertical
  // (and correct for it being between 0, nX-1).  The +0.5 is because the
  // base point is at the CENTER of pixel 0,0.
  auto const xIndex = int(double(grid.nX - 1) * tuv[1] + 0.5);
  auto const yIndex = int(double(grid.nY - 1) * tuv[2] + 0.5);

  // Out of range?
  if (xIndex < 0 || yIndex < 0 || xIndex >= grid.nX || yIndex >= grid.nY)
    return;

  size_t const pixel = compInfo.detectorIndexAtXYZ(index, xIndex, yIndex, 0);
  // ComponentID is only used as a handle, so this is never written through
  auto *const compID = const_cast<IComponent *>(compInfo.componentID(pixel));
  testRay.addLink(intersec, intersec, 0.0, compInfo.shape(pixel), compID);
}

///**
// * Perform a quick check as to whether the ray passes through the component
// * @param component :: The test component
// */
// bool InstrumentRayTracer::quickIntersectCheck(std::shared_ptr<IComponent>
// component, const Track & testRay) const
// {
//
// }
//
// /**
//  * Perform a proper intersection test of the physical object and accumulate
//  the results if necessary
//  * @param testRay :: An input/output parameter that defines the track and
//  accumulates the
//  * intersection results
//  */
// void slowIntersectCheck(std::shared_ptr<IComponent> component, Track &
// testRay) const;
} // namespace Mantid::Geometry
