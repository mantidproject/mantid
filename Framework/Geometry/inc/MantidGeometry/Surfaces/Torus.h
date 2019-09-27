// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef Torus_h
#define Torus_h

#include "MantidKernel/V3D.h"

namespace Mantid {

namespace Geometry {

/**
  \class Torus
  \brief Holds a torus in vector form
  \author S. Ansell
  \date December 2006
  \version 1.0

  Defines a Torus as a centre, normal and
  three parameters:
  - Iradius :: inner radius of the torus
  - Dradius :: displaced radius (the radius away from the plane)
  - Displacement :: elipse displacment from the centre.
  These are c,b,a in that order.
*/

class MANTID_GEOMETRY_DLL Torus : public Surface {
private:
  Kernel::V3D Centre;  ///< Geometry::Vec3D for centre
  Kernel::V3D Normal;  ///< Normal
  double Iradius;      ///< Inner radius
  double Dradius;      ///< Inner radius
  double Displacement; ///< Displacement

  void rotate(const Kernel::Matrix<double> &) override;
  void displace(const Kernel::V3D &) override;
  Torus *doClone() const override;

protected:
  Torus(const Torus &) = default;
  Torus &operator=(const Torus &) = delete;

public:
  /// Public identifier
  std::string className() const override { return "Torus"; }

  Torus();
  std::unique_ptr<Torus> clone() const;
  int operator==(const Torus &) const;

  /// Accept visitor for line calculation
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }

  int setSurface(const std::string &Pstr) override;
  int side(const Kernel::V3D &R) const override;
  int onSurface(const Kernel::V3D &R) const override;
  double distance(const Kernel::V3D &Pt) const override;

  /// Return centre point
  Kernel::V3D getCentre() const { return Centre; }
  /// Central normal
  Kernel::V3D getNormal() const { return Normal; }
  Kernel::V3D surfaceNormal(const Kernel::V3D &Pt) const override;

  void setCentre(const Kernel::V3D &A);
  void setNorm(const Kernel::V3D &A);

  /// Suppose to set the distance from centre of the torus to the centre of
  /// tube.
  /// TODO:
  void setDistanceFromCentreToTube(double dist);

  /// Suppose to set the radius of the tube which makes up the torus
  /// TODO:
  void setTubeRadius(double dist);

  void write(std::ostream &OX) const override;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) override;
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape createShape() override;
#endif
};

} // namespace Geometry

} // NAMESPACE Mantid

#endif
