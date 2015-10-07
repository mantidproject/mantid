#include "MantidGeometry/Crystal/NiggliCell.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Quat.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace Mantid {
namespace Geometry {
using Mantid::Kernel::DblMatrix;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Matrix;

namespace {
const double RAD_TO_DEG = 180. / M_PI;
}
/**
   Comparator function for sorting list of UB matrices based on the sum
   of the lengths of the corresponding real space cell edge lengths
   |a|+|b|+|C|
 */
static bool CompareABCsum(const DblMatrix &UB_1, const DblMatrix &UB_2) {
  V3D a1;
  V3D b1;
  V3D c1;
  V3D a2;
  V3D b2;
  V3D c2;
  OrientedLattice::GetABC(UB_1, a1, b1, c1);
  OrientedLattice::GetABC(UB_2, a2, b2, c2);

  double sum_1 = a1.norm() + b1.norm() + c1.norm();
  double sum_2 = a2.norm() + b2.norm() + c2.norm();

  return (sum_1 < sum_2);
}

/**
   Get the cell angles for the unit cell corresponding to matrix UB
   and calculate the sum of the differences of the cell angles from 90
   degrees.
   @param UB   the UB matrix
   @return The sum of the difference of the cell angles from 90 degrees.
 */
static double GetDiffFrom90Sum(const DblMatrix &UB) {
  V3D a;
  V3D b;
  V3D c;

  if (!OrientedLattice::GetABC(UB, a, b, c))
    return -1;

  double alpha = b.angle(c) * RAD_TO_DEG;
  double beta = c.angle(a) * RAD_TO_DEG;
  double gamma = a.angle(b) * RAD_TO_DEG;

  double sum = fabs(alpha - 90.0) + fabs(beta - 90.0) + fabs(gamma - 90.0);

  return sum;
}

/**
   Comparator to sort a list of UBs in decreasing order based on
   the difference of cell angles from 90 degrees.
*/
static bool CompareDiffFrom90(const DblMatrix &UB_1, const DblMatrix &UB_2) {
  double sum_1 = GetDiffFrom90Sum(UB_1);
  double sum_2 = GetDiffFrom90Sum(UB_2);

  return (sum_2 < sum_1);
}
/** Default constructor
@param Umatrix :: orientation matrix U. By default this will be identity matrix
*/
NiggliCell::NiggliCell(const DblMatrix &Umatrix) : UnitCell() {
  if (Umatrix.isRotation() == true) {
    U = Umatrix;
    UB = U * getB();
  } else
    throw std::invalid_argument("U is not a proper rotation");
}

/** Copy constructor
@param other :: The NiggliCell from which to copy information
*/
NiggliCell::NiggliCell(const NiggliCell &other)
    : UnitCell(other), U(other.U), UB(other.UB) {}

/** Constructor
@param _a :: lattice parameter \f$ a \f$ with \f$\alpha = \beta = \gamma =
90^\circ \f$
@param _b :: lattice parameter \f$ b \f$ with \f$\alpha = \beta = \gamma =
90^\circ \f$
@param _c :: lattice parameter \f$ c \f$ with \f$\alpha = \beta = \gamma =
90^\circ \f$
@param Umatrix :: orientation matrix U
*/
NiggliCell::NiggliCell(const double _a, const double _b, const double _c,
                       const DblMatrix &Umatrix)
    : UnitCell(_a, _b, _c) {
  if (Umatrix.isRotation() == true) {
    U = Umatrix;
    UB = U * getB();
  } else
    throw std::invalid_argument("U is not a proper rotation");
}

/** Constructor
@param _a :: lattice parameter \f$ a \f$
@param _b :: lattice parameter \f$ b \f$
@param _c :: lattice parameter \f$ c \f$
@param _alpha :: lattice parameter \f$ \alpha \f$
@param _beta :: lattice parameter \f$ \beta \f$
@param _gamma :: lattice parameter \f$ \gamma \f$
@param angleunit :: units for angle, of type #AngleUnits. Default is degrees.
@param Umatrix :: orientation matrix U
*/
NiggliCell::NiggliCell(const double _a, const double _b, const double _c,
                       const double _alpha, const double _beta,
                       const double _gamma, const DblMatrix &Umatrix,
                       const int angleunit)
    : UnitCell(_a, _b, _c, _alpha, _beta, _gamma, angleunit) {
  if (Umatrix.isRotation() == true) {
    U = Umatrix;
    UB = U * getB();
  } else
    throw std::invalid_argument("U is not a proper rotation");
}

/** UnitCell constructor
@param uc :: UnitCell
@param Umatrix :: orientation matrix U. By default this will be identity matrix
*/
NiggliCell::NiggliCell(const UnitCell &uc, const DblMatrix &Umatrix)
    : UnitCell(uc), U(Umatrix) {
  if (Umatrix.isRotation() == true) {
    U = Umatrix;
    UB = U * getB();
  } else
    throw std::invalid_argument("U is not a proper rotation");
}

NiggliCell::NiggliCell(const UnitCell *uc, const DblMatrix &Umatrix)
    : UnitCell(uc), U(Umatrix) {
  if (Umatrix.isRotation() == true) {
    U = Umatrix;
    UB = U * getB();
  } else
    throw std::invalid_argument("U is not a proper rotation");
}

/// Destructor
NiggliCell::~NiggliCell() {}

/**
    Check if a,b,c cell has angles satifying Niggli condition within epsilon.
    Specifically, check if all angles are strictly less than 90 degrees,
    or all angles are greater than or equal to 90 degrees.  The inequality
    requirements are relaxed by an amount specified by the paramter epsilon
    to accommodate some experimental and/or rounding error in the calculated
    angles.

    @param a_dir    Vector in the direction of the real cell edge vector 'a'
    @param b_dir    Vector in the direction of the real cell edge vector 'b'
    @param c_dir    Vector in the direction of the real cell edge vector 'c'
    @param epsilon  Tolerance (in degrees) around 90 degrees.  For example
                    an angle theta will be considered strictly less than 90
                    degrees, if it is less than 90+epsilon.
    @return true if all angles are less than 90 degrees, or if all angles
            are greater than or equal to 90 degrees.
 */
bool NiggliCell::HasNiggliAngles(const V3D &a_dir, const V3D &b_dir,
                                 const V3D &c_dir, double epsilon) {
  double alpha = b_dir.angle(c_dir) * RAD_TO_DEG;
  double beta = c_dir.angle(a_dir) * RAD_TO_DEG;
  double gamma = a_dir.angle(b_dir) * RAD_TO_DEG;

  if (alpha < 90 + epsilon && beta < 90 + epsilon && gamma < 90 + epsilon) {
    return true;
  }

  if (alpha >= 90 - epsilon && beta >= 90 - epsilon && gamma >= 90 - epsilon) {
    return true;
  }

  return false;
}

/**
 *  Try to find a UB that is equivalent to the original UB, but corresponds
 * to a Niggli reduced cell with the smallest sum of edge lengths and
 * with angles that are farthest from 90 degrees.
 *
 * @param UB      The original UB
 * @param newUB   Returns the newUB
 *
 * @return True if a possibly constructive change was made and newUB has been
 * set to a new matrix.  It returns false if no constructive change was found
 * and newUB is just set to the original UB.
 */

bool NiggliCell::MakeNiggliUB(const DblMatrix &UB, DblMatrix &newUB) {
  V3D a;
  V3D b;
  V3D c;

  if (!OrientedLattice::GetABC(UB, a, b, c)) {
    return false;
  }

  V3D v1;
  V3D v2;
  V3D v3;
  // first make a list of linear combinations
  // of vectors a,b,c with coefficients up to 5
  std::vector<V3D> directions;
  int N_coeff = 5;
  for (int i = -N_coeff; i <= N_coeff; i++) {
    for (int j = -N_coeff; j <= N_coeff; j++) {
      for (int k = -N_coeff; k <= N_coeff; k++) {
        if (i != 0 || j != 0 || k != 0) {
          v1 = a * i;
          v2 = b * j;
          v3 = c * k;
          V3D sum(v1);
          sum += v2;
          sum += v3;
          directions.push_back(sum);
        }
      }
    }
  }
  // next sort the list of linear combinations
  // in order of increasing length
  std::sort(directions.begin(), directions.end(), V3D::CompareMagnitude);

  // next form a list of possible UB matrices
  // using sides from the list of linear
  // combinations, using shorter directions first.
  // Keep trying more until 25 UBs are found.
  // Only keep UBs corresponding to cells with
  // at least a minimum cell volume
  std::vector<DblMatrix> UB_list;

  size_t num_needed = 25;
  size_t max_to_try = 5;
  while (UB_list.size() < num_needed && max_to_try < directions.size()) {
    max_to_try *= 2;
    size_t num_to_try = std::min(max_to_try, directions.size());

    V3D acrossb;
    double vol = 0;
    double min_vol = .1f; // what should this be? 0.1 works OK, but...?
    for (size_t i = 0; i < num_to_try - 2; i++) {
      a = directions[i];
      for (size_t j = i + 1; j < num_to_try - 1; j++) {
        b = directions[j];
        acrossb = a.cross_prod(b);
        for (size_t k = j + 1; k < num_to_try; k++) {
          c = directions[k];
          vol = acrossb.scalar_prod(c);
          if (vol > min_vol && HasNiggliAngles(a, b, c, 0.01)) {
            Matrix<double> new_tran(3, 3, false);
            OrientedLattice::GetUB(new_tran, a, b, c);
            UB_list.push_back(new_tran);
          }
        }
      }
    }
  }
  // if no valid UBs could be formed, return
  // false and the original UB
  if (UB_list.empty()) {
    newUB = UB;
    return false;
  }
  // now sort the UB's in order of increasing
  // total side length |a|+|b|+|c|
  std::sort(UB_list.begin(), UB_list.end(), CompareABCsum);

  // keep only those UB's with total side length
  // within .1% of the first one.  This can't
  // be much larger or "bad" UBs are made for
  // some tests with 5% noise
  double length_tol = 0.001;
  double total_length;

  std::vector<DblMatrix> short_list;
  short_list.push_back(UB_list[0]);
  OrientedLattice::GetABC(short_list[0], a, b, c);
  total_length = a.norm() + b.norm() + c.norm();

  bool got_short_list = false;
  size_t i = 1;
  while (i < UB_list.size() && !got_short_list) {
    OrientedLattice::GetABC(UB_list[i], v1, v2, v3);
    double next_length = v1.norm() + v2.norm() + v3.norm();
    if (fabs(next_length - total_length) / total_length < length_tol)
      short_list.push_back(UB_list[i]);
    else
      got_short_list = true;
    i++;
  }
  // now sort on the basis of difference of cell
  // angles from 90 degrees and return the one
  // with angles most different from 90
  std::sort(short_list.begin(), short_list.end(), CompareDiffFrom90);

  newUB = short_list[0];

  return true;
}

} // Namespace Geometry
} // Namespace Mantid
