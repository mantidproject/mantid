#ifndef MANTID_GEOMETRY_SYMMETRYOPERATION_H_
#define MANTID_GEOMETRY_SYMMETRYOPERATION_H_

#include "MantidGeometry/Crystal/MatrixVectorPair.h"
#include "MantidGeometry/Crystal/V3R.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {

/**
    @class SymmetryOperation

    Crystallographic symmetry operations are composed of a rotational component,
    which is represented by a matrix and a translational part, which is
    described by a vector.

    In this interface, each symmetry operation has a so-called order,
    which is an unsigned integer describing the number of times a
    symmetry operation has to be applied to an object until it is identical.

    Furthermore, each symmetry operation has a string-identifier. It contains
    the Jones faithful representation of the operation, as it is commonly used
    in many crystallographic programs and of course the International Tables
    for Crystallography, where the symmetry operations and their representations
    may be found [1].

    The Jones faithful notation is a very concise way of describing
    matrix/vector pairs. The matrix/vector pair of a two-fold rotation
    axis along z is for example:

        Matrix      Vector
        -1  0  0     0
         0 -1  0     0
         0  0  1     0

    This is described by the symbol '-x,-y,z'. If it were a 2_1 screw axis
    in the same direction, the matrix/vector pair would look like this:

        Matrix      Vector
        -1  0  0     0
         0 -1  0     0
         0  0  1    1/2

    And this is represented by the string '-x,-y,z+1/2'. In hexagonal systems
    there are often operations involving 1/3 or 2/3, so the translational part
    is kept as a vector of rational numbers in order to carry out precise
    calculations. For details, see the class Kernel::V3R.

    Using the symmetry operations in code is easy, since
    SymmetryOperationSymbolParser is automatically called by the string-based
    constructor of SymmetryOperation and the multiplication operator
    is overloaded:

        SymmetryOperation inversion("-x,-y,-z");
        V3D transformed = inversion * V3D(1, 1, -1); // results in -1, -1, 1

    The operator is templated and works for any object Kernel::IntMatrix can be
    multiplied with and V3R can be added to (for example V3R, V3D). Note that
    for the transformation of HKLs, the matrix needs to be transposed. In some
    cases, such as the example above, it does not matter, because the matrix is
    identical to its transposed. In general however, transposing is necessary,
    so there is a dedicated method for that:

        SymmetryOperation sixFold("x-y,x,z");
        V3D hklPrime = sixFold.transformHKL(V3D(1,0,0)); //


    A special case is the multiplication of several symmetry operations, which
    can be used to generate new operations:

        SymmetryOperation inversion("-x,-y,-z");
        SymmetryOperation identity = inversion * inversion;

    In context of Group, the vector of SymmetryOperation is wrapped onto the
    interval (0, 1] using the getUnitCellIntervalOperation-function.

    Constructing a SymmetryOperation object from a string is heavy, because the
    string has to be parsed every time. It's preferable to use
    the available factory:

        SymmetryOperation inversion =
            SymmetryOperationFactory::Instance().createSymOp("-x,-y,-z");

    It stores a prototype of the created operation and copy constructs a new
    instance on subsequent calls with the same string.

    SymmetryOperation-objects are for example used in PointGroup.

    References:
        [1] International Tables for Crystallography, Volume A, Fourth edition,
            pp 797-798.


      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 25/07/2014

    Copyright © 2014 PSI-MSS

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class MANTID_GEOMETRY_DLL SymmetryOperation {
public:
  SymmetryOperation();
  SymmetryOperation(const std::string &identifier);
  SymmetryOperation(const Kernel::IntMatrix &matrix, const V3R &vector);
  SymmetryOperation(const Kernel::DblMatrix &matrix, const V3R &vector);

  const Kernel::IntMatrix &matrix() const;
  const V3R &vector() const;
  const V3R &reducedVector() const;

  size_t order() const;
  std::string identifier() const;

  bool isIdentity() const;
  bool hasTranslation() const;

  /// Returns the transformed vector.
  template <typename T> T operator*(const T &operand) const {
    return m_matrixVectorPair * operand;
  }

  Kernel::V3D transformHKL(const Kernel::V3D &hkl) const;

  SymmetryOperation operator*(const SymmetryOperation &operand) const;
  SymmetryOperation inverse() const;

  SymmetryOperation operator^(size_t exponent) const;

  bool operator!=(const SymmetryOperation &other) const;
  bool operator==(const SymmetryOperation &other) const;
  bool operator<(const SymmetryOperation &other) const;

protected:
  void init(const MatrixVectorPair<int, V3R> &matrixVectorPair);

  size_t getOrderFromMatrix(const Kernel::IntMatrix &matrix) const;
  V3R getReducedVector(const Kernel::IntMatrix &matrix,
                       const V3R &vector) const;

  size_t m_order;
  Kernel::IntMatrix m_transposedInverseMatrix;
  V3R m_reducedVector;
  std::string m_identifier;

  MatrixVectorPair<int, V3R> m_matrixVectorPair;
};

MANTID_GEOMETRY_DLL std::ostream &
operator<<(std::ostream &stream, const SymmetryOperation &operation);
MANTID_GEOMETRY_DLL std::istream &operator>>(std::istream &stream,
                                             SymmetryOperation &operation);

MANTID_GEOMETRY_DLL V3R getWrappedVector(const V3R &vector);
MANTID_GEOMETRY_DLL Kernel::V3D getWrappedVector(const Kernel::V3D &vector);

MANTID_GEOMETRY_DLL SymmetryOperation
getUnitCellIntervalOperation(const SymmetryOperation &symOp);

template <typename T, typename U>
Kernel::Matrix<T> convertMatrix(const Kernel::Matrix<U> &matrix) {
  Kernel::Matrix<T> converted(matrix.numRows(), matrix.numCols());

  for (size_t i = 0; i < converted.numRows(); ++i) {
    for (size_t j = 0; j < converted.numCols(); ++j) {
      converted[i][j] = static_cast<int>(matrix[i][j]);
    }
  }

  return converted;
}

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SYMMETRYOPERATION_H_ */
