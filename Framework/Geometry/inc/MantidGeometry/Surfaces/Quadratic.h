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
  Quadratic *doClone() const override = 0;

protected:
  std::vector<double> BaseEqn; ///< Base equation (as a 10 point vector)
  Quadratic(const Quadratic &) = default;
  Quadratic &operator=(const Quadratic &) = default;

public:
  static const int Nprecision = 10; ///< Precision of the output

  Quadratic();
  std::unique_ptr<Quadratic> clone() const;

  /// Accept visitor for line calculation
  void acceptVisitor(BaseVisit &A) const override { A.Accept(*this); }

  /// Effective typeid
  std::string className() const override { return "Quadratic"; }

  const std::vector<double> &copyBaseEqn() const {
    return BaseEqn;
  } ///< access BaseEquation vector

  int side(const Kernel::V3D &) const override;

  virtual void setBaseEqn() = 0; ///< Abstract set baseEqn
  double eqnValue(const Kernel::V3D &) const;

  int onSurface(
      const Kernel::V3D &) const override; ///< is point valid on surface
  double distance(const Kernel::V3D &)
      const override; ///< distance between point and surface (approx)
  Kernel::V3D
  surfaceNormal(const Kernel::V3D &) const override; ///< Normal at surface

  void displace(const Kernel::V3D &) override;
  void rotate(const Kernel::Matrix<double> &) override;

  void write(std::ostream &) const override;
  void print() const override;
};

} // NAMESPACE Geometry

} // namespace Mantid

#endif
