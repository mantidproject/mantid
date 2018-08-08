#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_

#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/V3D.h"
#include <iterator>

/**
InstrumentRayTracer2 contains a set of free functions that are responsible for tracking rays and accumulating a list of objects that are intersected along the way.

@author Bhuvan Bezawada, STFC
@date 08/08/2018

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
 * @param resultsTrack :: The track to trace.
 */
void traceFromSource(const Kernel::V3D &dir, const ComponentInfo &componentInfo,
                     const Track &resultsTrack) const {
  resultsTrack.reset(componentInfo.sourcePosition(), dir);
  fireRay(resultsTrack);
}

/**
 * Trace a given track from the sample position in the given direction.
 * @param dir :: A directional vector. The starting point is defined by the
 * source.
 * @param componentInfo :: The object used to access the source position.
 * @param resultsTrack :: The track to trace.
 */
void traceFromSample(const Kernel::V3D &dir, const ComponentInfo &componentInfo,
                     const Track &resultsTrack) const {
  resultsTrack.reset(componentInfo.samplePosition(), dir);
  fireRay(resultsTrack);
}

/**
 * Return the results of any trace() calls since the last call the getResults.
 * @returns A collection of links defining intersection information
 */
Links getResults(const Track &resultsTrack) const {
  Links results(resultsTrack.cbegin(), resultsTrack.cend());
  resultsTrack.clearIntersectionResults();
  return results;
}

} // namespace InstrumentRayTracer2
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_INSTRUMENTRAYTRACER2_H_ */
