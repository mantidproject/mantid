/* File: ConventionalCell.cpp */

#include <stdexcept>
#include <algorithm>
#include <cstdio>

#include "MantidKernel/V3D.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

namespace Mantid {
namespace Geometry {
using Mantid::Kernel::V3D;
using Mantid::Kernel::DblMatrix;

/**
 *  Construct a ConventionalCell for the specified orientation matrix
 *  and the specified row of Table 2.  The form number must be between
 *  1 and 44.
 *  @param  UB        The orientation matrix corresponding to a Niggli
 *                    reduced cell.
 *  @param  form_num  The row number from Table 2, that specifies the
 *                    reduced form number.
 *  @param allowPermutations Allow permutations of conventional cells for
 *                           related UBs with better fit to peaks.
 */
ConventionalCell::ConventionalCell(const Kernel::DblMatrix &UB, size_t form_num,
                                   bool allowPermutations) {
  form_number = form_num;
  std::vector<double> lat_par;
  IndexingUtils::GetLatticeParameters(UB, lat_par);

  ReducedCell form_0 = ReducedCell(0, lat_par[0], lat_par[1], lat_par[2],
                                   lat_par[3], lat_par[4], lat_par[5]);
  ReducedCell form_i = ReducedCell(form_num, lat_par[0], lat_par[1], lat_par[2],
                                   lat_par[3], lat_par[4], lat_par[5]);
  init(UB, form_0, form_i, allowPermutations);
}

/**
 *  Get the form number corresponding to this conventional cell.
 *
 *  @return the form number for this conventional cell.
 */
size_t ConventionalCell::GetFormNum() const { return form_number; }

/**
 *  Get the error in the scalars for this conventional cell.
 *
 *  @return  The maximum absolute weighted difference between the
 *           scalars for this conventional cell and form 0.
 */
double ConventionalCell::GetError() const { return scalars_error; }

/**
 *  Get the cell type for this conventional cell.
 *
 *  @return a std::string specifying the cell type.
 */
std::string ConventionalCell::GetCellType() const {
  return std::string(cell_type);
}

/**
 *  Get centering for this conventional cell.
 *
 *  @return a std::string specifying the centering type.
 */
std::string ConventionalCell::GetCentering() const {
  return std::string(centering);
}

/**
 *  Get a copy of the original UB matrix that was passed in to the
 *  constructor for this object.
 *
 *  @return  a 3x3 matrix with the original UB matrix
 */
Kernel::DblMatrix ConventionalCell::GetOriginalUB() const {
  return Kernel::DblMatrix(original_UB);
}

/**
 *  Get a copy of the orientation matrix that indexes the peaks in a
 *  way that corresponds to the conventional cell.
 *
 *  @return  a 3x3 matrix with the new UB matrix.
 */
Kernel::DblMatrix ConventionalCell::GetNewUB() const {
  return Kernel::DblMatrix(adjusted_UB);
}

/**
 *  Get a copy of the transform that maps the original HKL values to new
 *  HKL values corresponding to the conventional cell.
 *
 *  @return  a 3x3 matrix with the HKL tranformation.
 */
Kernel::DblMatrix ConventionalCell::GetHKL_Tran() const {
  return Kernel::DblMatrix(hkl_tran);
}

/**
 *  Get the sum of the sides, |a|+|b|+|c| of the conventional cell.
 *
 *  @return The sum of the sides of the conventional cell.
 */
double ConventionalCell::GetSumOfSides() const {
  std::vector<double> lat_par;
  IndexingUtils::GetLatticeParameters(adjusted_UB, lat_par);
  return lat_par[0] + lat_par[1] + lat_par[2];
}

/**
 *  Get a formatted string listing the form number, error in scalars,
 *  the cell type and the centering.
 *
 *  @return a std::string with basic information about this cell.
 */
std::string ConventionalCell::GetDescription() const {
  char buffer[100];
  sprintf(buffer, std::string("Form #%2d").c_str(), GetFormNum());
  std::string message(buffer);

  sprintf(buffer, std::string("  Error:%7.4f").c_str(), GetError());
  message += std::string(buffer);

  sprintf(buffer, std::string("  %-12s").c_str(), GetCellType().c_str());
  message += std::string(buffer);

  sprintf(buffer, std::string("  %1s  ").c_str(), GetCentering().c_str());
  message += std::string(buffer);

  return message;
}

/**
 *  Initialize the fields of this ConventionalCell object, using
 *  a specified matrix and two forms.
 *
 *  @param UB       The orientation matrix for the Niggli cell
 *  @param form_0   The reduced cell form built with the lattice parameters
 *                  for UB and form number zero.
 *  @param form_i   The reduced cell form built with the lattice parameters
 *                  for UB and the form number of the desired conventional
 *                  cell.
 *  @param allowPermutations Allow permutations of conventional cells for
 *                           related UBs with better fit to peaks.
 */
void ConventionalCell::init(const Kernel::DblMatrix &UB, ReducedCell &form_0,
                            ReducedCell &form_i, bool allowPermutations) {
  scalars_error = form_0.WeightedDistance(form_i);
  cell_type = form_i.GetCellType();
  centering = form_i.GetCentering();

  original_UB = Kernel::DblMatrix(UB);

  hkl_tran = form_i.GetTransformation();

  Kernel::DblMatrix UB_tran(hkl_tran);
  UB_tran.Invert();
  adjusted_UB = UB * UB_tran;
  if (allowPermutations) {
    if (cell_type == ReducedCell::ORTHORHOMBIC()) {
      SetSidesIncreasing(adjusted_UB);
    } else if (cell_type == ReducedCell::TETRAGONAL()) {
      StandardizeTetragonal(adjusted_UB);
    } else if (cell_type == ReducedCell::HEXAGONAL() ||
               cell_type == ReducedCell::RHOMBOHEDRAL()) {
      StandardizeHexagonal(adjusted_UB);
    }
  }
}

/**
 *  Change UB to a new matrix corresponding to a unit cell with the sides
 *  in increasing order of magnitude.  This is used to arrange the UB matrix
 *  for an orthorhombic cell into a standard order.
 *
 *  @param UB on input this should correspond to an orthorhombic cell.
 *            On output, it will correspond to an orthorhombic cell with
 *            sides in increasing order.
 */
void ConventionalCell::SetSidesIncreasing(Kernel::DblMatrix &UB) {
  V3D a_dir;
  V3D b_dir;
  V3D c_dir;
  OrientedLattice::GetABC(UB, a_dir, b_dir, c_dir);

  std::vector<V3D> edges;
  edges.push_back(a_dir);
  edges.push_back(b_dir);
  edges.push_back(c_dir);
  std::sort(edges.begin(), edges.end(), V3D::CompareMagnitude);

  V3D a = edges[0];
  V3D b = edges[1];
  V3D c = edges[2];

  V3D acrossb = a.cross_prod(b); // keep a,b,c right handed
  if (acrossb.scalar_prod(c) < 0) {
    c = c * (-1);
  }
  OrientedLattice::GetUB(UB, a, b, c);
}

/**
 *  Change UB to a new matrix corresponding to a unit cell with the first
 *  two sides approximately equal in magnitude.  This is used to arrange
 *  the UB matrix for a tetragonal cell into a standard order.
 *
 *  @param UB on input this should correspond to a tetragonal cell.
 *            On output, it will correspond to a tetragonal cell with the
 *            first two sides, a and b, set to the two sides that are most
 *            nearly equal in length.
 */
void ConventionalCell::StandardizeTetragonal(Kernel::DblMatrix &UB) {
  V3D a;
  V3D b;
  V3D c;
  OrientedLattice::GetABC(UB, a, b, c);

  double a_b_diff = fabs(a.norm() - b.norm()) / std::min(a.norm(), b.norm());

  double a_c_diff = fabs(a.norm() - c.norm()) / std::min(a.norm(), c.norm());

  double b_c_diff = fabs(b.norm() - c.norm()) / std::min(b.norm(), c.norm());

  // if needed, change UB to have the two most nearly
  // equal sides first.
  if (a_c_diff <= a_b_diff && a_c_diff <= b_c_diff) {
    OrientedLattice::GetUB(UB, c, a, b);
  } else if (b_c_diff <= a_b_diff && b_c_diff <= a_c_diff) {
    OrientedLattice::GetUB(UB, b, c, a);
  }
}

/**
 *  Change UB to a new matrix corresponding to a hexagonal unit cell with
 *  angles approximately 90, 90, 120.  This is used to arrange
 *  the UB matrix for a hexagonal or rhombohedral cell into a standard order.
 *
 *  @param UB on input this should correspond to a hexagonal or rhombohedral
 *            On output, it will correspond to a hexagonal cell with angles
 *            approximately 90, 90, 120.
 */
void ConventionalCell::StandardizeHexagonal(Kernel::DblMatrix &UB) {
  V3D a;
  V3D b;
  V3D c;
  OrientedLattice::GetABC(UB, a, b, c);

  double alpha = b.angle(c) * 180.0 / M_PI;
  double beta = c.angle(a) * 180.0 / M_PI;
  // first, make the non 90
  // degree angle last
  if (fabs(alpha - 90) > 20) {
    OrientedLattice::GetUB(UB, b, c, a);
  } else if (fabs(beta - 90) > 20) {
    OrientedLattice::GetUB(UB, c, a, b);
  }
  // if the non 90 degree angle
  // is about 60 degrees, make
  // it about 120 degrees.
  OrientedLattice::GetABC(UB, a, b, c);
  double gamma = a.angle(b) * 180.0 / M_PI;
  if (fabs(gamma - 60) < 10) {
    a = a * (-1);                        // reflect a and c to change
    c = c * (-1);                        // alpha and gamma to their
    OrientedLattice::GetUB(UB, a, b, c); // supplementary angle
  }
}

} // namespace Mantid
} // namespace Geometry
