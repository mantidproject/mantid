/* File: ScalarUtils.cpp */

#include <iostream>
#include <stdexcept>
#include "MantidGeometry/Crystal/ScalarUtils.h"
#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidGeometry/Crystal/IndexingUtils.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;
using Mantid::Kernel::Matrix;
using Mantid::Kernel::DblMatrix;

static const std::string BRAVAIS_TYPE[15] = {
    ReducedCell::CUBIC(), // F
    ReducedCell::CUBIC(), // I
    ReducedCell::CUBIC(), // P

    ReducedCell::HEXAGONAL(), // P

    ReducedCell::RHOMBOHEDRAL(), // R

    ReducedCell::TETRAGONAL(), // I
    ReducedCell::TETRAGONAL(), // P

    ReducedCell::ORTHORHOMBIC(), // F
    ReducedCell::ORTHORHOMBIC(), // I
    ReducedCell::ORTHORHOMBIC(), // C
    ReducedCell::ORTHORHOMBIC(), // P

    ReducedCell::MONOCLINIC(), // C
    ReducedCell::MONOCLINIC(), // I
    ReducedCell::MONOCLINIC(), // P

    ReducedCell::TRICLINIC() // P
};

static const std::string BRAVAIS_CENTERING[15] = {
    ReducedCell::F_CENTERED(), // cubic
    ReducedCell::I_CENTERED(), // cubic
    ReducedCell::P_CENTERED(), // cubic

    ReducedCell::P_CENTERED(), // hexagonal

    ReducedCell::R_CENTERED(), // rhombohedral

    ReducedCell::I_CENTERED(), // tetragonal
    ReducedCell::P_CENTERED(), // tetragonal

    ReducedCell::F_CENTERED(), // orthorhombic
    ReducedCell::I_CENTERED(), // orthorhombic
    ReducedCell::C_CENTERED(), // orthorhombic
    ReducedCell::P_CENTERED(), // orthorhombic

    ReducedCell::C_CENTERED(), // monoclinic
    ReducedCell::I_CENTERED(), // monoclinic
    ReducedCell::P_CENTERED(), // monoclinic

    ReducedCell::P_CENTERED() // triclinic
};

/**
 *  Get a list of cell info objects, corresponding to all forms that match
 *  the specified UB, or related UBs, with pairs of edges reflected.  If the
 *  same form number matches several times when different pairs of edges are
 *  reflected, only the one with the smallest error value will be included.
 *  A pair of edges will be reflected if the angle between the edges
 *  is within 2 degrees of 90 degrees.  This is needed to take care
 *  of the case where a positive Niggli cell was found, but due to errors in
 *  the data, a negative Niggli cell should have been found, and visa-versa.
 *
 *  @param UB         The lattice parameters for this UB matrix and matrices
 *                    related to it by reflecting pairs of sides, are
 *                    used to form the list of possible conventional cells.
 *  @param best_only  If true, only include the best form for each Bravais
 *                    lattice.
 *  @param allowPermutations Allow permutations of conventional cells for
 *                           related UBs with better fit to peaks.
 *
 *  @return a vector of conventional cell info objects, corresponding to the
 *          best matching forms for UB and cells related to UB by reflections
 *          of pairs of cell edges.
 */
std::vector<ConventionalCell> ScalarUtils::GetCells(const DblMatrix &UB,
                                                    bool best_only,
                                                    bool allowPermutations) {
  std::vector<ConventionalCell> result;

  size_t num_lattices = 15;
  for (size_t i = 0; i < num_lattices; i++) {
    std::vector<ConventionalCell> temp =
        GetCells(UB, BRAVAIS_TYPE[i], BRAVAIS_CENTERING[i], allowPermutations);
    if (best_only) {
      ConventionalCell info = GetCellBestError(temp, true);
      temp.clear();
      temp.push_back(info);
    }
    for (size_t k = 0; k < temp.size(); k++)
      AddIfBest(result, temp[k]);
  }

  return result;
}

/**
 *  Get a list of cell info objects, corresponding to all forms that match
 *  the specified UB, or related UBs, with pairs of edges reflected, and
 *  have the specified cell type and centering.  If the
 *  same form number matches several times when different pairs of edges are
 *  reflected, only the one with the smallest error value will be included.
 *  A pair of edges will be reflected if the angle between the edges
 *  is within 2 degrees of 90 degrees.  This is needed to take care
 *  of the case where a positive Niggli cell was found, but due to errors in
 *  the data, a negative Niggli cell should have been found, and visa-versa.
 *
 *  @param UB         The lattice parameters for this UB matrix and matrices
 *                    related to it by reflecting pairs of sides, are
 *                    used to form the list of possible conventional cells.
 *  @param cell_type  String specifying the cell type, as listed in the
 *                    ReducedCell class.
 *  @param centering  String specifying the centering, as listed in the
 *                    ReducedCell class.
 *  @param allowPermutations Allow permutations of conventional cells for
 *                           related UBs with better fit to peaks.
 *
 *  @return a vector of conventional cell objects, for the specified
 *          cell type and centering, corresponding to the
 *          best matching forms for UB and cells related to UB by reflections
 *          of pairs of cell edges.
 */
std::vector<ConventionalCell>
ScalarUtils::GetCells(const DblMatrix &UB, const std::string &cell_type,
                      const std::string &centering, bool allowPermutations) {
  std::vector<ConventionalCell> result;

  std::vector<DblMatrix> UB_list;
  if (allowPermutations) {
    double angle_tolerance = 2.0;
    double length_factor = 1.05;
    UB_list = GetRelatedUBs(UB, length_factor, angle_tolerance);
  } else {
    // Get exact form requested and not permutations
    UB_list.push_back(UB);
  }

  for (size_t k = 0; k < UB_list.size(); k++) {
    std::vector<ConventionalCell> temp =
        GetCellsUBOnly(UB_list[k], cell_type, centering, allowPermutations);

    for (size_t i = 0; i < temp.size(); i++)
      AddIfBest(result, temp[i]);
  }

  return result;
}

/**
 *  Get a list of cell info objects that correspond to the specific given
 *  UB matrix.  The list will have at most one instance of any matching form
 *  since only the specified UB matrix is used (no reflections or other
 *  related cell modifications).  For any form with the specified cell type
 *  and centering, a cell info object will be added to the list, regardless
 *  of the error in cell scalars.  As a result, the list returned by this
 *  method will often have forms that don't fit well.  However, the list
 *  will be a complete list of matching forms for the specified UB, cell_type
 *  and centering.  Poorly matching entries can be removed subsequently by
 *  the calling code, if need be.
 *
 *  @param UB         The lattice parameters for this UB matrix are
 *                    used to form the list of possible conventional cells.
 *  @param cell_type  String specifying the cell type, as listed in the
 *                    ReducedCell class.
 *  @param centering  String specifying the centering, as listed in the
 *                    ReducedCell class.
 *  @param allowPermutations Allow permutations of conventional cells for
 *                           related UBs with better fit to peaks.
 *  @return a list of conventional cells for the specified UB, of the
 *          specified type and centering.
 */
std::vector<ConventionalCell>
ScalarUtils::GetCellsUBOnly(const DblMatrix &UB, const std::string &cell_type,
                            const std::string &centering,
                            bool allowPermutations) {
  std::vector<ConventionalCell> result;

  std::vector<double> lat_par;
  IndexingUtils::GetLatticeParameters(UB, lat_par);

  for (size_t i = 0; i <= ReducedCell::NUM_CELL_TYPES; i++) {
    ReducedCell rcell(i, lat_par[0], lat_par[1], lat_par[2], lat_par[3],
                      lat_par[4], lat_par[5]);

    if (rcell.GetCentering() == centering && rcell.GetCellType() == cell_type) {
      ConventionalCell cell_info(UB, i, allowPermutations);
      result.push_back(cell_info);
    }
  }

  return result;
}

/**
 *  Get one cell, the for specified UB and form, by trying different
 *  reflections of a,b,c and forming the ConventionalCellInfo object
 *  corresponding to the smallest form error.
 *
 *  @param UB        Crystal::Orientation transformation corresponding to a
 *Niggli
 *                   reduced cell.
 *  @param form_num  The form number to use.
 *  @param allowPermutations Allow permutations of conventional cells for
 *                           related UBs with better fit to peaks.
 *
 *  @return A ConventionalCellInfo object corresponding to the specified
 *          form number and UB (or a related matrix) with the smallest
 *          error of any related matrix.
 */
ConventionalCell ScalarUtils::GetCellForForm(const DblMatrix &UB,
                                             size_t form_num,
                                             bool allowPermutations) {
  ConventionalCell info(UB);
  ReducedCell form_0;
  ReducedCell form;

  double min_error = 1e20; // errors are usually < 10, so this is big enough

  std::vector<double> l_params;
  std::vector<DblMatrix> UB_list;
  if (allowPermutations) {
    double angle_tolerance = 2.0;
    double length_factor = 1.05;
    UB_list = GetRelatedUBs(UB, angle_tolerance, length_factor);
  } else {
    // Get exact form requested and not permutations
    UB_list.push_back(UB);
  }
  for (size_t i = 0; i < UB_list.size(); i++) {
    IndexingUtils::GetLatticeParameters(UB_list[i], l_params);

    form_0 = ReducedCell(0, l_params[0], l_params[1], l_params[2], l_params[3],
                         l_params[4], l_params[5]);

    form = ReducedCell(form_num, l_params[0], l_params[1], l_params[2],
                       l_params[3], l_params[4], l_params[5]);

    double error = form_0.WeightedDistance(form);
    if (error < min_error) {
      info = ConventionalCell(UB_list[i], form_num, allowPermutations);
      min_error = error;
    }
  }

  return info;
}

/**
 *  Remove any forms from the list that have errors that are above the
 *  specified level.
 *
 *  @param list   The list of conventional cell objects to be cleaned up.
 *  @param level  This specifies the maximum error for cells that will
 *                not be discarded from the list.
 */
void ScalarUtils::RemoveHighErrorForms(std::vector<ConventionalCell> &list,
                                       double level) {
  if (list.size() <= 0) // nothing to do
    return;

  std::vector<ConventionalCell> new_list;

  for (size_t i = 0; i < list.size(); i++)
    if (list[i].GetError() <= level)
      new_list.push_back(list[i]);

  list.clear();
  for (size_t i = 0; i < new_list.size(); i++)
    list.push_back(new_list[i]);
}

/**
 * Get the cell info object that has the smallest error of any of the
 * cells in the list.  If use_triclinc is false, this will skip triclinic
 * cells in the list, and return the non-triclinic cell with the smallest
 * error of all non-triclinic cells.  If there is no such cell, or if the
 * list is empty, this will throw an exception.
 *
 * @param list           The list of conventional cell info objects.
 * @param use_triclinic  If false, skip any triclinic cells in the
 *                       list when checking for smallest error.
 *
 * @return The entry in the list with the smallest error.
 */
ConventionalCell
ScalarUtils::GetCellBestError(const std::vector<ConventionalCell> &list,
                              bool use_triclinic) {
  if (list.size() == 0) {
    throw std::invalid_argument("GetCellBestError(): list is empty");
  }

  ConventionalCell info = list[0];
  double min_error = 1.0e20;
  std::string type;

  bool min_found = false;
  for (size_t i = 0; i < list.size(); i++) {
    type = list[i].GetCellType();
    double error = list[i].GetError();
    if ((use_triclinic || type != ReducedCell::TRICLINIC()) &&
        error < min_error) {
      info = list[i];
      min_error = error;
      min_found = true;
    }
  }

  if (!min_found) {
    throw std::invalid_argument(
        "GetCellBestError(): no allowed form with min error");
  }

  return info;
}

/**
 *
 *  Get a list of UB matrices that are related to the specified UB.
 *  The related UBs are generated by both reflecting pairs of
 *  of the unit cell edge vectors a,b,c and permuting the resulting
 *  edge vectors in ways that preserve the handedness of the cell.
 *  For example replacing a,b,c with b,c,a would preserve the handedness,
 *  but replacing a,b,c with a,c,b would not.  In this case we would also
 *  need to negate side a, so we would replace a,b,c with -a,c,b.  A
 *  permutation of the sides is only used provided the sides are still
 *  "essentially" in increasing order.  The specified factor provides a
 *  tolerance to relax the restriction that |a|<=|b|<=|c|.  As long as a
 *  side is less than or equal to the following side times the specified
 *  factor, it is considered less than or equal to the following side,
 *  for purposes of checking whether or not the list of sides could form
 *  a Niggli cell.  This provides a tolerance for experimental error.
 *  Two sides will be reflected across the origin if the angle between
 *  them is within the specified angle_tolerance of 90 degrees.  This will
 *  help find the correct Niggli cell for cases where the angle is near
 *  90 degrees.  In particular, if a positive Niggli cell was found with
 *  an angle near 90 degrees, due to errors in the data, it possibly should
 *  have been a negative Niggli cell.  Adding the cases with reflected sides
 *  for angles near 90 degrees will include the opposite sign Niggli cell
 *  so it is also checked.
 *
 *  @param UB               The original matrix (should be for Niggli cell)
 *  @param factor           Tolerance for error in real-space cell edge
 *                          lengths.
 *  @param angle_tolerance  Tolerance for angles near 90 degree.
 *
 *  @return  A vector of UB matrices related to the original UB matrix
 *           by reflections of pairs of the side vectors a, b, c.
 */
std::vector<DblMatrix> ScalarUtils::GetRelatedUBs(const DblMatrix &UB,
                                                  double factor,
                                                  double angle_tolerance) {
  std::vector<DblMatrix> result;

  V3D a, b, c, a_vec, b_vec, c_vec, // vectors for generating reflections
      m_a_vec, m_b_vec, m_c_vec;    // of pairs of sides

  V3D a_temp, b_temp, c_temp,       // vectors for generating handedness
      m_a_temp, m_b_temp, m_c_temp; // preserving permutations of sides

  OrientedLattice::GetABC(UB, a_vec, b_vec, c_vec);

  m_a_vec = a_vec * (-1.0);
  m_b_vec = b_vec * (-1.0);
  m_c_vec = c_vec * (-1.0);
  // make list of reflections of all pairs
  // of sides.  NOTE: These preserve the
  // ordering of magnitudes: |a|<=|b|<=|c|
  V3D reflections[4][3] = {{a_vec, b_vec, c_vec},
                           {m_a_vec, m_b_vec, c_vec},
                           {m_a_vec, b_vec, m_c_vec},
                           {a_vec, m_b_vec, m_c_vec}};

  // make list of the angles that are not
  // changed by each of the reflections.  IF
  // that angle is close to 90 degrees, then
  // we may need to switch between all angles
  // >= 90 and all angles < 90.  An angle
  // near 90 degrees may be mis-categorized
  // due to errors in the data.
  double alpha = b_vec.angle(c_vec) * 180.0 / M_PI;
  double beta = c_vec.angle(a_vec) * 180.0 / M_PI;
  double gamma = a_vec.angle(b_vec) * 180.0 / M_PI;
  double angles[4] = {90.0, gamma, beta, alpha};

  for (size_t row = 0; row < 4; row++) {
    if (fabs(angles[row] - 90.0) < angle_tolerance) // if nearly 90,
    {                                               // try related cell
      a_temp = reflections[row][0];                 // +cell <-> -cell
      b_temp = reflections[row][1];
      c_temp = reflections[row][2];
      // for each accepted reflection, try all
      // modified premutations that preserve the
      // handedness AND keep the cell edges
      // nearly ordered as a <= b <= c.
      m_a_temp = a_temp * (-1.0);
      m_b_temp = b_temp * (-1.0);
      m_c_temp = c_temp * (-1.0);

      V3D permutations[6][3] = {{a_temp, b_temp, c_temp},
                                {m_a_temp, c_temp, b_temp},
                                {b_temp, c_temp, a_temp},
                                {m_b_temp, a_temp, c_temp},
                                {c_temp, a_temp, b_temp},
                                {m_c_temp, b_temp, a_temp}};

      for (size_t perm = 0; perm < 6; perm++) {
        a = permutations[perm][0];
        b = permutations[perm][1];
        c = permutations[perm][2];
        if (a.norm() <= factor * b.norm() &&
            b.norm() <= factor * c.norm()) // could be Niggli within
        {                                  // experimental error
          Matrix<double> temp_UB(3, 3, false);
          OrientedLattice::GetUB(temp_UB, a, b, c);
          result.push_back(temp_UB);
        }
      }
    }
  }

  return result;
}

/**
 *  Add the conventional cell record to the list if there is not
 *  already an entry with the same form number, or replace the entry
 *  that has the same form number with the specified cell info, if
 *  the specified cell info has a smaller error.  NOTE: The list must
 *  initially contain at most one entry with the same form number as the
 *  specified ConventionalCellInfo object.  That list property will be
 *  maintained by this method.
 *
 *  @param list   The initial list of ConventionalCell objects.
 *  @param info   The new ConventionalCell object that might be added to
 *                the list.
 */
void ScalarUtils::AddIfBest(std::vector<ConventionalCell> &list,
                            ConventionalCell &info) {
  size_t form_num = info.GetFormNum();
  double new_error = info.GetError();
  bool done = false;
  size_t i = 0;
  while (!done && i < list.size()) {
    if (list[i].GetFormNum() == form_num) // if found, replace if better
    {
      done = true;
      if (list[i].GetError() > new_error)
        list[i] = info;
    } else
      i++;
  }

  if (!done) // if never found, add to end of list
    list.push_back(info);
}
