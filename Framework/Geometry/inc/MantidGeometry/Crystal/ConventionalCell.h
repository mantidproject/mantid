#ifndef MANTID_GEOMETRY_CONVENTIONAL_CELL_H_
#define MANTID_GEOMETRY_CONVENTIONAL_CELL_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include "MantidGeometry/Crystal/ReducedCell.h"

namespace Mantid {
namespace Geometry {
/**
    @class ConventionalCell

    Instances of this class represent information about a selected
    conventional cell based on a specified UB for a Niggli reduced cell.
    An instance of the class records the original UB matrix, as well as the
    new UB for the conventional cell, the maximum error in the scalars,
    cell type and centering.
    See "Lattice Symmetry and Identification -- The Fundamental Role of Reduced
     Cells in Materials Characterization", Alan D. Mighell, Vol. 106, Number 6,
    Nov-Dec 2001, Journal of Research of the National Institute of Standards
    and Technology.  This class is a direct port of the ISAW class
    ReducedCellInfo.

    @author Dennis Mikkelson
    @date   2012-01-10

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

class MANTID_GEOMETRY_DLL ConventionalCell {
public:
  /// Construct a ConventionalCell for the specified UB and form number
  ConventionalCell(const Kernel::DblMatrix &UB, size_t form_num = 0,
                   bool allowPermutations = false);

  /// get the form number for this conventional cell
  size_t GetFormNum() const;

  /// get the error in the scalars for this form
  double GetError() const;

  /// get the cell type name, as named in the ReducedCell class
  std::string GetCellType() const;

  /// get the centering type name, as named in the ReducedCell class
  std::string GetCentering() const;

  /// get the original orientation matrix as passed in to the constructor
  Kernel::DblMatrix GetOriginalUB() const;

  /// get the transformed orientation matrix for the conventional cell
  Kernel::DblMatrix GetNewUB() const;

  /// get the transform to change HKL to new conventional cell HKL
  Kernel::DblMatrix GetHKL_Tran() const;

  /// get the sum of the sides of the conventional unit cell
  double GetSumOfSides() const;

  /// get string listing form number, error, cell type and centering
  std::string GetDescription() const;

private:
  void init(const Kernel::DblMatrix &UB, ReducedCell &form_0,
            ReducedCell &form_i, bool allowPermutations);
  void SetSidesIncreasing(Kernel::DblMatrix &UB);
  void StandardizeTetragonal(Kernel::DblMatrix &UB);
  void StandardizeHexagonal(Kernel::DblMatrix &UB);

  size_t form_number;
  double scalars_error;
  std::string cell_type;
  std::string centering;
  Kernel::DblMatrix original_UB;
  Kernel::DblMatrix adjusted_UB;
  Kernel::DblMatrix hkl_tran;
};

} // namespace Mantid
} // namespace Geometry

#endif /* MANTID_GEOMETRY_CONVENTIONAL_CELL_H_ */
