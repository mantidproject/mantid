#ifndef MANTID_GEOMETRY_BEAMLINERAYTRACER_H_
#define MANTID_GEOMETRY_BEAMLINERAYTRACER_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <vector>

/**
BeamlineRayTracer contains a set of free functions that are responsible for
tracking rays and accumulating a list of objects that are intersected along the
way. These methods have been adapted from the original InstrumentRayTracer and
are intended to be a replacement for InstrumentRayTracer.

@author Bhuvan Bezawada & Samuel Jackson, STFC
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

// Type alias for caching bounding boxes when making
// multiple ray traces
using BoundingBoxCache = std::unordered_map<size_t, BoundingBox>;

namespace BeamlineRayTracer {

/// Trace a given track from the source in the given direction.
MANTID_GEOMETRY_DLL Links traceFromSource(const Kernel::V3D &dir,
                                          const ComponentInfo &componentInfo,
                                          BoundingBoxCache &cache);

/// Trace a given track from the sample position in the given direction.
MANTID_GEOMETRY_DLL Links traceFromSample(const Kernel::V3D &dir,
                                          const ComponentInfo &componentInfo,
                                          BoundingBoxCache &cache);

/// Trace a given track from the sample position in the given direction.
MANTID_GEOMETRY_DLL Links traceFromSource(const Kernel::V3D &dir,
                                          const ComponentInfo &componentInfo);

/// Trace a given track from the sample position in the given direction.
MANTID_GEOMETRY_DLL Links traceFromSample(const Kernel::V3D &dir,
                                          const ComponentInfo &componentInfo);

/// Gets the results of the trace, then returns the first detector index found
MANTID_GEOMETRY_DLL size_t getDetectorResult(const ComponentInfo &componentInfo,
                                             const DetectorInfo &detectorInfo,
                                             Track &resultsTrack);

/**
 * Trace a given track from the source position in the given directions.
 *
 * @param start :: beginning of the range of directions to test
 * @param end :: end of the range of directions to test
 * @param out :: beginning of the range to output ray trace results in
 * @param componentInfo :: component info object containing the geometry to
 * trace in
 */
template <class InputIt, class OutputIt>
void traceFromSource(InputIt start, InputIt end, OutputIt out,
                     const ComponentInfo &componentInfo) {

  BoundingBoxCache cache;
  std::transform(start, end, out, [&componentInfo, &cache](const auto &dir) {
    return traceFromSource(dir, componentInfo, cache);
  });
}

/**
 * Trace a given track from the sample position in the given directions.
 *
 * @param start :: beginning of the range of directions to test
 * @param end :: end of the range of directions to test
 * @param out :: beginning of the range to output ray trace results in
 * @param componentInfo :: component info object containing the geometry to
 * trace in
 */
template <class InputIt, class OutputIt>
void traceFromSample(InputIt start, InputIt end, OutputIt out,
                     const ComponentInfo &componentInfo) {

  BoundingBoxCache cache;
  std::transform(start, end, out, [&componentInfo, &cache](const auto &dir) {
    return traceFromSample(dir, componentInfo, cache);
  });
}

} // namespace BeamlineRayTracer

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_BEAMLINERAYTRACER_H_ */
