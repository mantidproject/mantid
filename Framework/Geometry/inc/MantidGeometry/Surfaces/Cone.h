// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef Cone_h
#define Cone_h

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace Geometry {
/**
  \class Cone
  \brief Holds a cone in vector form
  \author S. Ansell
  \date April 2004
  \version 1.0

  Defines a cone as a centre point (on main axis)
  a vector from that point (unit) and a radius.
  and an angle.

*/

class MANTID_GEOMETRY_DLL Cone : public Quadratic {
private:
  Kernel::V3D Centre; ///< Kernel::V3D for centre
  Kernel::V3D Normal; ///< Normal
  double alpha;       ///< Angle (degrees)
  double cangle;      ///< Cos(angle)

  void rotate(const Kernel::Matrix<double> &) override;
  void displace(const Kernel::V3D &) override;
  Cone *doClone() const override;

protected:
  Cone(const Cone &) = default;
  Cone &operator=(const Cone &) = default;

public:
  /// Public identifer
  std::string className() const override { return "Cone"; }
  Cone();
  std::unique_ptr<Cone> clone() const;
  int operator==(const Cone &) const;
  /// Calculate if the point R is within the cone (return -1) or outside (return
  /// 1)
  int side(const Kernel::V3D &R) const override;
  /// Calculate if the point R is on the cone(1=on the surface, 0=not)
  int onSurface(const Kernel::V3D &R) const override;

  /// Accept visitor for line calculation
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }

  /// Return centre point
  Kernel::V3D getCentre() const { return Centre; }
  /// Central normal
  Kernel::V3D getNormal() const { return Normal; }
  /// Edge Angle
  double getCosAngle() const { return cangle; }
  /// This method returns the distance of the point from the cone
  double distance(const Kernel::V3D &) const override;

  /// This method sets the cone surface using the input string in MCNPx format
  int setSurface(const std::string &) override;
  /// This method sets the centre of the cone
  void setCentre(const Kernel::V3D &);
  /// This method sets the cone normal
  void setNorm(const Kernel::V3D &);
  /// This method sets the angle of the cone
  void setAngle(double const);
  /// This method sets the tan angle which will be converted to cos used for
  /// MCNPX format
  void setTanAngle(double const);
  /// This method generates the quadratic equation for cone
  void setBaseEqn() override;
  /// This method will write the cone equation in MCNP geometry format
  void write(std::ostream &) const override;

  /// This will get the bounding box for the cone
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin,
                      double &ymin, double &zmin) override;

  /// The number of slices to approximate a cone
  constexpr static int g_nslices = 10;
  /// The number of stacks to approximate a cone
  constexpr static int g_nstacks = 1;
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape createShape() override;
#endif
};

} // namespace Geometry

} // NAMESPACE Mantid

#endif
