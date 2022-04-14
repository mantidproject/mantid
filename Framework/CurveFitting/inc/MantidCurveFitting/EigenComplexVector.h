// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"

#include <Eigen/Core>
#include <complex>
#include <ostream>
#include <vector>

namespace Mantid {
namespace CurveFitting {

using ComplexType = std::complex<double>;
class ComplexVector;

typedef Eigen::Stride<Eigen::Dynamic, Eigen::Dynamic> dynamic_stride;
typedef Eigen::Map<Eigen::VectorXcd, 0, dynamic_stride> complex_vec_map_type;

/**
A complex-valued vector for linear algebra computations.
*/
class MANTID_CURVEFITTING_DLL ComplexVector {
public:
  /// Constructor
  ComplexVector();
  /// Constructor
  explicit ComplexVector(const size_t n);
  /// Copy from a gsl vector
  explicit ComplexVector(const Eigen::VectorXcd v);
  /// Copy constructor.
  ComplexVector(const ComplexVector &v);
  /// Move constructor.
  ComplexVector(ComplexVector &&v);
  /// Copy assignment operator
  ComplexVector &operator=(const ComplexVector &v);
  /// Copy assignment operator
  ComplexVector &operator=(const Eigen::VectorXcd &v);
  /// Move assignment operator
  ComplexVector &operator=(ComplexVector &&v) noexcept;

  /// Get the pointer to the GSL vector
  Eigen::VectorXcd &eigen();
  /// Get the pointer to the GSL vector
  const Eigen::VectorXcd eigen() const;

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
  const ComplexType operator[](size_t i) const { return eigen()(i); }
  /// Get a "reference" to an element.
  ComplexType &operator[](size_t i) { return eigen()(i); }

  /// Add a vector
  ComplexVector &operator+=(const ComplexVector &v);
  /// Subtract a vector
  ComplexVector &operator-=(const ComplexVector &v);
  /// Multiply by a number
  ComplexVector &operator*=(const ComplexType d);
  /// Add a complex number
  ComplexVector &operator+=(const ComplexType &d);
  /// Sort Vector by indicies provided
  void sort(const std::vector<size_t> &indices);
  /// Get index of the minimum element

protected:
  /// Create a new ComplexVector and move all data to it. Destroys this vector.
  ComplexVector move();

private:
  /// Constructor
  ComplexVector(Eigen::VectorXcd &&gslVector);
  /// The Eigen vector
  Eigen::VectorXcd m_vector;
};

/// The << operator.
MANTID_CURVEFITTING_DLL std::ostream &operator<<(std::ostream &ostr, const ComplexVector &v);

} // namespace CurveFitting
} // namespace Mantid
