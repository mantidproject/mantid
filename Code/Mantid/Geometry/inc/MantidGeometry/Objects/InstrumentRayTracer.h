#ifndef MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_
#define MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_

//-------------------------------------------------------------
// Includes
//-------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IInstrument.h"
#include <list>
#include <deque>

namespace Mantid
{
  namespace Geometry
  {
    //-------------------------------------------------------------
    // Forward declarations
    //-------------------------------------------------------------
    struct Link;
    class Track;
    class V3D;

    /**
    This class is responsible for tracking rays and accumulating a list of objects that are 
    intersected along the way.

    @author Martyn Gigg, Tessella plc
    @date 22/10/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport InstrumentRayTracer
    {
    public:
      // Result type for trace tests
      typedef std::list<Link> TraceResults;

      /// Constructor taking an instrument
      InstrumentRayTracer(IInstrument_sptr instrument);      
      /// Trace a given track from the instrument source in the given direction 
      /// and compile a list of results that this track intersects.
      TraceResults trace(const V3D & ray) const;

    private:
      /// Default constructor
      InstrumentRayTracer();
      /// Fire the given track at the instrument
      void fireRay(Track & testRay) const;
      /// Test the physical intersection of a track and any component children
      void testIntersectionWithChildren(Track & testRay, ICompAssembly_sptr assembly, 
        std::deque<IComponent_sptr> & searchQueue) const;

      /// Pointer to the instrument
      IInstrument_sptr m_instrument;
      /// Logger
      static Kernel::Logger & g_log;
    };

  }
}

#endif //MANTID_GEOMETRY_INSTRUMENTRAYTRACER_H_