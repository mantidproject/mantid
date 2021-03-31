// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

*/

class MANTID_GEOMETRY_DLL Cylinder : public Quadratic {
private:
  static Kernel::Logger &PLog; ///< The official logger

  Kernel::V3D m_centre;  ///< Kernel::V3D for centre
  Kernel::V3D m_normal;  ///< Direction of centre line
  std::size_t m_normVec; ///< Normal vector is x,y or z :: (1-3) (0 if general)
  double m_radius;       ///< Radius of cylinder

  void rotate(const Kernel::Matrix<double> &) override;
  void displace(const Kernel::V3D &) override;
  void setNormVec(); ///< check to obtain orientation
  Cylinder *doClone() const override;

protected:
  Cylinder(const Cylinder &) = default;
  Cylinder &operator=(const Cylinder &) = delete;

public:
  /// Public identifer
  std::string className() const override { return "Cylinder"; }

  Cylinder();
  std::unique_ptr<Cylinder> clone() const;

  // Visit acceptor
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }

  virtual double lineIntersect(const Kernel::V3D &, const Kernel::V3D &) const;

  int side(const Kernel::V3D &) const override;
  int onSurface(const Kernel::V3D &) const override;
  double distance(const Kernel::V3D &) const override;

  int setSurface(const std::string &) override;
  void setCentre(const Kernel::V3D &);
  void setNorm(const Kernel::V3D &);
  Kernel::V3D getCentre() const { return m_centre; } ///< Return centre point
  Kernel::V3D getNormal() const { return m_normal; } ///< Return Central line
  double getRadius() const { return m_radius; }      ///< Get Radius
  /// Set Radius
  void setRadius(const double &r) {
    m_radius = r;
    setBaseEqn();
  }
  void setBaseEqn() override;

  void write(std::ostream &) const override;
  void print() const override;
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) override;

  /// The number of slices to approximate a cylinder
  constexpr static int g_NSLICES = 10;
  /// The number of stacks to approximate a cylinder
  constexpr static int g_NSTACKS = 1;
#ifdef ENABLE_OPENCASCADE
  TopoDS_Shape createShape() override;
#endif
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid
