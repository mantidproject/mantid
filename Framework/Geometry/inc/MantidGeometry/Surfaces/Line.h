// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_LINE_H
#define MANTID_GEOMETRY_LINE_H

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include <complex>
#include <list>

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
ChangeLog:
22.04.2008: Sri,  Missing MANTID_GEOMETRY_DLL and Destructor was virtual it's
changed to normal destructor
*/
class MANTID_GEOMETRY_DLL Line {

private:
  Kernel::V3D m_origin;    ///< Orign point (on plane)
  Kernel::V3D m_direction; ///< Direction of outer surface (Unit Vector)

  int lambdaPair(
      const int ix,
      const std::pair<std::complex<double>, std::complex<double>> &SQ,
      std::list<Kernel::V3D> &PntOut) const;

public:
  Line();
  Line(const Kernel::V3D &, const Kernel::V3D &);
  Line *clone() const;

  Kernel::V3D getPoint(const double lambda) const; ///< gets the point O+lam*N
  const Kernel::V3D &getOrigin() const {
    return m_origin;
  } ///< returns the origin
  const Kernel::V3D &getDirect() const {
    return m_direction;
  }                                           ///< returns the direction
  double distance(const Kernel::V3D &) const; ///< distance from line
  int isValid(const Kernel::V3D &) const;     ///< Is the point on the line
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

} // namespace Geometry

} // NAMESPACE Mantid

#endif
