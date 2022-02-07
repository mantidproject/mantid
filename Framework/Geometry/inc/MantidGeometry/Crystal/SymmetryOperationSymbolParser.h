// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/**
  @class SymmetryOperationSymbolParser

  This is a parser for symmetry operation symbols in the Jones
  faithful representation. It creates matrix and a vector component
  from the given symbol. First an example with no translational component,
  the inversion:

      -x,-y,-z

  Parsing this symbol returns the following matrix/vector pair:

       Matrix    Vector
      -1  0  0     0
       0 -1  0     0
       0  0 -1     0

  Translational components, as required for screw axes and glide planes
  are given as rational numbers, such as in this 2_1 screw axis around z:

      -x,-y,z+1/2

  Which returns the following matrix/vector pair:

       Matrix    Vector
      -1  0  0     0
       0 -1  0     0
       0  0  1    1/2

  From these components, a SymmetryOperation object can be constructed.
  See the documentation for SymmetryOperation and SymmetryOperationFactory.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 30/09/2014
*/

class MANTID_GEOMETRY_DLL SymmetryOperationSymbolParser {
public:
  static MatrixVectorPair<int, V3R> parseIdentifier(const std::string &identifier);
  static std::string getNormalizedIdentifier(const MatrixVectorPair<int, V3R> &data);
  static std::string getNormalizedIdentifier(const Kernel::IntMatrix &matrix, const V3R &vector);

protected:
  SymmetryOperationSymbolParser() = default;

  static void verifyMatrix(const Kernel::IntMatrix &matrix);
  static bool isValidMatrixRow(const int *element, size_t columnNumber);
};

} // namespace Geometry
} // namespace Mantid
