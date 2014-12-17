#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_

//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include <deque>
#include <list>

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
//-------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------
struct Link;
class Track;
/// Typedef for object intersections
typedef Track::LType Links;

/**
This class is responsible for tracking rays and accumulating a list of objects
that are
intersected along the way.

@author Martyn Gigg, Tessella plc
@date 22/10/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL InstrumentRayTracer {
public:
  /// Constructor taking an instrument
  InstrumentRayTracer(Instrument_const_sptr instrument);
  /// Trace a given track from the instrument source in the given direction
  /// and compile a list of results that this track intersects.
  void trace(const Kernel::V3D &direction) const;
  void traceFromSample(const Kernel::V3D &direction) const;
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
};
}
}

#endif // MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_
