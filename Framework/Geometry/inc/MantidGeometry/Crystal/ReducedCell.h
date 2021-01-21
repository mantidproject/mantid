// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include <string>

namespace Mantid {
namespace Geometry {
/**
    @class ReducedCell

    Instances of this class represent information about reduced cell types
    including the transformation required to transform the reduced cell to
    a conventional cell.  Essentially, each instance of this class represents
    one row of information from Table 2 in the paper:
    "Lattice Symmetry and Identification -- The Fundamental Role of Reduced
     Cells in Materials Characterization", Alan D. Mighell, Vol. 106, Number 6,
    Nov-Dec 2001, Journal of Research of the National Institute of Standards
    and Technology.  This class is a direct port of the ISAW class
    ReducedCellInfo.

    @author Dennis Mikkelson
    @date   2012-01-02
 */

class MANTID_GEOMETRY_DLL ReducedCell {
public:
  // Construct a ReducedCell for the specified form and lattice parameters
  ReducedCell(size_t form_num = 0, double a = 1, double b = 1, double c = 1, double alpha = 90, double beta = 90,
              double gamma = 90);

  size_t GetFormNum() const;
  std::string GetCellType() const;
  std::string GetCentering() const;

  // Get the "distance" between the scalars for this form and another form
  double WeightedDistance(const ReducedCell &other) const;

  // Get transformation between the Niggli cell and the conventional cell
  Kernel::DblMatrix GetTransformation();

  enum { NUM_CELL_TYPES = 44 };

  // String constants for cell types
  static const std::string NONE() { return "None"; }
  static const std::string CUBIC() { return "Cubic"; }
  static const std::string HEXAGONAL() { return "Hexagonal"; }
  static const std::string RHOMBOHEDRAL() { return "Rhombohedral"; }
  static const std::string TETRAGONAL() { return "Tetragonal"; }
  static const std::string ORTHORHOMBIC() { return "Orthorhombic"; }
  static const std::string MONOCLINIC() { return "Monoclinic"; }
  static const std::string TRICLINIC() { return "Triclinic"; }

  // String constants for centerings
  static const std::string F_CENTERED() { return "F"; }
  static const std::string I_CENTERED() { return "I"; }
  static const std::string C_CENTERED() { return "C"; }
  static const std::string P_CENTERED() { return "P"; }
  static const std::string R_CENTERED() { return "R"; }

private:
  void init(size_t f_num, double a_a, double b_b, double c_c, double b_c, double a_c, double a_b);
  void foot_note_b(double a_a, double a_c);
  void foot_note_c(double b_b, double b_c);
  void foot_note_d(double c_c, double b_c);
  void foot_note_e(double a_a, double c_c, double a_c);
  void foot_note_f(double b_b, double c_c, double b_c);
  void premultiply(size_t index);
  std::vector<double> norm_vals(const ReducedCell &info) const;

  size_t form_num;
  double scalars[6];
  Kernel::DblMatrix transform;
  std::string cell_type;
  std::string centering;
};

} // namespace Geometry
} // namespace Mantid
