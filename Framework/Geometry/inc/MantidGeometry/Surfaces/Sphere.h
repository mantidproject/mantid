#ifndef SPHERE_H
#define SPHERE_H

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/V3D.h"
#include <string>

class TopoDS_Shape;

namespace Mantid {

namespace Geometry {
/**
 \class Sphere
 \brief Holds a Sphere as vector form
 \author S. Ansell
 \date April 2004
 \version 1.0

 Defines a sphere as a centre point and a radius.

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
 */

class MANTID_GEOMETRY_DLL Sphere : public Quadratic {
private:
  Kernel::V3D Centre; ///< Point for centre
  double Radius;      ///< Radius of sphere
  void rotate(const Kernel::Matrix<double> & /*unused*/) override;
  void displace(const Kernel::V3D & /*unused*/) override;
  /// Compute the distance from the centre of the sphere to the given point
  double centreToPoint(const Kernel::V3D &pt) const;
  Sphere *doClone() const override;

protected:
  Sphere(const Sphere & /*A*/);
  Sphere &operator=(const Sphere & /*A*/);

public:
  Sphere();
  std::unique_ptr<Sphere> clone() const;
  /// Effective typename
  std::string className() const override { return "Sphere"; }
  // Visit acceptor
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }
  /// Set the sphere defination by input string in MCNP format
  int setSurface(const std::string & /*R*/) override;
  /// Checks the given input point to be inside, outside or on the surface of
  /// sphere
  int side(const Kernel::V3D & /*unused*/) const override;
  /// Checks whether the give input point is on the surface
  int onSurface(const Kernel::V3D & /*unused*/) const override;
  /// Gets the distance from the sphere to the input point
  double distance(const Kernel::V3D & /*unused*/) const override;
  /// Setter for centre of sphere
  void setCentre(const Kernel::V3D & /*A*/);
  /// Get Centre
  Kernel::V3D getCentre() const { return Centre; }
  /// Get Radius
  double getRadius() const { return Radius; }
  /// Set Radius
  void setRadius(const double &r) {
    Radius = r;
    setBaseEqn();
  }
  /// Generates the quadratic equation.
  void setBaseEqn() override;
  /// Writes the sphere equatation in MCNP format
  void write(std::ostream & /*unused*/) const override;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) override;

  /// The number of slices to approximate a sphere
  static int g_nslices;
  /// The number of stacks to approximate a sphere
  static int g_nstacks;
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape createShape() override;
#endif
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
