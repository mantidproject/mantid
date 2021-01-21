// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "BaseVisit.h"
#include "MantidGeometry/DllConfig.h"
#include <memory>
#include <string>

class TopoDS_Shape;

namespace Mantid {
namespace Kernel {
class V3D;
template <class T> class Matrix;
} // namespace Kernel
namespace Geometry {

/**
  \class  Surface
  \brief Holds a basic quadratic surface
  \author S. Ansell
  \date April 2004
  \version 1.0

  Holds a basic surface with equation form
  \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
*/
class MANTID_GEOMETRY_DLL Surface {
private:
  int Name;                             ///< Surface number (MCNPX identifier)
  virtual Surface *doClone() const = 0; ///< Abstract clone function
protected:
  Surface(const Surface &) = default;

public:
  static const int Nprecision = 10; ///< Precision of the output

  Surface();
  std::unique_ptr<Surface> clone() const { return std::unique_ptr<Surface>(doClone()); };

  Surface &operator=(const Surface &) = delete;
  virtual ~Surface() = default;

  /// Effective typeid
  virtual std::string className() const { return "Surface"; }

  /// Accept visitor for line calculation
  virtual void acceptVisitor(BaseVisit &A) const { A.Accept(*this); }

  void setName(int const N) { Name = N; } ///< Set Name
  int getName() const { return Name; }    ///< Get Name

  /// Sets the surface based on a string input in MCNPX format
  virtual int setSurface(const std::string &R) = 0;
  virtual int side(const Kernel::V3D &) const;

  /// is point valid on surface
  virtual int onSurface(const Kernel::V3D &R) const = 0;

  /// returns the minimum distance to the surface
  virtual double distance(const Kernel::V3D &) const = 0;
  /// returns the normal to the closest point on the surface
  virtual Kernel::V3D surfaceNormal(const Kernel::V3D &) const = 0;

  /// translates the surface
  virtual void displace(const Kernel::V3D &) = 0;
  /// rotates the surface
  virtual void rotate(const Kernel::Matrix<double> &) = 0;

  void writeHeader(std::ostream &) const;
  virtual void write(std::ostream &) const;
  virtual void print() const;
  /// bounding box for the surface
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) = 0;
#ifdef ENABLE_OPENCASCADE
  virtual TopoDS_Shape createShape();
#endif
};

} // NAMESPACE Geometry

} // NAMESPACE Mantid
