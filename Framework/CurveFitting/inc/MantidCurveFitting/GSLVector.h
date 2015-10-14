#ifndef MANTID_CURVEFITTING_GSLVECTOR_H_
#define MANTID_CURVEFITTING_GSLVECTOR_H_

#include "MantidCurveFitting/DllConfig.h"
#include <gsl/gsl_vector.h>

#include <ostream>
#include <vector>

namespace Mantid {
namespace CurveFitting {
/**
A wrapper around gsl_vector.

@author Roman Tolchenov, Tessella plc
@date 24/02/2012

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class MANTID_CURVEFITTING_DLL GSLVector {
public:
  /// Constructor
  GSLVector();
  /// Constructor
  explicit GSLVector(const size_t n);
  /// Construct from a std vector
  explicit GSLVector(const std::vector<double> &v);
  /// Copy from a gsl vector
  explicit GSLVector(const gsl_vector *v);
  /// Copy constructor.
  GSLVector(const GSLVector &v);
  /// Copy assignment operator
  GSLVector &operator=(const GSLVector &v);

  /// Get the pointer to the GSL vector
  gsl_vector *gsl();
  /// Get the pointer to the GSL vector
  const gsl_vector *gsl() const;

  /// Resize the vector
  void resize(const size_t n);
  /// Size of the vector
  size_t size() const;

  /// Set an element
  void set(size_t i, double value);
  /// Get an element
  double get(size_t i) const;
  /// Get a const reference to an element
  const double &operator[](size_t i) const { return m_data[i]; }
  /// Get a reference to an element
  double &operator[](size_t i) { return m_data[i]; }
  // Set all elements to zero
  void zero();
  /// Normalise this vector
  void normalize();
  /// Get vector norm (length)
  double norm() const;
  /// Get vector norm squared
  double norm2() const;
  /// Calculate the dot product
  double dot(const GSLVector &v) const;

  /// Add a vector
  GSLVector &operator+=(const GSLVector &v);
  /// Subtract a vector
  GSLVector &operator-=(const GSLVector &v);
  /// Multiply by a number
  GSLVector &operator*=(const double d);

private:
  /// Default element storage
  std::vector<double> m_data;
  /// The pointer to the GSL vector
  gsl_vector_view m_view;
};

/// The << operator.
MANTID_CURVEFITTING_DLL std::ostream &operator<<(std::ostream &ostr,
                                                 const GSLVector &v);

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLVECTOR_H_*/
