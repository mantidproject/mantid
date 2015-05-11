//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/V2D.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Tolerance.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace Kernel {
//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------
/**
* Index access.
* @param index :: Index value
* @returns Value at the given index
* @throws if out of range
*/
const double &V2D::operator[](const size_t index) const {
  switch (index) {
  case 0:
    return m_x;
  case 1:
    return m_y;
  default:
    throw Exception::IndexError(index, 1, "V2D::operator[]");
  }
}

/**
 * Sum this and the rhs
 * @param rhs :: The other V2D
 * @returns The sum of this and the given V2D
 */
V2D V2D::operator+(const V2D &rhs) const {
  return V2D(m_x + rhs.m_x, m_y + rhs.m_y);
}

/// Increment this vector by rhs
V2D &V2D::operator+=(const V2D &rhs) {
  m_x += rhs.m_x;
  m_y += rhs.m_y;
  return *this;
}
/// Subtract this and the rhs
V2D V2D::operator-(const V2D &rhs) const {
  return V2D(m_x - rhs.m_x, m_y - rhs.m_y);
}
/// Decrement this by rhs
V2D &V2D::operator-=(const V2D &rhs) {
  m_x -= rhs.m_x;
  m_y -= rhs.m_y;
  return *this;
}

/**
* Scale and return
* @param factor :: The scale factor
* @returns A new V2D object scaled by the given factor
*/
V2D V2D::operator*(const double factor) const {
  return V2D(m_x * factor, m_y * factor);
}

/**
* Scale this vector
* @param factor :: The scale factor
* @returns A reference to this object that has been scaled by the given factor
*/
V2D &V2D::operator*=(const double factor) {
  m_x *= factor;
  m_y *= factor;
  return *this;
}

/**
 * Equality operator including a tolerance
 * @param rhs :: The rhs object
 * @returns True if they are considered equal false otherwise
 */
bool V2D::operator==(const V2D &rhs) const {
  return (std::fabs(m_x - rhs.m_x) < Tolerance &&
          std::fabs(m_y - rhs.m_y) < Tolerance);
}

/**
 * Inequality operator including a tolerance
 * @param rhs :: The rhs object
 * @returns True if they are not considered equal false otherwise
 */
bool V2D::operator!=(const V2D &rhs) const { return !(rhs == *this); }

/**
 * Make a normalized vector (return norm value)
 * @returns The norm of the vector
 */
double V2D::normalize() {
  const double length = norm();
  m_x /= length;
  m_y /= length;
  return length;
}

/**
 * Compute the norm
 * @returns The norm of the vector
 */
double V2D::norm() const { return std::sqrt(norm2()); }

/**
  * Compute the square of the norm
  * @returns The square of the norm
  */
double V2D::norm2() const { return m_x * m_x + m_y * m_y; }

/**
 * Compute the scalar product with another vector
 * @param other :: A second vector
 */
double V2D::scalar_prod(const V2D &other) const {
  return m_x * other.m_x + m_y * other.m_y;
}

/**
 * Cross product
 */
V3D V2D::cross_prod(const V2D &other) const {
  return V3D(0.0, 0.0, m_x * other.m_y - m_y * other.m_x);
}

/**
 * Distance (R) between two points defined as vectors
 * @param other :: A second vector
 * @returns The distance between the two points
 */
double V2D::distance(const V2D &other) const {
  return V2D(m_x - other.m_x, m_y - other.m_y).norm();
}

/**
 * Angle between this and another vector
 */
double V2D::angle(const V2D &other) const {
  double ratio = this->scalar_prod(other) / (this->norm() * other.norm());

  if (ratio >= 1.0)       // NOTE: Due to rounding errors, if "other"
    return 0.0;           //       is nearly the same as "this" or
  else if (ratio <= -1.0) //       as "-this", ratio can be slightly
    return M_PI;          //       more than 1 in absolute value.
                          //       That causes acos() to return NaN.
  return acos(ratio);
}

//--------------------------------------------------------------------------
// Non-member, non-friend functions
//--------------------------------------------------------------------------
/**
 * Output stream operator
 * @param os :: An output stream
 * @param point :: The V2D to send to the stream
 */
std::ostream &operator<<(std::ostream &os, const V2D &point) {
  os << "[" << point.X() << "," << point.Y() << "]";
  return os;
}

} // namespace Kernel
} // namespace Mantid
