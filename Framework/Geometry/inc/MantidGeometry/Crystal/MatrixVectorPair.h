// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"

namespace Mantid {
namespace Geometry {

/** MatrixVectorPair

  This class represents matrix/vector pairs (W, w) that are for example used
  to describe symmetry operations. On vector- or point-like objects
  they perform the following operation:

    P' = (W * P) + w

  The operation on other matrix/vector pairs is defined like this:

    (P, p)' = ((W * P), (W * p) + w)

  A very important use is in SymmetryOperation, which encapsulated
  a MatrixVectorPair<int, V3R> object to represent symmetry operations
  that are used to define point- and space groups.

      @author Michael Wedel, ESS
      @date 02/11/2015
*/
template <typename MatrixNumericType, typename VectorType> class MatrixVectorPair {
public:
  /// Default constructor, unit matrix and 0-vector.
  MatrixVectorPair() : m_matrix(3, 3, true), m_vector() {}

  /// Constructor from matrix and vector.
  MatrixVectorPair(const Kernel::Matrix<MatrixNumericType> &matrix, const VectorType &vector)
      : m_matrix(matrix), m_vector(vector) {}

  virtual ~MatrixVectorPair() = default;

  /// Returns a const reference to the internally stored matrix.
  const Kernel::Matrix<MatrixNumericType> &getMatrix() const { return m_matrix; }

  /// Returns a const reference to the stored vector.
  const VectorType &getVector() const { return m_vector; }

  /// Operator to transform a vector or point.
  template <typename T> T operator*(const T &operand) const { return (m_matrix * operand) + m_vector; }

  /// Operator to combine with another MatrixVectorPair.
  MatrixVectorPair<MatrixNumericType, VectorType>
  operator*(const MatrixVectorPair<MatrixNumericType, VectorType> &other) const {
    return MatrixVectorPair<MatrixNumericType, VectorType>(m_matrix * other.m_matrix,
                                                           (m_matrix * other.m_vector) + m_vector);
  }

  /// Returns the inverse MatrixVectorPair.
  MatrixVectorPair<MatrixNumericType, VectorType> getInverse() const {
    Kernel::Matrix<MatrixNumericType> matrix(m_matrix);
    matrix.Invert();

    return MatrixVectorPair<MatrixNumericType, VectorType>(matrix, -(matrix * m_vector));
  }

  /// Comparison operator, compares the matrix & vector stored internally.
  bool operator==(const MatrixVectorPair<MatrixNumericType, VectorType> &other) const {
    return m_matrix == other.m_matrix && m_vector == other.m_vector;
  }

  /// Inequality operator.
  bool operator!=(const MatrixVectorPair<MatrixNumericType, VectorType> &other) const { return !operator==(other); }

private:
  Kernel::Matrix<MatrixNumericType> m_matrix;
  VectorType m_vector;
};

} // namespace Geometry
} // namespace Mantid
