// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Track.h"
#include <boost/unordered_map.hpp>
#include <deque>
#include <list>
#include <mutex>

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class ComponentInfo;
class DetectorInfo;
struct Link;
class Track;
/// Typedef for object intersections
using Links = Track::LType;

/**
This class is responsible for tracking rays and accumulating a list of objects
that are
intersected along the way.

@author Martyn Gigg, Tessella plc
@date 22/10/2010
*/
class MANTID_GEOMETRY_DLL InstrumentRayTracer {
public:
  /// Constructor taking the ComponentInfo/DetectorInfo of the instrument to
  /// trace, which must outlive this object
  InstrumentRayTracer(ComponentInfo const &componentInfo, DetectorInfo const &detectorInfo);
  /// Trace a given track from the instrument source in the given direction
  /// and compile a list of results that this track intersects.
  void trace(const Kernel::V3D &dir) const;
  void traceFromSample(const Kernel::V3D &dir) const;
  /// Get the results of the intersection tests that have been updated
  /// since the previous call to trace
  Links getResults() const;

  /// Get the index of the first non-monitor detector intersected since the
  /// previous call to trace, or ComponentInfo::invalidIndex if none
  size_t getDetectorResult() const;
  /// ComponentInfo/DetectorInfo for the traced instrument, against which the
  /// index returned by getDetectorResult is defined.
  const ComponentInfo &componentInfo() const;
  const DetectorInfo &detectorInfo() const;

private:
  /// Fire the given track at the instrument
  void fireRay(Track &testRay) const;
  /// Intersect the track with the shape of a leaf component
  void testIntersectionWithShape(size_t const index, Track &testRay) const;
  /// Intersect the track with the first layer of a Rectangular or Grid bank
  void testIntersectionWithGrid(size_t const index, Track &testRay) const;

  /// ComponentInfo of the instrument being traced
  ComponentInfo const &m_componentInfo;
  /// DetectorInfo of the instrument being traced
  DetectorInfo const &m_detectorInfo;
  /// Accumulate results in this Track object, aids performance. This is cleared
  /// when getResults is called.
  mutable Track m_resultsTrack;
  /// Map of component index -> bounding box.
  mutable boost::unordered_map<size_t, BoundingBox> m_boxCache;
  /// Mutex to lock box cache
  mutable std::mutex m_mutex;
};
} // namespace Geometry
} // namespace Mantid
