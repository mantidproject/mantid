#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_

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
InstrumentRayTracer2 contains a set of free functions that are responsible for
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
namespace InstrumentRayTracer2 {

using Kernel::Matrix;
using Kernel::Quat;
using Kernel::V3D;
using Links = Track::LType;
using Types = Mantid::Beamline::ComponentType;

namespace IntersectionHelpers {

void checkIntersectsWithRectangularBank(Track &testRay,
                                        const ComponentInfo &componentInfo,
                                        size_t componentIndex) {
  /// Base point (x,y,z) = position of pixel 0,0
  V3D basePoint;

  /// Vertical (y-axis) basis vector of the detector
  V3D vertical;

  /// Horizontal (x-axis) basis vector of the detector
  V3D horizontal;

  // Get the corners of the detector
  auto corners = componentInfo.quadrilateralComponent(componentIndex);

  basePoint = componentInfo.position(corners.bottomLeft);

  V3D bottomRight = componentInfo.position(corners.bottomRight);
  V3D topLeft = componentInfo.position(corners.topLeft);

  horizontal = bottomRight - basePoint;
  vertical = topLeft - basePoint;

  std::cout << "BASEPOINT : " << basePoint << std::endl;
  std::cout << "HORIZONTAL : " << horizontal << std::endl;
  std::cout << "VERTICAL : " << vertical << std::endl;
  std::cout << "xpixels : " << corners.nX << std::endl;
  std::cout << "ypixels : " << corners.nY << std::endl;

  // The beam direction
  V3D beam = testRay.direction();

  // From: http://en.wikipedia.org/wiki/Line-plane_intersection (taken on May 4,
  // 2011),
  // We build a matrix to solve the linear equation:
  Matrix<double> mat(3, 3);
  mat.setColumn(0, beam * -1.0);
  mat.setColumn(1, horizontal);
  mat.setColumn(2, vertical);
  mat.Invert();

  // Multiply by the inverted matrix to find t,u,v
  V3D tuv = mat * (testRay.startPoint() - basePoint);
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

  std::cout << "\n U V Values: " << u << ", " << v << "\n";

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

  // TODO: Do I need to put something smart here for the first 3 parameters?
  // componentInfo.componentID();
  // componentInfo.shape();

  //auto comp = getAtXY(xIndex, yIndex);
  auto childrenX = componentInfo.children(componentIndex);
  auto childrenY = componentInfo.children(childrenX[xIndex]);

  testRay.addLink(intersec, intersec, 0.0, componentInfo.shape(childrenY[yIndex]),
                  componentInfo.componentID(childrenY[yIndex])->getComponentID());

  std::cout << "xIndex: " << xIndex << std::endl;
  std::cout << "yIndex: " << yIndex << std::endl;

  std::cout << "Number of Links RD: " << testRay.count() << std::endl;
}

void interceptSurface(Track &track, const ComponentInfo &componentInfo,
                      size_t componentIndex) {

  // TODO: If scaling parameters are ever enabled, would they need need to be
  // used here?
  V3D factored = track.startPoint() - componentInfo.position(componentIndex);
  Quat unrotate = componentInfo.rotation(componentIndex);
  unrotate.inverse();
  unrotate.rotate(factored);
  V3D trkStart = factored;

  V3D factored2 = track.direction();
  Quat unrotate2 = componentInfo.rotation(componentIndex);
  unrotate2.inverse();
  unrotate2.rotate(factored2);
  V3D trkDirection = factored2;

  Track probeTrack(trkStart, trkDirection);
  int intercepts =
      componentInfo.shape(componentIndex).interceptSurface(probeTrack);

  Track::LType::const_iterator it;
  for (it = probeTrack.cbegin(); it != probeTrack.cend(); ++it) {
    V3D in = it->entryPoint;
    componentInfo.rotation(componentIndex).rotate(in);

    // use the scale factor
    in *= componentInfo.scaleFactor(componentIndex);
    in += componentInfo.position(componentIndex);
    V3D out = it->exitPoint;
    componentInfo.rotation(componentIndex).rotate(out);

    // use the scale factor
    out *= componentInfo.scaleFactor(componentIndex);
    out += componentInfo.position(componentIndex);
    track.addLink(in, out, out.distance(track.startPoint()),
                  componentInfo.shape(componentIndex),
                  componentInfo.componentID(componentIndex)->getComponentID());
  }

  std::cout << "Number of Links OC: " << track.count() << std::endl;
}

} // namespace IntersectionHelpers


/**
 * Fire the test ray at the instrument and perform a bread-first search of the
 * object tree to find the objects that were intersected.
 * @param componentInfo :: The object that will provide access to the bounding
 * boxes
 * @param testRay :: An input/output parameter that defines the track and
 * accumulates the intersection results
 */
void fireRay(Track &testRay, const ComponentInfo &componentInfo) {

  // Cast size of componentInfo to int
  int size = static_cast<int>(componentInfo.size());
  --size;

  // Loop through the bounding boxes in reverse
  // (essentially a breadth first search)
  for (int i = size; i >= 0; --i) {
    // Store the bounding box
    BoundingBox box = componentInfo.boundingBox(i);

    // Test for intersection
    if (box.doesLineIntersect(testRay)) {

      std::cout << "IRT Component Name: "
                << componentInfo.componentID(i)->getFullName() << std::endl;

      auto childComponentType = componentInfo.componentType(i);
      auto grandParent = componentInfo.componentType(componentInfo.parent(componentInfo.parent(i)));

      if (childComponentType == Types::Detector && grandParent == Types::Rectangular) {
        continue;
      }

      if (childComponentType == Types::Rectangular) {
        IntersectionHelpers::checkIntersectsWithRectangularBank(
            testRay, componentInfo, i);
      } else {
        IntersectionHelpers::interceptSurface(testRay, componentInfo, i);
      }
    }
  }
}

/**
 * Return the results of any trace() calls since the last call to getResults.
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

} // namespace InstrumentRayTracer2
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_ */
