#ifndef MANTID_GEOMETRY_BEAMLINERAYTRACER_H_
#define MANTID_GEOMETRY_BEAMLINERAYTRACER_H_

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <deque>
#include <iterator>
#include <list>

/**
BeamlineRayTracer contains a set of free functions that are responsible for
tracking rays and accumulating a list of objects that are intersected along the
way. These methods have been adapted from the original InstrumentRayTracer.

@author Bhuvan Bezawada, STFC
@date 17/08/2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

namespace Mantid {
namespace Geometry {
namespace BeamlineRayTracer {

using Kernel::Matrix;
using Kernel::Quat;
using Kernel::V3D;
using Links = Track::LType;
using Types = Mantid::Beamline::ComponentType;

namespace IntersectionHelpers {

/**
 * Tests the intersection of the ray with a rectangular bank of detectors.
 * Uses the knowledge of the RectangularDetector shape to significantly speed up
 * tracking.
 *
 * @param track :: Track under test. The results are stored here.
 */
void checkIntersectionWithRectangularBank(Track &track,
                                          const ComponentInfo &componentInfo,
                                          size_t componentIndex) {

  // Base point (x,y,z) = position of pixel 0,0
  V3D basePoint;

  // Vertical (y-axis) basis vector of the detector
  V3D vertical;

  // Horizontal (x-axis) basis vector of the detector
  V3D horizontal;

  // Get the corners of the detector
  auto corners = componentInfo.quadrilateralComponent(componentIndex);

  // Set up locations
  basePoint = componentInfo.position(corners.bottomLeft);
  V3D bottomRight = componentInfo.position(corners.bottomRight);
  V3D topLeft = componentInfo.position(corners.topLeft);
  horizontal = bottomRight - basePoint;
  vertical = topLeft - basePoint;

  // The beam direction
  V3D beam = track.direction();

  // From: http://en.wikipedia.org/wiki/Line-plane_intersection (taken on May 4,
  // 2011),
  // We build a matrix to solve the linear equation:
  Matrix<double> mat(3, 3);
  mat.setColumn(0, beam * -1.0);
  mat.setColumn(1, horizontal);
  mat.setColumn(2, vertical);
  mat.Invert();

  // Multiply by the inverted matrix to find t,u,v
  V3D tuv = mat * (track.startPoint() - basePoint);
  //  std::cout << tuv << "\n";

  // Intersection point
  V3D intersec = beam;
  intersec *= tuv[0];

  // t = coordinate along the line
  // u,v = coordinates along horizontal, vertical
  // (and correct for it being between 0, xpixels-1).  The +0.5 is because the
  // base point is at the CENTER of pixel 0,0.
  double u = (double(corners.nX - 1) * tuv[1] + 0.5);
  double v = (double(corners.nY - 1) * tuv[2] + 0.5);

  // In indices
  int xIndex = int(u);
  int yIndex = int(v);

  // Out of range?
  if (xIndex < 0)
    return;
  if (yIndex < 0)
    return;
  if (xIndex >= corners.nX)
    return;
  if (yIndex >= corners.nY)
    return;

  // Get the componentIndex of the component in the assembly at the (X, Y) pixel
  // position.
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
 * @param componentInfo :: ComponentInfo object to use to help find components.
 * @param componentIndex :: The component to be checked.
 */
void checkIntersectionWithComponent(Track &track,
                                    const ComponentInfo &componentInfo,
                                    size_t componentIndex) {

  // First subtract the component's position, then undo the rotation
  V3D factored = track.startPoint() - componentInfo.position(componentIndex);

  // Get the total rotation of this component and calculate the inverse (reverse
  // rotation)
  Quat unrotate = componentInfo.rotation(componentIndex);
  unrotate.inverse();
  unrotate.rotate(factored);
  V3D trkStart = factored;

  // Get the total rotation of this component and calculate the inverse (reverse
  // rotation)
  V3D direction = track.direction();
  Quat unrotate2 = componentInfo.rotation(componentIndex);
  unrotate2.inverse();
  unrotate2.rotate(direction);
  V3D trkDirection = direction;

  // Create a new track and store number of surface interceptions
  Track probeTrack(trkStart, trkDirection);
  int intercepts =
      componentInfo.shape(componentIndex).interceptSurface(probeTrack);

  // Iterate over track
  Track::LType::const_iterator it;
  for (it = probeTrack.cbegin(); it != probeTrack.cend(); ++it) {
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
} // namespace IntersectionHelpers

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
  // Cast size of componentInfo to int
  int size = static_cast<int>(componentInfo.size());
  --size;

  // Loop through the bounding boxes in reverse
  // (essentially a breadth first search)
  for (int i = size; i >= 0; --i) {
    // Store the bounding box
    BoundingBox box = componentInfo.boundingBox(i);

    // Test for intersection
    if (box.doesLineIntersect(track)) {

      // Store the type of the child component
      auto childComponentType = componentInfo.componentType(i);
      // Store the type of the child's grandparent
      auto grandParent = componentInfo.componentType(
          componentInfo.parent(componentInfo.parent(i)));

      // Don't want to count the bank and the detector
      if (grandParent == Types::Rectangular &&
          childComponentType == Types::Detector) {
        continue;
      }

      // Process a rectangular bank or normal component
      if (childComponentType == Types::Rectangular) {
        IntersectionHelpers::checkIntersectionWithRectangularBank(
            track, componentInfo, i);
      } else {
        IntersectionHelpers::checkIntersectionWithComponent(track,
                                                            componentInfo, i);
      }
    }
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
 * @return size_t index value or throws an error if index is invalid
 */
size_t getDetectorResult(const ComponentInfo &componentInfo,
                         Track &resultsTrack) {
  // Store the results
  Links results = getResults(resultsTrack);

  // Go through all results via an iterator
  Links::const_iterator resultIterator = results.begin();

  // Return the first detectorIndex
  while (resultIterator != results.end()) {
    // Increment the iterator
    ++resultIterator;

    // Get the detector index
    auto index = componentInfo.indexOf(resultIterator->componentID);

    // Validate the index
    if (index >= 0 && index <= componentInfo.size()) {
      return index;
    }

    // If we reach here, the index must be invalid
    throw std::out_of_range("Error, index is invalid!");
  }
}

} // namespace BeamlineRayTracer
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BEAMLINERAYTRACER_H_ */
