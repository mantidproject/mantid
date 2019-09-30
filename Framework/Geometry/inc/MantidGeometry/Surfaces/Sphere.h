// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
 */

class MANTID_GEOMETRY_DLL Sphere : public Quadratic {
private:
  Kernel::V3D m_centre; ///< Point for centre
  double m_radius;      ///< Radius of sphere
  void rotate(const Kernel::Matrix<double> &) override;
  void displace(const Kernel::V3D &) override;
  /// Compute the distance from the centre of the sphere to the given point
  double centreToPoint(const Kernel::V3D &pt) const;
  Sphere *doClone() const override;

protected:
  Sphere(const Sphere &) = default;
  Sphere &operator=(const Sphere &) = delete;

public:
  Sphere();
  Sphere(Kernel::V3D centre, double radius);
  std::unique_ptr<Sphere> clone() const;
  /// Effective typename
  std::string className() const override { return "Sphere"; }
  // Visit acceptor
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }
  /// Set the sphere defination by input string in MCNP format
  int setSurface(const std::string &) override;
  /// Checks the given input point to be inside, outside or on the surface of
  /// sphere
  int side(const Kernel::V3D &) const override;
  /// Checks whether the give input point is on the surface
  int onSurface(const Kernel::V3D &) const override;
  /// Gets the distance from the sphere to the input point
  double distance(const Kernel::V3D &) const override;
  /// Setter for centre of sphere
  void setCentre(const Kernel::V3D &);
  /// Get Centre
  Kernel::V3D getCentre() const { return m_centre; }
  /// Get Radius
  double getRadius() const { return m_radius; }
  /// Set Radius
  void setRadius(const double &r) {
    m_radius = r;
    setBaseEqn();
  }
  /// Generates the quadratic equation.
  void setBaseEqn() override;
  /// Writes the sphere equatation in MCNP format
  void write(std::ostream &) const override;
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
