/* File: Scalar_Utils.h */

#ifndef MANTID_GEOMETRY_SCALAR_UTILS_H_
#define MANTID_GEOMETRY_SCALAR_UTILS_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/ConventionalCell.h"

namespace Mantid {
namespace Geometry {
/**
    @class ScalarUtils

    This class contains static utility methods for determining an orientation
    matrix corresponding to a conventional, given the orientation matrix
    corresponding to the Niggli reduced cell.

    @author Dennis Mikkelson
    @date   2012-01-12

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

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

    File change history is stored at:
                 <https://github.com/mantidproject/mantid>

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

class MANTID_GEOMETRY_DLL ScalarUtils {
public:
  /// Get list of all possible conventional cells for UB, regardless of errors,
  /// using this UB, and three related "almost Niggli" cells obtained by
  /// reflecting pairs of sides a, b, c.
  static std::vector<ConventionalCell> GetCells(const Kernel::DblMatrix &UB,
                                                bool best_only,
                                                bool allowPermutations = false);

  /// Get list of conventional cells for UB with specified type and centering,
  /// using this UB, and three related "almost Niggli" cells obtained by
  /// reflecting pairs of sides a, b, c.
  static std::vector<ConventionalCell> GetCells(const Kernel::DblMatrix &UB,
                                                const std::string &cell_type,
                                                const std::string &centering,
                                                bool allowPermutations = false);

  /// Get list of conventional cells for UB with specified type and centering,
  /// using ONLY this UB, not related reflected cells.
  static std::vector<ConventionalCell>
  GetCellsUBOnly(const Kernel::DblMatrix &UB, const std::string &cell_type,
                 const std::string &centering, bool allowPermutations = false);

  /// Get the best conventional cell for the form number, using the specified
  /// UB, and three related "almost Niggli" cells obtained by reflecting
  /// pairs of sides a, b, c.
  static ConventionalCell GetCellForForm(const Kernel::DblMatrix &UB,
                                         size_t form_num,
                                         bool allowPermutations = false);
  /// Remove cells from list that have scalar errors above the specified level
  static void RemoveHighErrorForms(std::vector<ConventionalCell> &list,
                                   double level);

  /// Get the cell from the list with the smallest error, possibly excluding
  /// triclinic cells
  static ConventionalCell
  GetCellBestError(const std::vector<ConventionalCell> &list,
                   bool use_triclinic);

  /// Get list of related cells obtained by reflecting pairs of sides with
  /// nearly a 90 degree angle between the sides, and permuting sides.
  static std::vector<Kernel::DblMatrix>
  GetRelatedUBs(const Kernel::DblMatrix &UB, double factor,
                double angle_tolerance);

private:
  /// Add conventional cell to list if it has the least error for its form num
  static void AddIfBest(std::vector<ConventionalCell> &list,
                        ConventionalCell &info);
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SCALAR_UTILS_H_ */
