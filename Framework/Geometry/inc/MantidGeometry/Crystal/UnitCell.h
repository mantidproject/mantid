#ifndef MANTID_GEOMETRY_UNITCELL_H_
#define MANTID_GEOMETRY_UNITCELL_H_

#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <iosfwd>
#include <vector>

namespace Mantid {
namespace Geometry {
/** @class UnitCell UnitCell.h Geometry/Crystal/UnitCell.h
  Class to implement unit cell of crystals.
  It is based on code by Laurent Chapon. It does not contain information about
  lattice orientation.
  See documentation about UB matrix in the Mantid repository.\n
  For documentation purposes, units for lengths are assumed to be \f$ \mbox{ \AA
  } \f$, and for reciprocal lattice lengths
  \f$ \mbox{ \AA }^{-1} \f$,  but can be anything, as long as used consistently.
  Note that the convention used for
  reciprocal lattice follows the one in International Tables for
  Crystallography, meaning that for an orthogonal lattice
  \f$ a^{*} = 1/a \f$ , not   \f$ a^{*} = 2 \pi /a \f$

  References:
    - International Tables for Crystallography (2006). Vol. B, ch. 1.1, pp. 2-9
    - W. R. Busing and H. A. Levy, Angle calculations for 3- and 4-circle X-ray
  and neutron diffractometers - Acta Cryst. (1967). 22, 457-464


  @author Andrei Savici, SNS, ORNL
  @date 2011-03-23
  Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class MANTID_GEOMETRY_DLL UnitCell {
public:
  // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
  UnitCell();
  // a,b,c constructor
  UnitCell(const double _a, const double _b, const double _c);
  // a,b,c,alpha,beta,gamma constructor
  UnitCell(const double _a, const double _b, const double _c,
           const double _alpha, const double _beta, const double _gamma,
           const int angleunit = angDegrees);
  // Destructor
  virtual ~UnitCell() = default;

  // Get and set lattice parameters
  // Direct lattice parameters, angle in radians.
  double a1() const;
  double a2() const;
  double a3() const;
  double alpha1() const;
  double alpha2() const;
  double alpha3() const;
  // Direct lattice parameters, angle in degrees.
  double a(int nd) const; // get lattice parameter as function of index (0-2)
  double a() const;
  double b() const;
  double c() const;
  double alpha() const;
  double beta() const;
  double gamma() const;
  // Reciprocal lattice parameters, angle in radians.
  double b1() const;
  double b2() const;
  double b3() const;
  double beta1() const;
  double beta2() const;
  double beta3() const;
  // Reciprocal lattice parameters, angle in degrees.
  double astar() const;
  double bstar() const;
  double cstar() const;
  double alphastar() const;
  double betastar() const;
  double gammastar() const;
  // Set lattice
  void set(double _a, double _b, double _c, double _alpha, double _beta,
           double _gamma, const int angleunit = angDegrees);
  void seta(double _a);
  void setb(double _b);
  void setc(double _c);
  void setalpha(double _alpha, const int angleunit = angDegrees);
  void setbeta(double _beta, const int angleunit = angDegrees);
  void setgamma(double _gamma, const int angleunit = angDegrees);
  // Set errors
  void setError(double _aerr, double _berr, double _cerr, double _alphaerr,
                double _betaerr, double _gammaerr,
                const int angleunit = angDegrees);
  void setErrora(double _aerr);
  void setErrorb(double _berr);
  void setErrorc(double _cerr);
  void setErroralpha(double _alphaerr, const int angleunit = angDegrees);
  void setErrorbeta(double _betaerr, const int angleunit = angDegrees);
  void setErrorgamma(double _gammaerr, const int angleunit = angDegrees);
  // Get errors in latice parameters
  double errora() const;
  double errorb() const;
  double errorc() const;
  double erroralpha(const int angleunit = angDegrees) const;
  double errorbeta(const int angleunit = angDegrees) const;
  double errorgamma(const int angleunit = angDegrees) const;
  double errorvolume() const;
  // Access private variables
  const Kernel::DblMatrix &getG() const;
  const Kernel::DblMatrix &getGstar() const;
  const Kernel::DblMatrix &getB() const;
  const Kernel::DblMatrix &getBinv() const;

  // Calculate things about lattice and vectors
  double d(double h, double k, double l) const;
  double dstar(double h, double k, double l) const;
  double d(const Kernel::V3D &hkl) const;
  double dstar(const Kernel::V3D &hkl) const;
  double recAngle(double h1, double k1, double l1, double h2, double k2,
                  double l2, const int angleunit = angDegrees) const;
  double volume() const;
  double recVolume() const;
  virtual void recalculateFromGstar(const Kernel::Matrix<double> &NewGstar);

protected:
  /// Lattice parameter a,b,c,alpha,beta,gamma (in \f$ \mbox{ \AA } \f$ and
  /// radians)
  std::vector<double> da;
  /// Reciprocal lattice parameters (in \f$ \mbox{ \AA }^{-1} \f$ and radians)
  std::vector<double> ra;
  /// Error in lattice parameters (in \f$ \mbox{ \AA } \f$ and radians)
  std::vector<double> errorda;
  /** Metric tensor
   \f[ \left( \begin{array}{ccc}
    aa & ab\cos(\gamma) & ac\cos(\beta) \\
    ab\cos(\gamma) & bb & bc\cos(\alpha) \\
    ac\cos(\beta) & bc\cos(\alpha) & cc \end{array} \right) \f]
   */
  Kernel::DblMatrix G;
  /** Reciprocal lattice tensor
   *\f[ \left( \begin{array}{ccc}
    a^*a^* & a^*b^*\cos(\gamma^*) & a^*c^*\cos(\beta^*) \\
    a^*b^*\cos(\gamma^*) & b^*b^* & b^*c^*\cos(\alpha^*) \\
    a^*c^*\cos(\beta^*) & b^*c^*\cos(\alpha^*) & c^*c^* \end{array} \right) \f]
   */
  Kernel::DblMatrix Gstar;
  /** B matrix for a right-handed coordinate system, in Busing-Levy convention
   \f[ \left( \begin{array}{ccc}
    a^* & b^*\cos(\gamma^*) & c^*\cos(\beta^*) \\
    0 & b^*\sin(\gamma^*) & -c^*\sin(\beta^*)\cos(\alpha) \\
    0 & 0 & 1/c \end{array} \right) \f]
   */
  Kernel::DblMatrix B;

  /** Inverse of the B matrix.
   */
  Kernel::DblMatrix Binv;

  // Private functions

  void calculateG();
  void calculateGstar();
  void calculateReciprocalLattice();
  void calculateB();

  virtual void recalculate();
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &out,
                                             const UnitCell &unitCell);

MANTID_GEOMETRY_DLL UnitCell strToUnitCell(const std::string &unitCellString);
MANTID_GEOMETRY_DLL std::string unitCellToStr(const UnitCell &unitCell);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_UNITCELL_H_ */
