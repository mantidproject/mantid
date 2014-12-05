#ifndef MANTID_GEOMETRY_LINEINTERSECTVISIT_H
#define MANTID_GEOMETRY_LINEINTERSECTVISIT_H

#include "MantidGeometry/Surfaces/BaseVisit.h"
#include "MantidGeometry/Surfaces/Line.h"
#include "MantidKernel/V3D.h"
#include <list>

namespace Mantid
{

  namespace Geometry
  {

    //---------------------------------------------------------
    // Forward declarations
    //---------------------------------------------------------
    class Surface;
    class Quadratic;
    class Plane;
    class Sphere;
    class Cone;
    class Cylinder;
    class General;

    /**
    \class LineIntersectVisit
    \author S. Ansell
    \version 1.0
    \date September 2007 
    \brief Interset of Line with a surface 

    Creates interaction with a line

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    */
    class MANTID_GEOMETRY_DLL LineIntersectVisit : public BaseVisit
    {
    private:

      Line ATrack;                         ///< The line
      std::list<Kernel::V3D> PtOut;  ///< The intersection point
      std::list<double> DOut;            ///< The distance

      void procTrack();

    public:

      LineIntersectVisit(const Kernel::V3D&,
        const Kernel::V3D&);
      /// Destructor
      virtual ~LineIntersectVisit() {}

      void Accept(const Surface&);
      void Accept(const Quadratic&);
      void Accept(const Plane&);
      void Accept(const Sphere&);
      void Accept(const Cone&);
      void Accept(const Cylinder&);
      void Accept(const General&);

      // Accessor
      /// Get the distance
      const std::list<double>& getDistance() const 
      { return DOut; }
      /// Get the intersection points
      const std::list<Kernel::V3D>& getPoints() const 
      { return PtOut; }
      /// Get the number of intersection points
      unsigned long getNPoints() const { return (unsigned long)PtOut.size(); }

      /// Re-set the line
      void setLine(const Kernel::V3D&,const Kernel::V3D&);

    };

  }  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
