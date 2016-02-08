#ifndef MANTID_CURVEFITTING_COMPLEXVECTOR_H_
#define MANTID_CURVEFITTING_COMPLEXVECTOR_H_

#include "MantidCurveFitting/DllConfig.h"
#include <gsl/gsl_vector.h>

#include <ostream>
#include <vector>

namespace Mantid {
namespace CurveFitting {

typedef gsl_complex ComplexType;

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
class MANTID_CURVEFITTING_DLL ComplexVector {
public:
  /// Constructor
  ComplexVector();
  /// Destructor
  ~ComplexVector();
  /// Constructor
  explicit ComplexVector(const size_t n);
  /// Copy from a gsl vector
  explicit ComplexVector(const gsl_vector_complex *v);
  /// Copy constructor.
  ComplexVector(const ComplexVector &v);
  /// Copy assignment operator
  ComplexVector &operator=(const ComplexVector &v);

  /// Get the pointer to the GSL vector
  gsl_vector_complex *gsl();
  /// Get the pointer to the GSL vector
  const gsl_vector_complex *gsl() const;

  /// Resize the vector
  void resize(const size_t n);
  /// Size of the vector
  size_t size() const;

  /// Set an element
  void set(size_t i, const ComplexType &value);
  /// Get an element
  ComplexType get(size_t i) const;
  // Set all elements to zero
  void zero();

  /// Add a vector
  ComplexVector &operator+=(const ComplexVector &v);
  /// Subtract a vector
  ComplexVector &operator-=(const ComplexVector &v);
  /// Multiply by a number
  ComplexVector &operator*=(const ComplexType d);

private:
  /// The pointer to the GSL vector
  gsl_vector_complex *m_vector;
};

/// The << operator.
MANTID_CURVEFITTING_DLL std::ostream &operator<<(std::ostream &ostr,
                                                 const ComplexVector &v);

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COMPLEXVECTOR_H_*/
