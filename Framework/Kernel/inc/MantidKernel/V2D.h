// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_V2D_H_
#define MANTID_KERNEL_V2D_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/V3D.h"
#include <array>
#include <cmath>
#include <iosfwd>
#include <limits>

namespace Mantid {
namespace Kernel {

/**
Implements a 2-dimensional vector embedded in a 3D space, i.e.
such that the cross product of two 2D vectors is a 3D vector in the
Z direction

@author Martyn Gigg, Tessella plc
*/
class MANTID_KERNEL_DLL V2D {
public:
  /**
   * Default constructor. It puts the vector at the origin
   */
  inline V2D() noexcept : m_pt{{0.0, 0.0}} {}
  /**
   * Constructor taking an x and y value.
   */
  inline V2D(double x, double y) noexcept : m_pt{{x, y}} {}

  /**
   * X position
   * @returns The X position
   */
  inline double X() const { return m_pt[0]; }
  /**
   * Y position
   * @returns The Y position
   */
  inline double Y() const { return m_pt[1]; }

  /**
   * X position (non-const reference)
   * @returns The X position
   */
  inline double &X() { return m_pt[0]; }
  /**
   * Y position (non-const)
   * @returns The Y position
   */
  inline double &Y() { return m_pt[1]; }

  /// Unchecked index access.
  inline const double &operator[](const size_t index) const {
    return m_pt[index];
  }

  ///@name Arithmetic operations
  ///@{
  /// Sum this and the rhs
  inline V2D operator+(const V2D &rhs) const {
    return V2D(X() + rhs.X(), Y() + rhs.Y());
  }
  /// Increment this vector by rhs
  inline V2D &operator+=(const V2D &rhs) {
    X() += rhs.X();
    Y() += rhs.Y();
    return *this;
  }
  /// Subtract rhs
  inline V2D operator-(const V2D &rhs) const {
    return V2D(X() - rhs.X(), Y() - rhs.Y());
  }
  /// Decrement this by rhs
  inline V2D &operator-=(const V2D &rhs) {
    X() -= rhs.X();
    Y() -= rhs.Y();
    return *this;
  }
  /// Scale and return
  inline V2D operator*(const double factor) const {
    return V2D(X() * factor, Y() * factor);
  }
  /// Scale this
  V2D &operator*=(const double factor) {
    X() *= factor;
    Y() *= factor;
    return *this;
  }
  /// Negate
  inline V2D operator-() const noexcept { return V2D{-X(), -Y()}; }

  ///@}

  ///@name Comparison operators
  ///@{
  /**
   * Equality operator including a tolerance
   * @param rhs :: The rhs object
   * @returns True if they are considered equal false otherwise
   */
  inline bool operator==(const V2D &rhs) const {
    return (std::fabs(X() - rhs.X()) < std::numeric_limits<double>::epsilon() &&
            std::fabs(Y() - rhs.Y()) < std::numeric_limits<double>::epsilon());
  }
  /**
   * Inequality operator including a tolerance
   * @param rhs :: The rhs object
   * @returns True if they are not considered equal false otherwise
   */
  inline bool operator!=(const V2D &rhs) const { return !(rhs == *this); }
  ///@}

  /// Make a normalized vector (return norm value)
  double normalize();
  /**
   * Compute the norm
   * @returns The norm of the vector
   */
  inline double norm() const { return std::sqrt(norm2()); }

  /**
   * Compute the square of the norm
   * @returns The square of the norm
   */
  inline double norm2() const { return X() * X() + Y() * Y(); }

  /**
   * Compute the scalar product with another vector
   * @param other :: A second vector
   */
  inline double scalar_prod(const V2D &other) const {
    return X() * other.X() + Y() * other.Y();
  }

  /**
   * Cross product
   */
  inline V3D cross_prod(const V2D &other) const {
    return V3D(0.0, 0.0, X() * other.Y() - Y() * other.X());
  }

  /**
   * Distance (R) between two points defined as vectors
   * @param other :: A second vector
   * @returns The distance between the two points
   */
  inline double distance(const V2D &other) const {
    return V2D(X() - other.X(), Y() - other.Y()).norm();
  }

  // Angle between this and another vector
  double angle(const V2D &other) const;

private:
  // X,Y
  std::array<double, 2> m_pt;
};

// Overload operator <<
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const V2D &);

} // namespace Kernel
} // namespace Mantid

#endif // MANTID_KERNEL_V2D_H_
