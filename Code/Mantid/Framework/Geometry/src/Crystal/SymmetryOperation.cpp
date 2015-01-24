#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/Crystal/SymmetryOperationSymbolParser.h"
#include <boost/make_shared.hpp>

#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace Geometry {

/// Default constructor, results in identity.
SymmetryOperation::SymmetryOperation()
    : m_order(1), m_matrix(Kernel::IntMatrix(3, 3, true)), m_vector(),
      m_identifier() {
  m_identifier = SymmetryOperationSymbolParser::getNormalizedIdentifier(
      m_matrix, m_vector);
}

/**
 * Construct a symmetry operation from a Jones faithful representation
 *
 * This method invokes SymmetryOperationSymbolParser and tries to parse the
 * supplied string. Please not that parsing this string is not very efficient.
 * If you have to create the same operations very often, use
 * SymmetryOperationFactory, which works with the copy constructor - it's orders
 * of magnitude faster.
 *
 * @param identifier :: Jones faithful representation of a symmetry operation
 */
SymmetryOperation::SymmetryOperation(const std::string &identifier) {
  const std::pair<Kernel::IntMatrix, V3R> parsedSymbol =
      SymmetryOperationSymbolParser::parseIdentifier(identifier);
  init(parsedSymbol.first, parsedSymbol.second);
}

/// Constructs a symmetry operation from a matrix component and a vector,
/// derives order and identifier from matrix and vector.
SymmetryOperation::SymmetryOperation(const Kernel::IntMatrix &matrix,
                                     const V3R &vector) {
  init(matrix, vector);
}

/// Copy-constructor
SymmetryOperation::SymmetryOperation(const SymmetryOperation &other)
    : m_order(other.m_order), m_matrix(other.m_matrix),
      m_vector(other.m_vector), m_identifier(other.m_identifier) {}

/// Assignment operator
SymmetryOperation &SymmetryOperation::
operator=(const SymmetryOperation &other) {
  m_order = other.m_order;
  m_matrix = other.m_matrix;
  m_vector = other.m_vector;
  m_identifier = other.m_identifier;

  return *this;
}

/// Initialize from matrix and vector.
void SymmetryOperation::init(const Kernel::IntMatrix &matrix,
                             const V3R &vector) {
  m_matrix = matrix;
  m_vector = getWrappedVector(vector);

  m_order = getOrderFromMatrix(m_matrix);
  m_identifier = SymmetryOperationSymbolParser::getNormalizedIdentifier(
      m_matrix, m_vector);
}

/// Returns a const reference to the internally stored matrix
const Kernel::IntMatrix &SymmetryOperation::matrix() const { return m_matrix; }

/// Returns a const reference to the internall stored vector
const V3R &SymmetryOperation::vector() const { return m_vector; }

/**
 * Returns the order of the symmetry operation
 *
 * @return Order of the symmetry operation
 */
size_t SymmetryOperation::order() const { return m_order; }

/**
 * Returns the string-identifier for this symmetry operation
 *
 * @return Identifier of the symmetry operation
 */
std::string SymmetryOperation::identifier() const { return m_identifier; }

/// Returns true if this is the identity operation.
bool SymmetryOperation::isIdentity() const {
  return !hasTranslation() && m_matrix == Kernel::IntMatrix(3, 3, true);
}

/// Returns true if the operation has a translational component.
bool SymmetryOperation::hasTranslation() const { return m_vector != 0; }

/**
 * Multiplication operator for combining symmetry operations
 *
 * This operator constructs from S1 (this) and S2 (other) a new symmetry
 *operation SymOp' with
 *
 *      SymOp'(M', v')
 *
 * where
 *      M' = M1 * M2
 *
 * and
 *      v' = (M1 * v2) + v1
 *
 * and the components of v' are on the interval (0, 1].
 *
 * @param operand
 * @return
 */
SymmetryOperation SymmetryOperation::
operator*(const SymmetryOperation &operand) const {
  return SymmetryOperation(
      m_matrix * operand.m_matrix,
      getWrappedVector((m_matrix * operand.m_vector) + m_vector));
}

/// Returns the inverse of the symmetry operation.
SymmetryOperation SymmetryOperation::inverse() const {
  Kernel::IntMatrix matrix(m_matrix);
  matrix.Invert();

  return SymmetryOperation(matrix, -(matrix * m_vector));
}

/// Returns the symmetry operation, applied to itself (exponent) times.
SymmetryOperation SymmetryOperation::operator ^(size_t exponent) const
{
    // If the exponent is 1, no calculations are necessary.
    if(exponent == 1) {
        return SymmetryOperation(*this);
    }

    SymmetryOperation op;

    // The same for 0, which means identity in every case.
    if(exponent == 0) {
        return op;
    }

    for(size_t i = 0; i < exponent; ++i) {
        op = (*this) * op;
    }

    return op;
}

/// Returns true if matrix and vector are equal
bool SymmetryOperation::operator==(const SymmetryOperation &other) const {
  return m_matrix == other.m_matrix && m_vector == other.m_vector;
}

/// Returns true if SymmetryOperation is "smaller" than other, determined by
/// using the identifier strings.
bool SymmetryOperation::operator<(const SymmetryOperation &other) const {
  return m_identifier < other.m_identifier;
}

/// Returns true if operatios are not equal
bool SymmetryOperation::operator!=(const SymmetryOperation &other) const {
  return !(this->operator==(other));
}

/// Returns the order of the symmetry operation based on the matrix. From
/// "Introduction to Crystal Growth and Characterization, Benz and Neumann,
/// Wiley, 2014, p. 51."
size_t
SymmetryOperation::getOrderFromMatrix(const Kernel::IntMatrix &matrix) const {
  int trace = matrix.Trace();
  int determinant = matrix.determinant();

  if (determinant == 1) {
    switch (trace) {
    case 3:
      return 1;
    case 2:
      return 6;
    case 1:
      return 4;
    case 0:
      return 3;
    case -1:
      return 2;
    default:
      break;
    }
  } else if (determinant == -1) {
    switch (trace) {
    case -3:
      return 2;
    case -2:
      return 6;
    case -1:
      return 4;
    case 0:
      return 6;
    case 1:
      return 2;
    default:
      break;
    }
  }

  throw std::runtime_error("There is something wrong with supplied matrix.");
}

/**
 * Wraps a V3R to the interval (0, 1]
 *
 * For certain crystallographic calculations it is necessary to constrain
 * fractional coordinates to the unit cell, for example to generate all
 * atomic positions in the cell. In this context, the fractional coordinate
 * -0.45 is equal to "0.55 of the next cell", so it's transformed to 0.55.
 *
 * @param vector :: Input vector with arbitrary numbers.
 * @return Vector with components on the interval (0, 1]
 */
V3R getWrappedVector(const V3R &vector) {
  V3R wrappedVector(vector);
  for (size_t i = 0; i < 3; ++i) {
    if (wrappedVector[i] < 0) {
      wrappedVector[i] +=
          (abs(vector[i].numerator() / vector[i].denominator()) + 1);
    } else if (wrappedVector[i] >= 1) {
      wrappedVector[i] -= (vector[i].numerator() / vector[i].denominator());
    }
  }

  return wrappedVector;
}

/// Returns a V3D with components on the interval (0, 1], as the version for
/// V3R.
Kernel::V3D getWrappedVector(const Kernel::V3D &vector) {
  Kernel::V3D wrappedVector(vector);
  for (size_t i = 0; i < 3; ++i) {
    if (wrappedVector[i] < 0) {
      wrappedVector[i] = fmod(vector[i], 1.0) + 1.0;
    } else if (wrappedVector[i] >= 1) {
      wrappedVector[i] = fmod(vector[i], 1.0);
    }
  }

  return wrappedVector;
}

} // namespace Geometry
} // namespace Mantid
