#ifndef MANTID_GEOMETRY_LINE_H
#define MANTID_GEOMETRY_LINE_H

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace Kernel {
template <typename T> class Matrix;
}

namespace Geometry {
//--------------------------------------
// Forward declarations
//--------------------------------------
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

Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
ChangeLog:
22.04.2008: Sri,  Missing MANTID_GEOMETRY_DLL and Destructor was virtual it's
changed to normal destructor
*/
class MANTID_GEOMETRY_DLL Line {

private:
  Kernel::V3D Origin; ///< Orign point (on plane)
  Kernel::V3D Direct; ///< Direction of outer surface (Unit Vector)

  int
  lambdaPair(const int ix,
             const std::pair<std::complex<double>, std::complex<double>> &SQ,
             std::list<Kernel::V3D> &PntOut) const;

public:
  Line();
  Line(const Kernel::V3D &, const Kernel::V3D &);
  Line(const Line &);
  Line &operator=(const Line &);
  Line *clone() const;

  ~Line();

  Kernel::V3D getPoint(const double lambda) const; ///< gets the point O+lam*N
  Kernel::V3D getOrigin() const { return Origin; } ///< returns the origin
  Kernel::V3D getDirect() const { return Direct; } ///< returns the direction
  double distance(const Kernel::V3D &) const;      ///< distance from line
  int isValid(const Kernel::V3D &) const;          ///< Is the point on the line
  void print() const;

  void rotate(const Kernel::Matrix<double> &);
  void displace(const Kernel::V3D &);

  int setLine(const Kernel::V3D &,
              const Kernel::V3D &); ///< input Origin + direction

  int intersect(std::list<Kernel::V3D> &, const Quadratic &) const;
  int intersect(std::list<Kernel::V3D> &, const Cylinder &) const;
  int intersect(std::list<Kernel::V3D> &, const Plane &) const;
  int intersect(std::list<Kernel::V3D> &, const Sphere &) const;
};

} // NAMESPACE MonteCarlo

} // NAMESPACE Mantid

#endif
