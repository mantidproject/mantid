// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
  /// Construct from an initialisation list
  GSLVector(std::initializer_list<double> ilist);
  /// Copy constructor.
  GSLVector(const GSLVector &v);
  /// Move constructor.
  GSLVector(std::vector<double> &&v);
  /// Copy assignment operator
  GSLVector &operator=(const GSLVector &v);
  /// Assignment operator
  GSLVector &operator=(const std::vector<double> &v);

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
  /// Get index of the minimum element
  size_t indexOfMinElement() const;
  /// Get index of the maximum element
  size_t indexOfMaxElement() const;
  /// Get indices of both the minimum and maximum elements
  std::pair<size_t, size_t> indicesOfMinMaxElements() const;
  /// Create an index array that would sort this vector
  std::vector<size_t> sortIndices(bool ascending = true) const;
  /// Sort this vector in order defined by an index array
  void sort(const std::vector<size_t> &indices);
  /// Copy the values to an std vector of doubles
  std::vector<double> toStdVector() const;

  /// Add a vector
  GSLVector &operator+=(const GSLVector &v);
  /// Subtract a vector
  GSLVector &operator-=(const GSLVector &v);
  /// Multiply by a vector (per element)
  GSLVector &operator*=(const GSLVector &v);
  /// Multiply by a number
  GSLVector &operator*=(const double d);
  /// Add a number
  GSLVector &operator+=(const double d);

protected:
  /// Create a new GSLVector and move all data to it. Destroys this vector.
  GSLVector move();

private:
  /// Default element storage
  std::vector<double> m_data;
  /// The pointer to the GSL vector
  gsl_vector_view m_view;
};

/// The << operator.
MANTID_CURVEFITTING_DLL std::ostream &operator<<(std::ostream &ostr, const GSLVector &v);

} // namespace CurveFitting
} // namespace Mantid
