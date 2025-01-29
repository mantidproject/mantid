// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"

#include "Eigen/Core"
#include "Eigen/Dense"
#include "MantidCurveFitting/EigenVectorView.h"

#include <ostream>
#include <vector>

namespace Mantid {
namespace CurveFitting {
/**
A wrapper around Eigen::Vector.

@author Roman Tolchenov, Tessella plc
@date 24/02/2012
*/

class MANTID_CURVEFITTING_DLL EigenVector {
public:
  /// Constructor
  EigenVector();
  /// Constructor
  explicit EigenVector(const size_t n);
  /// Construct from a std vector
  explicit EigenVector(std::vector<double> v);
  /// Copy from a eigen::vector
  explicit EigenVector(const Eigen::VectorXd *v);
  /// Construct from an initialisation list
  EigenVector(std::initializer_list<double> ilist);
  /// Copy constructor.
  EigenVector(const EigenVector &v);
  /// Move constructor.
  EigenVector(EigenVector &&v) noexcept;
  /// Copy assignment operator
  EigenVector &operator=(const EigenVector &v);
  /// Assignment operator
  EigenVector &operator=(const std::vector<double> &v);
  /// Assignment operator - Eigen::VectorXd
  EigenVector &operator=(const Eigen::VectorXd v);

  /// Get the map of the eigen vector
  inline vec_map_type &mutator() { return m_view.vector_mutator(); }
  /// Get the const map of the eigen vector
  const vec_map_type inspector() const { return m_view.vector_inspector(); }
  /// Get a copy of the eigen vector
  vec_map_type copy_view() const { return m_view.vector_copy(); }

  /// Resize the vector
  void resize(const size_t n);
  /// Size of the vector
  size_t size() const;

  /// Set an element
  void set(const size_t i, const double value);
  /// Get an element
  double get(const size_t i) const;
  /// Get a const reference to an element
  const double &operator[](const size_t i) const { return m_data[i]; }
  /// Get a reference to an element
  double &operator[](const size_t i) { return m_data[i]; }
  // Set all elements to zero
  void zero();
  /// Normalise this vector
  void normalize();
  /// Get vector norm (length)
  double norm() const;
  /// Get vector norm squared
  double norm2() const;
  /// Calculate the dot product
  double dot(const EigenVector &v) const;
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
  const std::vector<double> toStdVector() const;
  /// Return a reference to m_data
  std::vector<double> &StdVectorRef();

  /// Add a vector
  EigenVector &operator+=(const EigenVector &v);
  /// Subtract a vector
  EigenVector &operator-=(const EigenVector &v);
  /// Multiply by a vector (per element)
  EigenVector &operator*=(const EigenVector &v);
  /// Multiply by a number
  EigenVector &operator*=(const double d);
  /// Add a number
  EigenVector &operator+=(const double d);

private:
  /// Default element storage
  std::vector<double> m_data;
  /// The map to the Eigen vector
  EigenVector_View m_view;
};

/// The << operator.
MANTID_CURVEFITTING_DLL std::ostream &operator<<(std::ostream &ostr, const EigenVector &v);

} // namespace CurveFitting
} // namespace Mantid
