#ifndef MANTID_CURVEFITTING_COMPLEXVECTOR_H_
#define MANTID_CURVEFITTING_COMPLEXVECTOR_H_

#include "MantidCurveFitting/DllConfig.h"
#include <gsl/gsl_vector.h>

#include <complex>
#include <ostream>
#include <vector>

namespace Mantid {
namespace CurveFitting {

using ComplexType = std::complex<double>;
class ComplexVector;

/// Struct helping converting complex values
/// between ComplexType and internal type of
/// ComplexVector
struct ComplexVectorValueConverter {
  ComplexVector &m_vector;
  size_t m_index;
  ComplexVectorValueConverter(ComplexVector &vector, size_t i)
      : m_vector(vector), m_index(i) {}
  operator ComplexType() const;
  ComplexVectorValueConverter &operator=(const ComplexType &c);
};

/**
A complex-valued vector for linear algebra computations.

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
  /// Move constructor.
  ComplexVector(ComplexVector &&v);
  /// Copy assignment operator
  ComplexVector &operator=(const ComplexVector &v);
  /// Move assignment operator
  ComplexVector &operator=(ComplexVector &&v);

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
  /// Get a "const reference" to an element.
  const ComplexVectorValueConverter operator[](size_t i) const {
    return ComplexVectorValueConverter(const_cast<ComplexVector &>(*this), i);
  }
  /// Get a "reference" to an element.
  ComplexVectorValueConverter operator[](size_t i) {
    return ComplexVectorValueConverter(*this, i);
  }

  /// Add a vector
  ComplexVector &operator+=(const ComplexVector &v);
  /// Subtract a vector
  ComplexVector &operator-=(const ComplexVector &v);
  /// Multiply by a number
  ComplexVector &operator*=(const ComplexType d);

protected:
  /// Create a new ComplexVector and move all data to it. Destroys this vector.
  ComplexVector move();

private:
  /// Constructor
  ComplexVector(gsl_vector_complex *&&gslVector);
  /// The pointer to the GSL vector
  gsl_vector_complex *m_vector;
};

/// The << operator.
MANTID_CURVEFITTING_DLL std::ostream &operator<<(std::ostream &ostr,
                                                 const ComplexVector &v);
/// Convert an internal complex value (GSL type) to ComplexType.
inline ComplexVectorValueConverter::operator ComplexType() const {
  return m_vector.get(m_index);
}

/// Convert a value of ComplexType to the internal complex value (GSL type).
inline ComplexVectorValueConverter &ComplexVectorValueConverter::
operator=(const ComplexType &c) {
  m_vector.set(m_index, c);
  return *this;
}

/// Equality operator
inline bool operator==(const ComplexType &c,
                       const ComplexVectorValueConverter &conv) {
  return c == static_cast<ComplexType>(conv);
}

/// Equality operator
inline bool operator==(const ComplexVectorValueConverter &conv,
                       const ComplexType &c) {
  return c == static_cast<ComplexType>(conv);
}

/// Multiplication operator
inline ComplexType operator*(const ComplexVectorValueConverter &conv,
                             const ComplexType &c) {
  return static_cast<ComplexType>(conv) * c;
}

/// Multiplication operator
inline ComplexType operator*(const ComplexType &c,
                             const ComplexVectorValueConverter &conv) {
  return c * static_cast<ComplexType>(conv);
}

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COMPLEXVECTOR_H_*/
