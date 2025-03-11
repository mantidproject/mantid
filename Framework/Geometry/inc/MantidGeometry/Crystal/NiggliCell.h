// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/UnitCell.h"
#include "MantidNexus/NeXusFile.hpp"

namespace Mantid {
namespace Geometry {
/** @class NiggliCell NiggliCell.h Geometry/Crystal/NiggliCell.h
Class to implement UB matrix.
See documentation about UB matrix in the Mantid repository.\n

@author Andrei Savici, SNS, ORNL
@date 2011-04-15
*/
class MANTID_GEOMETRY_DLL NiggliCell : public UnitCell {
public:
  // Default constructor. a = b = c = 1, alpha = beta = gamma = 90 degrees
  NiggliCell(const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3, true));
  // a,b,c constructor
  NiggliCell(const double _a, const double _b, const double _c,
             const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3, true));
  // a,b,c,alpha,beta,gamma constructor
  NiggliCell(const double _a, const double _b, const double _c, const double _alpha, const double _beta,
             const double _gamma, const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3, true),
             const int angleunit = angDegrees);
  // UnitCell constructor
  NiggliCell(const UnitCell &uc, const Kernel::DblMatrix &Umatrix = Kernel::DblMatrix(3, 3, true));
  // Access private variables
  /// Check if a,b,c cell has angles satifying Niggli condition within epsilon
  static bool HasNiggliAngles(const Kernel::V3D &a_dir, const Kernel::V3D &b_dir, const Kernel::V3D &c_dir,
                              double epsilon);

  /// Construct a newUB corresponding to a Niggli cell from the given UB
  static bool MakeNiggliUB(const Kernel::DblMatrix &UB, Kernel::DblMatrix &newUB);

private:
  Kernel::DblMatrix U;
  Kernel::DblMatrix UB;
};
} // namespace Geometry
} // namespace Mantid
