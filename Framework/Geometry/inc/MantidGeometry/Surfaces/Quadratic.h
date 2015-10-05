#ifndef Geometry_Quadratic_h
#define Geometry_Quadratic_h

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include <vector>

namespace Mantid {

namespace Geometry {

/**
\class  Quadratic
\brief Holds a basic quadratic surface
\author S. Ansell
\date April 2004
\version 1.0

Holds a basic surface with equation form
\f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]

*/

class MANTID_GEOMETRY_DLL Quadratic : public Surface {
private:
  void matrixForm(Kernel::Matrix<double> &, Kernel::V3D &, double &) const;

protected:
  std::vector<double> BaseEqn; ///< Base equation (as a 10 point vector)

public:
  static const int Nprecision = 10; ///< Precision of the output

  Quadratic();
  Quadratic(const Quadratic &);
  virtual Quadratic *clone() const = 0; ///< Abstract clone function
  Quadratic &operator=(const Quadratic &);
  virtual ~Quadratic();

  /// Accept visitor for line calculation
  virtual void acceptVisitor(BaseVisit &A) const { A.Accept(*this); }

  /// Effective typeid
  virtual std::string className() const { return "Quadratic"; }

  const std::vector<double> &copyBaseEqn() const {
    return BaseEqn;
  } ///< access BaseEquation vector

  virtual int side(const Kernel::V3D &) const;

  virtual void setBaseEqn() = 0; ///< Abstract set baseEqn
  double eqnValue(const Kernel::V3D &) const;

  virtual int
  onSurface(const Kernel::V3D &) const; ///< is point valid on surface
  virtual double distance(const Kernel::V3D &)
      const; ///< distance between point and surface (approx)
  virtual Kernel::V3D
  surfaceNormal(const Kernel::V3D &) const; ///< Normal at surface

  virtual void displace(const Kernel::V3D &);
  virtual void rotate(const Kernel::Matrix<double> &);

  virtual void write(std::ostream &) const;
  virtual void print() const;
};

} // NAMESPACE Geometry

} // NAMESPACE Geometry

#endif
