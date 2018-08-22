#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/V3D.h"

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

using Kernel::V3D;
using Links = Track::LType;
using Types = Mantid::Beamline::ComponentType;

/**
 * Fire the test ray at the instrument and perform a bread-first search of the
 * object tree to find the objects that were intersected.
 * @param componentInfo :: The object that will provide access to the bounding
 * boxes
 * @param testRay :: An input/output parameter that defines the track and
 * accumulates the intersection results

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
    box.doesLineIntersect(testRay);
  }
}*/

void fireRay(Track &testRay, const ComponentInfo &componentInfo) {
  // Store all the components, starting with root
  std::deque<size_t> components;
  components.push_back(componentInfo.root());

  // Loop through each of the components
  // comp is the componentIndex
  size_t comp;
  while (!components.empty()) {
    comp = components.front();
    components.pop_front();
    BoundingBox box = componentInfo.boundingBox(comp);

    // Test for intersection
    if (box.doesLineIntersect(testRay)) {
      // Get all the children of the current component
      const auto &children = componentInfo.children(comp);

      // Test intersection with children
      for (auto child : children) {
        auto childComponentType = componentInfo.componentType(child);
        if (childComponentType == Types::Rectangular) {
          //checkIntersectsWithRectangularBank(testRay);
        } else if (childComponentType == Types::OutlineComposite) {
          //checkIntersectsWithTubeBank(testRay);
        } else if (childComponentType == Types::Structured) {
          //checkIntersectsWithStructuredBank(testRay);
        } else if (childComponentType == Types::Detector) {
          //checkIntersectsWithDetector(testRay);
        }
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
