// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EigenVector.h"

#include <algorithm>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Mantid::CurveFitting {

/// Constructor
EigenVector::EigenVector() : m_data(1), m_view(EigenVector_View(m_data.data(), m_data.size())) {}

/// Constructor
/// @param n :: The length of the vector.
EigenVector::EigenVector(const size_t n) : m_data(n), m_view(EigenVector_View(m_data.data(), n)) {}

/// Construct from a std vector
/// @param v :: A std vector.
EigenVector::EigenVector(std::vector<double> v)
    : m_data(std::move(v)), m_view(EigenVector_View(m_data.data(), m_data.size())) {}

/// Construct from an initialisation list
/// @param ilist :: A list of doubles: {V0, V1, V2, ...}
EigenVector::EigenVector(std::initializer_list<double> ilist) : EigenVector(ilist.size()) {
  for (auto cell = ilist.begin(); cell != ilist.end(); ++cell) {
    auto i = static_cast<size_t>(std::distance(ilist.begin(), cell));
    set(i, *cell);
  }
}

/// Copy constructor.
/// @param v :: The other vector
EigenVector::EigenVector(const EigenVector &v)
    : m_data(v.m_data), m_view(EigenVector_View(m_data.data(), m_data.size())) {}

/// Copy from an Eigen::Vector
/// @param v :: A vector to copy from.
EigenVector::EigenVector(const Eigen::VectorXd *v)
    : m_data(v->size()), m_view(EigenVector_View(m_data.data(), m_data.size())) {
  for (size_t i = 0; i < (size_t)v->size(); ++i) {
    m_data[i] = (*v)(i);
  }
}

EigenVector::EigenVector(EigenVector &&v) noexcept
    : m_data(std::move(v.m_data)), m_view(EigenVector_View(m_data.data(), size())) {}

/// Copy assignment operator
/// @param v :: The other vector
EigenVector &EigenVector::operator=(const EigenVector &v) {
  m_data = v.m_data;
  m_view = EigenVector_View(m_data.data(), m_data.size());
  return *this;
}

/// Assignment operator
EigenVector &EigenVector::operator=(const std::vector<double> &v) {
  if (v.empty()) {
    m_data.resize(1);
  } else {
    m_data = v;
  }
  m_view = EigenVector_View(m_data.data(), m_data.size());
  return *this;
}

/// Assignment operator - Eigen::VectorXd
EigenVector &EigenVector::operator=(const Eigen::VectorXd v) {
  if (v.size() == 0) {
    m_data.resize(1);
  } else {
    m_data.resize(v.size());
    for (size_t i = 0; i < (size_t)v.size(); ++i) {
      m_data[i] = v(i);
    }
  }
  m_view = EigenVector_View(m_data.data(), m_data.size());
  return *this;
}

/// Resize the vector
/// @param n :: The new length
void EigenVector::resize(const size_t n) {
  if (n == 0) {
    // vector minimum size is 1, retained from gsl for consistency.
    m_data.resize(1);
    m_view = EigenVector_View(m_data.data(), m_data.size());
  } else if (n != size()) {
    m_data.resize(n);
    m_view = EigenVector_View(m_data.data(), m_data.size());
  }
}

/// Size of the vector
size_t EigenVector::size() const { return m_data.size(); }

/// set an element
/// @param i :: The element index
/// @param value :: The new value
void EigenVector::set(const size_t i, const double value) {
  if (i < m_data.size())
    m_data[i] = value;
  else {
    std::stringstream errmsg;
    errmsg << "EigenVector index = " << i << " is out of range = " << m_data.size() << " in EigenVector.set()";
    throw std::out_of_range(errmsg.str());
  }
}
/// get an element
/// @param i :: The element index
double EigenVector::get(const size_t i) const {
  if (i < m_data.size())
    return m_data[i];

  std::stringstream errmsg;
  errmsg << "EigenVector index = " << i << " is out of range = " << m_data.size() << " in EigenVector.get()";
  throw std::out_of_range(errmsg.str());
}

// Set all elements to zero
void EigenVector::zero() { m_data.assign(m_data.size(), 0.0); }

/// Add a vector
/// @param v :: The other vector
EigenVector &EigenVector::operator+=(const EigenVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("EigenVectors have different sizes.");
  }
  mutator() = inspector() + v.inspector();
  return *this;
}

/// Subtract a vector
/// @param v :: The other vector
EigenVector &EigenVector::operator-=(const EigenVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("EigenVectors have different sizes.");
  }
  mutator() = inspector() - v.inspector();
  return *this;
}

/// Multiply by a vector (per element)
EigenVector &EigenVector::operator*=(const EigenVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("EigenVectors have different sizes.");
  }
  mutator() = inspector() * v.inspector();
  return *this;
}

/// Multiply by a number
/// @param d :: The number
EigenVector &EigenVector::operator*=(const double d) {
  std::transform(m_data.begin(), m_data.end(), m_data.begin(), [d](double x) { return x * d; });
  return *this;
}

/// Add a number
/// @param d :: The number
EigenVector &EigenVector::operator+=(const double d) {
  std::transform(m_data.begin(), m_data.end(), m_data.begin(), [d](double x) { return x + d; });
  return *this;
}

// Normalise this vector
void EigenVector::normalize() {
  double N = norm();
  if (N == 0.0 || !boost::math::isfinite(N)) {
    throw std::runtime_error("Cannot normalize null vector.");
  }
  *this *= 1.0 / N;
}

/// Get vector norm (length)
double EigenVector::norm() const { return sqrt(norm2()); }

/// Get vector's norm squared
double EigenVector::norm2() const {
  return std::accumulate(m_data.cbegin(), m_data.cend(), 0.,
                         [](double sum, double element) { return sum + element * element; });
}

/// Calculate the dot product
/// @param v :: The other vector.
double EigenVector::dot(const EigenVector &v) const {
  if (size() != v.size()) {
    throw std::runtime_error("Vectors have different sizes in dot product.");
  }
  double res = 0.0;
  res = (inspector()).dot(v.inspector());
  return res;
}

/// Get index of the minimum element
size_t EigenVector::indexOfMinElement() const {
  if (m_data.empty()) {
    throw std::runtime_error("Cannot find min element of empty vector.");
  }
  auto it = std::min_element(m_data.begin(), m_data.end());
  if (it == m_data.end()) {
    // can it ever happen?
    throw std::runtime_error("Cannot find min element of vector.");
  }
  return static_cast<size_t>(std::distance(m_data.begin(), it));
}

/// Get index of the maximum element
size_t EigenVector::indexOfMaxElement() const {
  if (m_data.empty()) {
    throw std::runtime_error("Cannot find ax element of empty vector.");
  }
  auto it = std::max_element(m_data.begin(), m_data.end());
  if (it == m_data.end()) {
    // can it ever happen?
    throw std::runtime_error("Cannot find max element of vector.");
  }
  return static_cast<size_t>(std::distance(m_data.begin(), it));
}

/// Get indices of both the minimum and maximum elements
std::pair<size_t, size_t> EigenVector::indicesOfMinMaxElements() const {
  if (m_data.empty()) {
    throw std::runtime_error("Cannot find min or max element of empty vector.");
  }
  auto pit = std::minmax_element(m_data.begin(), m_data.end());
  if (pit.first == m_data.end() || pit.second == m_data.end()) {
    // can it ever happen?
    throw std::runtime_error("Cannot find min or max element of vector.");
  }
  return std::make_pair(static_cast<size_t>(std::distance(m_data.begin(), pit.first)),
                        static_cast<size_t>(std::distance(m_data.begin(), pit.second)));
}

/// Create an index array that would sort this vector
/// @param ascending :: If true sort in ascending order. Otherwise
///     sort in descending order.
std::vector<size_t> EigenVector::sortIndices(bool ascending) const {
  std::vector<size_t> indices(size());
  for (size_t i = 0; i < size(); ++i) {
    indices[i] = i;
  }
  if (ascending) {
    std::sort(indices.begin(), indices.end(), [this](size_t i, size_t j) { return this->m_data[i] < m_data[j]; });
  } else {
    std::sort(indices.begin(), indices.end(), [this](size_t i, size_t j) { return this->m_data[i] > m_data[j]; });
  }
  return indices;
}

/// Sort this vector in order defined by an index array
/// @param indices :: Indices defining the order of elements in sorted vector.
void EigenVector::sort(const std::vector<size_t> &indices) {
  std::vector<double> data(size());
  for (size_t i = 0; i < size(); ++i) {
    data[i] = m_data[indices[i]];
  }
  std::swap(m_data, data);
  m_view = EigenVector_View(m_data.data(), m_data.size());
}

/// Copy the values to an std vector of doubles
const std::vector<double> &EigenVector::toStdVector() const { return m_data; }

/// return reference to m_data
std::vector<double> &EigenVector::StdVectorRef() { return m_data; }

/// The << operator.
std::ostream &operator<<(std::ostream &ostr, const EigenVector &v) {
  std::ios::fmtflags fflags(ostr.flags());
  ostr << std::scientific << std::setprecision(6);
  for (size_t j = 0; j < v.size(); ++j) {
    ostr << std::setw(13) << v[j] << ' ';
  }
  ostr.flags(fflags);
  return ostr;
}
} // namespace Mantid::CurveFitting
