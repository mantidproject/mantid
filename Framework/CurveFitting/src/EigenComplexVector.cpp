// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EigenComplexVector.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace Mantid::CurveFitting {

/// Constructor
ComplexVector::ComplexVector() : m_vector(Eigen::VectorXcd(1)) { zero(); }

/// Constructor
/// @param n :: The length of the vector.
ComplexVector::ComplexVector(const size_t n) : m_vector(Eigen::VectorXcd(n)) {}

/// Copy constructor.
/// @param v :: The other vector
ComplexVector::ComplexVector(const ComplexVector &v) : m_vector(v.m_vector) {}

/// Move constructor
/// @param v :: The other vector
ComplexVector::ComplexVector(ComplexVector &&v) : m_vector(std::move(v.m_vector)) {}

/// Copy from an Eigen::Vector
/// @param v :: A vector to copy from.
ComplexVector::ComplexVector(const Eigen::VectorXcd v) : m_vector(v) {}

/// Move from an Eigen::Vector
/// @param v :: A vector to move.
ComplexVector::ComplexVector(Eigen::VectorXcd &&v) : m_vector(std::move(v)) {}

/// Copy assignment operator
/// @param v :: The other vector
ComplexVector &ComplexVector::operator=(const ComplexVector &v) {
  m_vector = v.m_vector;
  return *this;
}

/// Copy assignment operator
/// @param v :: The other Eigen::vector
ComplexVector &ComplexVector::operator=(const Eigen::VectorXcd &v) {
  m_vector = v;
  return *this;
}

/// Move assignment operator
/// @param v :: The other vector
ComplexVector &ComplexVector::operator=(ComplexVector &&v) noexcept {
  m_vector = std::move(v.m_vector);
  return *this;
}

/// Get the reference to the Eigen vector
Eigen::VectorXcd &ComplexVector::eigen() { return m_vector; }

/// Get a const copy of the Eigen vector
const Eigen::VectorXcd ComplexVector::eigen() const { return m_vector; }

/// Resize the vector
/// @param n :: The new length
void ComplexVector::resize(const size_t n) {
  auto newVector = Eigen::VectorXcd(n);
  size_t m = size() < n ? size() : n;
  for (size_t i = 0; i < m; ++i) {
    newVector(i) = m_vector(i);
  }
  for (size_t i = m; i < n; ++i) {
    newVector(i) = ComplexType(0, 0);
  }
  m_vector = newVector;
}

/// Size of the vector
size_t ComplexVector::size() const { return m_vector.size(); }

/// set an element
/// @param i :: The element index
/// @param value :: The new value
void ComplexVector::set(size_t i, const ComplexType &value) {
  if (i < size()) {
    m_vector(i) = value;
  } else {
    std::stringstream errmsg;
    errmsg << "ComplexVector index = " << i << " is out of range = " << m_vector.size() << " in ComplexVector.set()";
    throw std::out_of_range(errmsg.str());
  }
}
/// get an element
/// @param i :: The element index
ComplexType ComplexVector::get(size_t i) const {
  if (i < size()) {
    return m_vector(i);
  }
  std::stringstream errmsg;
  errmsg << "ComplexVector index = " << i << " is out of range = " << m_vector.size() << " in ComplexVector.get()";
  throw std::out_of_range(errmsg.str());
}

// Set all elements to zero
void ComplexVector::zero() { m_vector.setZero(); }

/// Add a vector
/// @param v :: The other vector
ComplexVector &ComplexVector::operator+=(const ComplexVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("ComplexVectors have different sizes.");
  }
  m_vector += v.eigen();
  return *this;
}

/// Subtract a vector
/// @param v :: The other vector
ComplexVector &ComplexVector::operator-=(const ComplexVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("ComplexVectors have different sizes.");
  }
  m_vector -= v.eigen();
  return *this;
}

/// Multiply by a number
/// @param d :: The number
ComplexVector &ComplexVector::operator*=(const ComplexType d) {
  m_vector *= d;
  return *this;
}

/// Add a complex number
/// @param d :: The complex number
ComplexVector &ComplexVector::operator+=(const ComplexType &d) {
  m_vector.array() += d;
  return *this;
}

/// Create a new ComplexVector and move all data to it.
/// Destroys this vector.
ComplexVector ComplexVector::move() { return ComplexVector(m_vector); }

/// The << operator.
std::ostream &operator<<(std::ostream &ostr, const ComplexVector &v) {
  std::ios::fmtflags fflags(ostr.flags());
  ostr << std::scientific << std::setprecision(6);
  for (size_t j = 0; j < v.size(); ++j) {
    auto value = v.get(j);
    ostr << std::setw(28) << value.real() << "+" << value.imag() << "j";
  }
  ostr.flags(fflags);
  return ostr;
}

/// Sort this vector in order defined by an index array
/// @param indices :: Indices defining the order of elements in sorted vector.
void ComplexVector::sort(const std::vector<size_t> &indices) {
  std::vector<ComplexType> temp_data(size());

  for (size_t i = 0; i < size(); ++i) {
    temp_data[i] = m_vector(indices[i]);
  }
  resize(indices.size());

  m_vector = complex_vec_map_type(temp_data.data(), temp_data.size(), dynamic_stride(0, 1));
}

} // namespace Mantid::CurveFitting
