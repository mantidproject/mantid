#include "MantidGeometry/Objects/BeamlineRayTracer.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

using Kernel::V3D;
using Links = Track::LType;

// Anonymous namespace
// Contains helper functions
namespace {
using Mantid::Kernel::Matrix;
using Mantid::Kernel::Quat;
using Types = Mantid::Beamline::ComponentType;

/**
 * Tests the intersection of the ray with a rectangular bank of detectors.
 * Uses the knowledge of the RectangularDetector shape to significantly speed
 * up tracking.
 *
 * @param track :: Track under test. The results are stored here.
 * @param componentInfo :: The object that will provide access to component
 * information.
 * @param componentIndex :: The component to be checked.
 */
void checkIntersectionWithRectangularBank(Track &track,
                                          const ComponentInfo &componentInfo,
                                          size_t componentIndex) {

  // Get the corners of the bank
  auto corners = componentInfo.quadrilateralComponent(componentIndex);

  // Store points of the bank
  V3D bottomLeft = componentInfo.position(corners.bottomLeft);
  V3D bottomRight = componentInfo.position(corners.bottomRight);
  V3D topLeft = componentInfo.position(corners.topLeft);

  // Horizontal (x-axis) basis vector of the detector
  V3D horizontal = bottomRight - bottomLeft;

  // Vertical (y-axis) basis vector of the detector
  V3D vertical = topLeft - bottomLeft;

  // The beam direction
  V3D beam = track.direction();

  // From: http://en.wikipedia.org/wiki/Line-plane_intersection (taken on May
  // 4, 2011), We build a matrix to solve the linear equation:
  Matrix<double> mat(3, 3);
  mat.setColumn(0, beam * -1.0);
  mat.setColumn(1, horizontal);
  mat.setColumn(2, vertical);
  mat.Invert();

  // Multiply by the inverted matrix to find t,u,v
  V3D tuv = mat * (track.startPoint() - bottomLeft);

  // Intersection point
  V3D intersec = beam;
  intersec *= tuv[0];

  // t = coordinate along the line
  // u,v = coordinates along horizontal, vertical
  // (and correct for it being between 0, xpixels-1).  The +0.5 is because the
  // bottomLeft is at the CENTER of pixel 0,0.
  double u = (double(corners.nX - 1) * tuv[1] + 0.5);
  double v = (double(corners.nY - 1) * tuv[2] + 0.5);

  // In indices
  size_t xIndex = size_t(u);
  size_t yIndex = size_t(v);

  // Out of range?
  if (xIndex >= corners.nX)
    return;
  if (yIndex >= corners.nY)
    return;

  // Get the componentIndex of the component in the assembly at the (X, Y)
  // pixel position.
  auto childrenX = componentInfo.children(componentIndex);
  auto childrenY = componentInfo.children(childrenX[xIndex]);

  // Create a link and add it to the track
  track.addLink(intersec, intersec, 0.0, componentInfo.shape(childrenY[yIndex]),
                componentInfo.componentID(childrenY[yIndex])->getComponentID());
}

/**
 * Checks whether the track given will pass through the given Component.
 *
 * @param track :: Track under test. The results are stored here.
 * @param componentInfo :: ComponentInfo object to use to help find
 * components.
 * @param componentIndex :: The component to be checked.
 */
void checkIntersectionWithComponent(Track &track,
                                    const ComponentInfo &componentInfo,
                                    size_t componentIndex) {

  // First subtract the component's position, then undo the rotation
  V3D factored = track.startPoint() - componentInfo.position(componentIndex);

  // Get the total rotation of this component and calculate the inverse
  // (reverse rotation)
  Quat unrotate = componentInfo.rotation(componentIndex);
  unrotate.inverse();
  unrotate.rotate(factored);
  V3D trkStart = factored;

  // Get the total rotation of this component and calculate the inverse
  // (reverse rotation)
  V3D direction = track.direction();
  Quat unrotate2 = componentInfo.rotation(componentIndex);
  unrotate2.inverse();
  unrotate2.rotate(direction);
  V3D trkDirection = direction;

  // Create a new track and store number of surface interceptions
  Track probeTrack(trkStart, trkDirection);

  // Returns an int but we don't need to store it
  componentInfo.shape(componentIndex).interceptSurface(probeTrack);

  // Iterate over track
  for (auto it = probeTrack.cbegin(); it != probeTrack.cend(); ++it) {
    // Starting position
    V3D in = it->entryPoint;
    componentInfo.rotation(componentIndex).rotate(in);

    // Use the scale factor
    in *= componentInfo.scaleFactor(componentIndex);
    in += componentInfo.position(componentIndex);
    V3D out = it->exitPoint;
    componentInfo.rotation(componentIndex).rotate(out);

    // Use the scale factor
    out *= componentInfo.scaleFactor(componentIndex);
    out += componentInfo.position(componentIndex);

    // Create a link and add it to the track
    track.addLink(in, out, out.distance(track.startPoint()),
                  componentInfo.shape(componentIndex),
                  componentInfo.componentID(componentIndex)->getComponentID());
  }
}

/**
 * Return the results of any trace() calls since the last call to getResults.
 *
 * @return A collection of links defining intersection information
 */
Links getResults(Track &resultsTrack) {
  // Create a list of the results
  Links results(resultsTrack.cbegin(), resultsTrack.cend());

  // Clear intersection results
  resultsTrack.clearIntersectionResults();

  return results;
}

/**
 * Add a link to the track for the intersected component
 *
 * @param track :: a track to add a link to for the intersected component
 * @param componentInfo :: a ComponentInfo instance containing the intersected component
 * @param index :: the index of the interesected component
 * @return whether the intersected component has children that need to be searched.
 */
bool addLink(Track &track, const ComponentInfo &componentInfo, const size_t index) {
    // Store the type of the child component
    auto componentType = componentInfo.componentType(index);

    // Process a rectangular bank or normal component
    if (componentType == Types::Rectangular) {
        checkIntersectionWithRectangularBank(track, componentInfo, index);
        return false; // Rectangular panel: we don't need to look at any children
    } else {
        checkIntersectionWithComponent(track, componentInfo, index);
        return true; // We don't know what the children of this are
    }
}

/**
 * Recursively search for the intersection of the component.
 *
 * @param track :: a track to add a links to for any intersected component
 * @param componentInfo :: a ComponentInfo instance containing components to search for intersection with the test ray.
 * @param children :: the children of the previous component to search for intersections.
 */
void fireRayInner(Track &track, const ComponentInfo &componentInfo, const std::vector<size_t>& children) {
    for (const auto& index : children) {
        auto boundingBox = componentInfo.boundingBox(index);
        if (boundingBox.doesLineIntersect(track)) {
            // add link to this component
            const auto lookAtChildren = addLink(track, componentInfo, index);
            if (lookAtChildren) {
                const auto& nextChildren = componentInfo.children(index);
                fireRayInner(track, componentInfo, nextChildren);
            }
        }
    }
}

/**
 * Fire the test ray at the instrument and perform a bread-first search of the
 * object tree to find the objects that were intersected.
 *
 * @param testRay :: An input/output parameter that defines the track and
 * accumulates the intersection results.
 * @param componentInfo :: The object that will provide access to component
 * information.
 */
void fireRay(Track &track, const ComponentInfo &componentInfo) {
    const auto root = componentInfo.root();
    const auto& children = componentInfo.children(root);
    fireRayInner(track, componentInfo, children);
}

} // namespace

namespace BeamlineRayTracer {

/**
 * Trace a given track from the source in the given direction.
 *
 * @param dir :: A directional vector. The starting point is defined by the
 * instrument source.
 * @param componentInfo :: The object used to access the source position.
 * @return Links :: A collection of links defining intersection information.
 */
Links traceFromSource(const Kernel::V3D &dir,
                      const ComponentInfo &componentInfo) {

  // Create a results track
  Track resultsTrack(componentInfo.sourcePosition(), dir);

  // Fire the test ray at the instrument
  fireRay(resultsTrack, componentInfo);

  return getResults(resultsTrack);
}

/**
 * Trace a given track from the sample position in the given direction.
 *
 * @param dir :: A directional vector. The starting point is defined by the
 * source.
 * @param componentInfo :: The object used to access the source position.
 * @return Links :: A collection of links defining intersection information.
 */
Links traceFromSample(const Kernel::V3D &dir,
                      const ComponentInfo &componentInfo) {

  // Create a new results track
  Track resultsTrack(componentInfo.samplePosition(), dir);

  // Fire the test ray at the instrument
  fireRay(resultsTrack, componentInfo);

  return getResults(resultsTrack);
}

/**
 * Gets the results of the trace, then returns the first detector
 * index found in the results.
 *
 * @param componentInfo :: The object used to access the source position.
 * @param detectorInfo :: The object used to check if a component is a monitor.
 * @param resultsTrack :: Track under test. The results are stored here.
 * @return size_t index value or throws an error if index is invalid
 */
size_t getDetectorResult(const ComponentInfo &componentInfo,
                         const DetectorInfo &detectorInfo,
                         Track &resultsTrack) {

  for (auto result : getResults(resultsTrack)) {
    auto component = componentInfo.indexOf(result.componentID);
    if (componentInfo.isDetector(component)) {
      if (!detectorInfo.isMonitor(component)) {
        return component;
      }
    }
  }

  // Return an invalid index
  // If no detector is found
  return -1;
}

} // namespace BeamlineRayTracer

} // namespace Geometry
} // namespace Mantid
