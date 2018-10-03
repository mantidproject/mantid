#ifndef MANTID_GEOMETRY_BEAMLINERAYTRACER_H_
#define MANTID_GEOMETRY_BEAMLINERAYTRACER_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include <vector>

/**
BeamlineRayTracer contains a set of free functions that are responsible for
tracking rays and accumulating a list of objects that are intersected along the
way. These methods have been adapted from the original InstrumentRayTracer and
are intended to be a replacement for InstrumentRayTracer.

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

namespace Kernel {
class V3D;
}

namespace Geometry {
class Track;
using Links = Track::LType;

namespace BeamlineRayTracer {

/**
 * Trace a given track from the instrument source in the given direction
 * and compile a list of results that this track intersects.
 */
MANTID_GEOMETRY_DLL Links traceFromSource(const Kernel::V3D &dir,
                                          const ComponentInfo &componentInfo);
/**
 * Trace a given track from the instrument sample in the given direction
 * and compile a list of results that this track intersects.
 */
MANTID_GEOMETRY_DLL Links traceFromSample(const Kernel::V3D &dir,
                                          const ComponentInfo &componentInfo);
/**
 * Using the results of the trace, return the first detector
 * (that is NOT a monitor) found in the results.
 */
MANTID_GEOMETRY_DLL size_t getDetectorResult(const ComponentInfo &componentInfo,
                                             const DetectorInfo &detectorInfo,
                                             Track &resultsTrack);
/**
 * Trace a collection of tracks from the instrument source in the given directions
 * and compile a list of results that each track intersects.
 */
MANTID_GEOMETRY_DLL std::vector<Links> traceFromSource(const std::vector<Kernel::V3D> &dirs,
                      const ComponentInfo &componentInfo);
/**
 * Trace a collection of tracks from the instrument sample in the given directions
 * and compile a list of results that each track intersects.
 */
MANTID_GEOMETRY_DLL std::vector<Links> traceFromSample(const std::vector<Kernel::V3D> &dirs,
                      const ComponentInfo &componentInfo);

} // namespace BeamlineRayTracer

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BEAMLINERAYTRACER_H_ */
