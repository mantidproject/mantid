#ifndef MDPoint_H_
#define MDPoint_H_

/** Abstract type to represent a multidimensional pixel/point. 
*   This type may not be a suitable pure virtual type in future iterations. Future implementations should be non-abstract.

    @author Owen Arnold, RAL ISIS
    @date 15/11/2010

    Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllExport.h"
#include <vector>
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/MDGeometry/Coordinate.h"

namespace Mantid
{
  namespace Geometry
  {
    //TODO: relocate once name is stable.
    class MDPoint;
    class EXPORT_OPT_MANTID_GEOMETRY SignalAggregate
    {
    public:
      virtual std::vector<coordinate> getVertexes() const = 0;
      virtual double getSignal() const = 0;
      virtual double getError() const = 0;
      virtual std::vector<boost::shared_ptr<MDPoint> > getContributingPoints() const = 0;
      virtual ~SignalAggregate(){};
    };

    class EXPORT_OPT_MANTID_GEOMETRY MDPoint : public SignalAggregate
    {
    private:
      double m_signal;
      double m_error;
      IInstrument_sptr m_instrument;
      IDetector_sptr m_detector;
      std::vector<coordinate> m_vertexes;
    public:
      MDPoint(){};
      MDPoint(double signal, double error, const std::vector<coordinate>& vertexes, IDetector_sptr detector, IInstrument_sptr instrument);
      std::vector<coordinate> getVertexes() const;
      double getSignal() const;
      double getError() const;
      IDetector_sptr getDetector() const;
      IInstrument_sptr getInstrument() const;
      virtual ~MDPoint();
      std::vector<boost::shared_ptr<MDPoint> > getContributingPoints() const;
      //virtual Mantid::API::Run getRun();
    protected:
    };
  }
}

#endif 
