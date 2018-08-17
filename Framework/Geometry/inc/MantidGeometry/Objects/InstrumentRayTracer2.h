#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/V3D.h"
#include <iterator>

/**
InstrumentRayTracer2 contains a set of free functions that are responsible for
tracking rays and accumulating a list of objects that are intersected along the
way.

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

/**
 * Trace a given track from the source in the given direction.
 * @param dir :: A directional vector. The starting point is defined by the
 * instrument source.
 * @param componentInfo :: The object used to access the source position.
 * @return resultsTrack :: The track to trace.
 */
Track traceFromSource(const Kernel::V3D &dir,
                      const ComponentInfo &componentInfo) {

  // Create a new results track to return
  Track resultsTrack(componentInfo.sourcePosition(), dir);

  // Fire the test ray at the instrument
  fireRay(componentInfo, resultsTrack);

  return resultsTrack;
}

/**
 * Trace a given track from the sample position in the given direction.
 * @param dir :: A directional vector. The starting point is defined by the
 * source.
 * @param componentInfo :: The object used to access the source position.
 * @return resultsTrack :: The track to trace.
 */
Track traceFromSample(const Kernel::V3D &dir,
                      const ComponentInfo &componentInfo) {

  // Create a new results track to return
  Track resultsTrack(componentInfo.samplePosition(), dir);

  // Fire the test ray at the instrument
  fireRay(componentInfo, resultsTrack);

  return resultsTrack;
}

/**
 * Return the results of any trace() calls since the last call the getResults.
 * @returns A collection of links defining intersection information
 */
Links getResults(Track &resultsTrack) {
  // Create a list of the results
  Links results(resultsTrack.cbegin(), resultsTrack.cend());

  // Clear intersection results
  resultsTrack.clearIntersectionResults();

  return results;
}

// Return the detector index
size_t getDetectorResult(const ComponentInfo &componentInfo,
                         Track &resultsTrack) {
  // Store the results
  Links results = getResults(resultsTrack);

  // Go through all results
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

/**
 * Fire the test ray at the instrument and perform a bread-first search of the
 * object tree to find the objects that were intersected.
 * @param componentInfo :: The object that will provide access to the bounding
 * boxes
 * @param testRay :: An input/output parameter that defines the track and
 * accumulates the intersection results
 */
void fireRay(const ComponentInfo &componentInfo, Track &testRay) {
  // Loop through the bounding boxes in reverse
  // (essentially a breadth first search)
  for (size_t i = componentInfo.size(); i <= 0; --i) {
    // Store the bounding box
    auto box = componentInfo.boundingBox(i);
    // Test for intersection
    box.doesLineIntersect(testRay);
  }
}

} // namespace InstrumentRayTracer2
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_ */
