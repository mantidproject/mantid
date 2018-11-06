// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_

#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
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
class IComponent;
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
  /// Constructor taking an instrument
  InstrumentRayTracer(Instrument_const_sptr instrument);
  /// Trace a given track from the instrument source in the given direction
  /// and compile a list of results that this track intersects.
  void trace(const Kernel::V3D &dir) const;
  void traceFromSample(const Kernel::V3D &dir) const;
  /// Get the results of the intersection tests that have been updated
  /// since the previous call to trace
  Links getResults() const;

  IDetector_const_sptr getDetectorResult() const;

private:
  /// Default constructor
  InstrumentRayTracer();
  /// Fire the given track at the instrument
  void fireRay(Track &testRay) const;

  /// Pointer to the instrument
  Instrument_const_sptr m_instrument;
  /// Accumulate results in this Track object, aids performance. This is cleared
  /// when getResults is called.
  mutable Track m_resultsTrack;
  /// Map of component id -> bounding box.
  mutable boost::unordered_map<IComponent *, BoundingBox> m_boxCache;
  /// Mutex to lock box cache
  mutable std::mutex m_mutex;
};
} // namespace Geometry
} // namespace Mantid

#endif // MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_
