#ifndef MANTID_GEOMETRY_LINE_H
#define MANTID_GEOMETRY_LINE_H

#include "MantidGeometry/V3D.h"

namespace Mantid
{

  namespace Kernel
  {
    class Logger;
  }

  namespace Geometry
  {
    //--------------------------------------
    // Forward declarations
    //--------------------------------------
    template<typename T> class Matrix;
    class Quadratic;
    class Cylinder;
    class Plane;
    class Sphere;

    /**
    \class Line
    \brief Impliments a line
    \author S. Ansell
    \date Apr 2005
    \version 0.7

    Impliments the line 
    \f[ r=\vec{O} + \lambda \vec{n} \f]

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    ChangeLog:
    22.04.2008: Sri,  Missing DLLExport and Destructor was virtual it's changed to normal destructor
    */
    class DLLExport Line 
    {

    private:

      static Kernel::Logger& PLog;           ///< The official logger


      Geometry::V3D Origin;   ///< Orign point (on plane)
      Geometry::V3D Direct;   ///< Direction of outer surface (Unit Vector) 

      int lambdaPair(const int ix,const std::pair<std::complex<double>,
        std::complex<double> >& SQ,std::vector<Geometry::V3D>& PntOut) const;

    public: 

      Line();
      Line(const Geometry::V3D&,const Geometry::V3D&);
      Line(const Line&);
      Line& operator=(const Line&);
      Line* clone() const;

      ~Line();

      Geometry::V3D getPoint(const double lambda) const;   ///< gets the point O+lam*N
      Geometry::V3D getOrigin() const { return Origin; }   ///< returns the origin
      Geometry::V3D getDirect() const { return Direct; }   ///< returns the direction
      double distance(const Geometry::V3D&) const;  ///< distance from line
      int isValid(const Geometry::V3D&) const;     ///< Is the point on the line
      void print() const;

      void rotate(const Geometry::Matrix<double>&);
      void displace(const Geometry::V3D&);

      int setLine(const Geometry::V3D&,const Geometry::V3D&);     ///< input Origin + direction

      int intersect(std::vector<Geometry::V3D>&,const Quadratic&) const;
      int intersect(std::vector<Geometry::V3D>&,const Cylinder&) const;
      int intersect(std::vector<Geometry::V3D>&,const Plane&) const;
      int intersect(std::vector<Geometry::V3D>&,const Sphere&) const;

    };

  }  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid

#endif
