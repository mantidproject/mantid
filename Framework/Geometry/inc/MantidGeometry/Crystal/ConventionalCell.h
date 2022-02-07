// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/ReducedCell.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

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
 */

class MANTID_GEOMETRY_DLL ConventionalCell {
public:
  /// Construct a ConventionalCell for the specified UB and form number
  ConventionalCell(const Kernel::DblMatrix &UB, size_t form_num = 0, bool allowPermutations = false);

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
  void init(const Kernel::DblMatrix &UB, ReducedCell &form_0, ReducedCell &form_i, bool allowPermutations);
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

} // namespace Geometry
} // namespace Mantid
