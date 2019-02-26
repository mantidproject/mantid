// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/V2D.h"
#include "MantidKernel/Exception.h"

#include <ostream>

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------
// Public member functions
//-----------------------------------------------------------------------------

/**
 * Make a normalized vector (return norm value)
 * @returns The norm of the vector
 */
double V2D::normalize() {
  const double length = norm();
  X() /= length;
  Y() /= length;
  return length;
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
