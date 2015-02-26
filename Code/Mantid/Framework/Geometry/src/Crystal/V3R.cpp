#include "MantidGeometry/Crystal/V3R.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace Geometry {

/// Default constructor, all elements 0
V3R::V3R() : m_x(0), m_y(0), m_z(0) {}

/// Constructor from three RationalNumbers, which may also be integers
V3R::V3R(const RationalNumber &x, const RationalNumber &y,
         const RationalNumber &z)
    : m_x(x), m_y(y), m_z(z) {}

/// Constructor from an appropriately sized integer vector
V3R::V3R(const std::vector<int> &vector) {
  if (vector.size() != 3) {
    throw Kernel::Exception::MisMatch<size_t>(
        vector.size(), 3, "V3R(const std::vector<int> &vector)");
  }

  m_x = vector[0];
  m_y = vector[1];
  m_z = vector[2];
}

/// Copy constructor
V3R::V3R(const V3R &other) : m_x(other.m_x), m_y(other.m_y), m_z(other.m_z) {}

/// Assigment operator
V3R &V3R::operator=(const V3R &other) {
  m_x = other.m_x;
  m_y = other.m_y;
  m_z = other.m_z;

  return *this;
}

/// Destructor
V3R::~V3R() {}

/// Returns the x-component of the vector
const RationalNumber &V3R::x() const { return m_x; }

/// Assigns a new value to the x-component
void V3R::setX(const RationalNumber &newX) { m_x = newX; }

/// Returns the y-component of the vector
const RationalNumber &V3R::y() const { return m_y; }

/// Assigns a new value to the y-component
void V3R::setY(const RationalNumber &newY) { m_y = newY; }

/// Returns the z-component of the vector
const RationalNumber &V3R::z() const { return m_z; }

/// Assigns a new value to the z-component
void V3R::setZ(const RationalNumber &newZ) { m_z = newZ; }

/// Array-style non-const access to the components. Throws
/// Kernel::Exception::IndexError if index is out of range.
RationalNumber &V3R::operator[](size_t index) {
  switch (index) {
  case 0:
    return m_x;
  case 1:
    return m_y;
  case 2:
    return m_z;
  default:
    throw Kernel::Exception::IndexError(index, 2,
                                        "V3R::operator [] index out of range.");
  }
}

/// Array-style const access to the components. Throws
/// Kernel::Exception::IndexError if index is out of range.
const RationalNumber &V3R::operator[](size_t index) const {
  switch (index) {
  case 0:
    return m_x;
  case 1:
    return m_y;
  case 2:
    return m_z;
  default:
    throw Kernel::Exception::IndexError(index, 2,
                                        "V3R::operator [] index out of range.");
  }
}

// Operations with other vectors
/// Performs the operation v1 + v2, which sums the vectors component-wise.
V3R V3R::operator+(const V3R &other) const {
  V3R result(*this);
  return result += other;
}

/// Performs the operation v1 += v2 in place, which adds the components of v2 to
/// the components of v1.
V3R &V3R::operator+=(const V3R &other) {
  m_x += other.m_x;
  m_y += other.m_y;
  m_z += other.m_z;

  return *this;
}

/// Negates all components of the vector
V3R V3R::operator-() const { return V3R(-m_x, -m_y, -m_z); }

/// Performs the operation v1 - v2, which subtracts the vectors component-wise.
V3R V3R::operator-(const V3R &other) const {
  V3R result(*this);
  return result -= other;
}

/// Performs the operation v1 -= v2 in place, which subtracts the components of
/// v2 from the components of v1.
V3R &V3R::operator-=(const V3R &other) {
  m_x -= other.m_x;
  m_y -= other.m_y;
  m_z -= other.m_z;

  return *this;
}

// Operations with int
/// Performs the operation v' = v1 + i, which adds the integer i to each
/// component of v1.
V3R V3R::operator+(int other) const {
  V3R result(*this);
  return result += other;
}

/// Performs the operation v1 += i in place, which adds the integer i to each
/// component of v1.
V3R &V3R::operator+=(int other) {
  m_x += other;
  m_y += other;
  m_z += other;

  return *this;
}

/// Performs the operation v' = v1 - i, which subtracts the integer i from each
/// component of v1.
V3R V3R::operator-(int other) const {
  V3R result(*this);
  return result -= other;
}

/// Performs the operation v1 -= i in place, which subtracts the integer i from
/// each component of v1.
V3R &V3R::operator-=(int other) {
  m_x -= other;
  m_y -= other;
  m_z -= other;

  return *this;
}

/// Performs the operation v' = v1 * i, which multiplies each component of v1
/// with the integer i.
V3R V3R::operator*(int other) const {
  V3R result(*this);
  return result *= other;
}

/// Performs the operation v1 *= i in place, which multiplies each component of
/// v1 with the integer i.
V3R &V3R::operator*=(int other) {
  m_x *= other;
  m_y *= other;
  m_z *= other;

  return *this;
}

/// Performs the operation v' = v1 / i, which divides each component of v1 by
/// the integer i.
V3R V3R::operator/(int other) const {
  V3R result(*this);
  return result /= other;
}

/// Performs the operation v1 /= i in place, which divides each component of v1
/// by the integer i.
V3R &V3R::operator/=(int other) {
  m_x /= other;
  m_y /= other;
  m_z /= other;

  return *this;
}

// Operations with rational numbers
/// Performs the operation v' = v1 + r, which adds the RationalNumber r to each
/// component of v1.
V3R V3R::operator+(const RationalNumber &other) const {
  V3R result(*this);
  return result += other;
}

/// Performs the operation v1 += r in place, which adds the RationalNumber r to
/// each component of v1.
V3R &V3R::operator+=(const RationalNumber &other) {
  m_x += other;
  m_y += other;
  m_z += other;

  return *this;
}

/// Performs the operation v' = v1 - r, which subtracts the RationalNumber r
/// from each component of v1.
V3R V3R::operator-(const RationalNumber &other) const {
  V3R result(*this);
  return result -= other;
}

/// Performs the operation v1 -= r, which subtracts the RationalNumber r from
/// each component of v1.
V3R &V3R::operator-=(const RationalNumber &other) {
  m_x -= other;
  m_y -= other;
  m_z -= other;

  return *this;
}

/// Performs the operation v' = v1 * r, which multiplies each component of v1
/// with the RationalNumber r.
V3R V3R::operator*(const RationalNumber &other) const {
  V3R result(*this);
  return result *= other;
}

/// Performs the operation v1 *= r in place, which multiplies each component of
/// v1 with the RationalNumber r.
V3R &V3R::operator*=(const RationalNumber &other) {
  m_x *= other;
  m_y *= other;
  m_z *= other;

  return *this;
}

/// Performs the operation v' = v1 / r, which divides each component of v1 by
/// the RationalNumber r.
V3R V3R::operator/(const RationalNumber &other) const {
  V3R result(*this);
  return result /= other;
}

/// Performs the operation v1 /= r in place, which divides each component of v1
/// by the RationalNumber r.
V3R &V3R::operator/=(const RationalNumber &other) {
  m_x /= other;
  m_y /= other;
  m_z /= other;

  return *this;
}

/// Returns an instance of Kernel::V3D with floating point approximations of the
/// components.
V3R::operator Kernel::V3D() const {
  return Kernel::V3D(boost::rational_cast<double>(m_x),
                     boost::rational_cast<double>(m_y),
                     boost::rational_cast<double>(m_z));
}

/// Returns the result of the operation d3' = r3 + d3, which is again a
/// Kernel::V3D.
Kernel::V3D V3R::operator+(const Kernel::V3D &other) const {
  return other + static_cast<Kernel::V3D>(*this);
}

/// Returns the result of the operation d3' = r3 - d3, which is again a
/// Kernel::V3D.
Kernel::V3D V3R::operator-(const Kernel::V3D &other) const {
  return static_cast<Kernel::V3D>(*this) - other;
}

/// Returns true if all components of the compared vectors are equal, false
/// otherwise.
bool V3R::operator==(const V3R &other) const {
  return m_x == other.m_x && m_y == other.m_y && m_z == other.m_z;
}

/// Returns true if the compared vectors are not equal.
bool V3R::operator!=(const V3R &other) const {
  return !(this->operator==(other));
}

/// Compares x of both vectors first, if those are equal the function compares y
/// and finally z.
bool V3R::operator<(const V3R &other) const {
  if (m_x != other.m_x) {
    return m_x < other.m_x;
  }

  if (m_y != other.m_y) {
    return m_y < other.m_y;
  }

  return m_z < other.m_z;
}

/// Returns true if all components are equal to the integer used for comparison.
/// Useful for checking against 0.
bool V3R::operator==(int other) const {
  return m_x == other && m_y == other && m_z == other;
}

/// Returns true if any component is different from the integer.
bool V3R::operator!=(int other) const { return !(this->operator==(other)); }

/// Returns a V3R with absolute components.
V3R V3R::getPositiveVector() const {
  return V3R(boost::abs(m_x), boost::abs(m_y), boost::abs(m_z));
}

/// Returns an std::vector<double> with approximations of the components.
V3R::operator std::vector<double>() const {
  std::vector<double> vector;
  vector.push_back(boost::rational_cast<double>(m_x));
  vector.push_back(boost::rational_cast<double>(m_y));
  vector.push_back(boost::rational_cast<double>(m_z));

  return vector;
}

/// Performs a matrix multiplication v' = M * v, throws
/// Kernel::Exception::MisMatch<size_t> if M does not have exactly 3 columns.
V3R operator*(const Kernel::IntMatrix &lhs, const V3R &rhs) {
  size_t rows = lhs.numRows();
  size_t cols = lhs.numCols();

  if (cols != 3) {
    throw Kernel::Exception::MisMatch<size_t>(cols, 3,
                                              "operator*(IntMatrix, V3R)");
  }

  V3R result;
  for (size_t r = 0; r < rows; ++r) {
    for (size_t c = 0; c < cols; ++c) {
      result[r] += lhs[r][c] * rhs[c];
    }
  }

  return result;
}

} // namespace Geometry
} // namespace Mantid
