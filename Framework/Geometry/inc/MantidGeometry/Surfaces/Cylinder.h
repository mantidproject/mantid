#ifndef Cylinder_h
#define Cylinder_h

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Kernel {
class Logger;
}

namespace Geometry {

/**
\class  Cylinder
\brief Holds a cylinder as a vector form
\author S. Ansell
\date April 2004
\version 1.0

Defines a cylinder as a centre point (on main axis)
a vector from that point (unit) and a radius.

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

class MANTID_GEOMETRY_DLL Cylinder : public Quadratic {
private:
  static Kernel::Logger &PLog; ///< The official logger

  Kernel::V3D Centre; ///< Kernel::V3D for centre
  Kernel::V3D Normal; ///< Direction of centre line
  std::size_t Nvec;   ///< Normal vector is x,y or z :: (1-3) (0 if general)
  double Radius;      ///< Radius of cylinder

  void rotate(const Kernel::Matrix<double> & /*unused*/) override;
  void displace(const Kernel::V3D & /*unused*/) override;
  void setNvec(); ///< check to obtain orientation
  Cylinder *doClone() const override;

protected:
  Cylinder(const Cylinder & /*A*/);
  Cylinder &operator=(const Cylinder & /*A*/);

public:
  /// Public identifer
  std::string className() const override { return "Cylinder"; }

  Cylinder();
  std::unique_ptr<Cylinder> clone() const;

  // Visit acceptor
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }

  virtual double lineIntersect(const Kernel::V3D & /*Pt*/,
                               const Kernel::V3D & /*uVec*/) const;

  int side(const Kernel::V3D & /*unused*/) const override;
  int onSurface(const Kernel::V3D & /*unused*/) const override;
  double distance(const Kernel::V3D & /*unused*/) const override;

  int setSurface(const std::string & /*R*/) override;
  void setCentre(const Kernel::V3D & /*A*/);
  void setNorm(const Kernel::V3D & /*A*/);
  Kernel::V3D getCentre() const { return Centre; } ///< Return centre point
  Kernel::V3D getNormal() const { return Normal; } ///< Return Central line
  double getRadius() const { return Radius; }      ///< Get Radius
  /// Set Radius
  void setRadius(const double &r) {
    Radius = r;
    setBaseEqn();
  }
  void setBaseEqn() override;

  void write(std::ostream & /*unused*/) const override;
  void print() const override;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) override;

  /// The number of slices to approximate a cylinder
  static int g_nslices;
  /// The number of stacks to approximate a cylinder
  static int g_nstacks;
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape createShape() override;
#endif
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid

#endif
