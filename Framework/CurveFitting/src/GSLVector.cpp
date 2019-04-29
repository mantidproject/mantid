// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GSLVector.h"

#include <algorithm>
#include <cmath>
#include <gsl/gsl_blas.h>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace Mantid {
namespace CurveFitting {

/// Constructor
GSLVector::GSLVector()
    : m_data(1), m_view(gsl_vector_view_array(m_data.data(), 1)) {}

/// Constructor
/// @param n :: The length of the vector.
GSLVector::GSLVector(const size_t n)
    : m_data(n), m_view(gsl_vector_view_array(m_data.data(), n)) {}

/// Construct from a std vector
/// @param v :: A std vector.
GSLVector::GSLVector(const std::vector<double> &v)
    : m_data(v), m_view(gsl_vector_view_array(m_data.data(), m_data.size())) {}

/// Construct from an initialisation list
/// @param ilist :: A list of doubles: {V0, V1, V2, ...}
GSLVector::GSLVector(std::initializer_list<double> ilist)
    : GSLVector(ilist.size()) {
  for (auto cell = ilist.begin(); cell != ilist.end(); ++cell) {
    auto i = static_cast<size_t>(std::distance(ilist.begin(), cell));
    set(i, *cell);
  }
}

/// Copy constructor.
/// @param v :: The other vector
GSLVector::GSLVector(const GSLVector &v)
    : m_data(v.m_data),
      m_view(gsl_vector_view_array(m_data.data(), m_data.size())) {}

/// Copy from a gsl vector
/// @param v :: A vector to copy from.
GSLVector::GSLVector(const gsl_vector *v)
    : m_data(v->size),
      m_view(gsl_vector_view_array(m_data.data(), m_data.size())) {
  for (size_t i = 0; i < v->size; ++i) {
    m_data[i] = gsl_vector_get(v, i);
  }
}

/// Move constructor.
GSLVector::GSLVector(std::vector<double> &&v)
    : m_data(std::move(v)),
      m_view(gsl_vector_view_array(m_data.data(), m_data.size())) {}

/// Copy assignment operator
/// @param v :: The other vector
GSLVector &GSLVector::operator=(const GSLVector &v) {
  m_data = v.m_data;
  m_view = gsl_vector_view_array(m_data.data(), m_data.size());
  return *this;
}

/// Assignment operator
GSLVector &GSLVector::operator=(const std::vector<double> &v) {
  if (v.empty()) {
    m_data.resize(1, 0);
  } else {
    m_data = v;
  }
  m_view = gsl_vector_view_array(m_data.data(), m_data.size());
  return *this;
}

/// Get the pointer to the GSL vector
gsl_vector *GSLVector::gsl() { return &m_view.vector; }

/// Get the pointer to the GSL vector
const gsl_vector *GSLVector::gsl() const { return &m_view.vector; }

/// Resize the vector
/// @param n :: The new length
void GSLVector::resize(const size_t n) {
  if (n != size()) {
    m_data.resize(n);
    m_view = gsl_vector_view_array(m_data.data(), m_data.size());
  }
}

/// Size of the vector
size_t GSLVector::size() const { return m_data.size(); }

/// set an element
/// @param i :: The element index
/// @param value :: The new value
void GSLVector::set(size_t i, double value) {
  if (i < m_data.size())
    m_data[i] = value;
  else {
    std::stringstream errmsg;
    errmsg << "GSLVector index = " << i
           << " is out of range = " << m_data.size() << " in GSLVector.set()";
    throw std::out_of_range(errmsg.str());
  }
}
/// get an element
/// @param i :: The element index
double GSLVector::get(size_t i) const {
  if (i < m_data.size())
    return m_data[i];

  std::stringstream errmsg;
  errmsg << "GSLVector index = " << i << " is out of range = " << m_data.size()
         << " in GSLVector.get()";
  throw std::out_of_range(errmsg.str());
}

// Set all elements to zero
void GSLVector::zero() { m_data.assign(m_data.size(), 0.0); }

/// Add a vector
/// @param v :: The other vector
GSLVector &GSLVector::operator+=(const GSLVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("GSLVectors have different sizes.");
  }
  gsl_vector_add(gsl(), v.gsl());
  return *this;
}

/// Subtract a vector
/// @param v :: The other vector
GSLVector &GSLVector::operator-=(const GSLVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("GSLVectors have different sizes.");
  }
  gsl_vector_sub(gsl(), v.gsl());
  return *this;
}

/// Multiply by a vector (per element)
GSLVector &GSLVector::operator*=(const GSLVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("GSLVectors have different sizes.");
  }
  gsl_vector_mul(gsl(), v.gsl());
  return *this;
}

/// Multiply by a number
/// @param d :: The number
GSLVector &GSLVector::operator*=(const double d) {
  for (auto &x : m_data) {
    x *= d;
  }
  return *this;
}

/// Add a number
/// @param d :: The number
GSLVector &GSLVector::operator+=(const double d) {
  for (auto &x : m_data) {
    x += d;
  }
  return *this;
}

/// Normalise this vector
void GSLVector::normalize() {
  double N = norm();
  if (N == 0.0) {
    throw std::runtime_error("Cannot normalize null vector.");
  }
  *this *= 1.0 / N;
}

/// Get vector norm (length)
double GSLVector::norm() const { return sqrt(norm2()); }

/// Get vector's norm squared
double GSLVector::norm2() const {
  return std::accumulate(
      m_data.cbegin(), m_data.cend(), 0.,
      [](double sum, double element) { return sum + element * element; });
}

/// Calculate the dot product
/// @param v :: The other vector.
double GSLVector::dot(const GSLVector &v) const {
  if (size() != v.size()) {
    throw std::runtime_error("Vectors have different sizes in dot product.");
  }
  double res = 0.0;
  gsl_blas_ddot(gsl(), v.gsl(), &res);
  return res;
}

/// Get index of the minimum element
size_t GSLVector::indexOfMinElement() const {
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
size_t GSLVector::indexOfMaxElement() const {
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
std::pair<size_t, size_t> GSLVector::indicesOfMinMaxElements() const {
  if (m_data.empty()) {
    throw std::runtime_error("Cannot find min or max element of empty vector.");
  }
  auto pit = std::minmax_element(m_data.begin(), m_data.end());
  if (pit.first == m_data.end() || pit.second == m_data.end()) {
    // can it ever happen?
    throw std::runtime_error("Cannot find min or max element of vector.");
  }
  return std::make_pair(
      static_cast<size_t>(std::distance(m_data.begin(), pit.first)),
      static_cast<size_t>(std::distance(m_data.begin(), pit.second)));
}

/// Create an index array that would sort this vector
/// @param ascending :: If true sort in ascending order. Otherwise
///     sort in descending order.
std::vector<size_t> GSLVector::sortIndices(bool ascending) const {
  std::vector<size_t> indices(size());
  for (size_t i = 0; i < size(); ++i) {
    indices[i] = i;
  }
  if (ascending) {
    std::sort(indices.begin(), indices.end(), [this](size_t i, size_t j) {
      return this->m_data[i] < m_data[j];
    });
  } else {
    std::sort(indices.begin(), indices.end(), [this](size_t i, size_t j) {
      return this->m_data[i] > m_data[j];
    });
  }
  return indices;
}

/// Sort this vector in order defined by an index array
/// @param indices :: Indices defining the order of elements in sorted vector.
void GSLVector::sort(const std::vector<size_t> &indices) {
  std::vector<double> data(size());
  for (size_t i = 0; i < size(); ++i) {
    data[i] = m_data[indices[i]];
  }
  std::swap(m_data, data);
  m_view = gsl_vector_view_array(m_data.data(), m_data.size());
}

/// Create a new GSLVector and move all data to it. Destroys this vector.
GSLVector GSLVector::move() { return GSLVector(std::move(m_data)); }

/// Copy the values to an std vector of doubles
std::vector<double> GSLVector::toStdVector() const { return m_data; }

/// The << operator.
std::ostream &operator<<(std::ostream &ostr, const GSLVector &v) {
  std::ios::fmtflags fflags(ostr.flags());
  ostr << std::scientific << std::setprecision(6);
  for (size_t j = 0; j < v.size(); ++j) {
    ostr << std::setw(13) << v[j] << ' ';
  }
  ostr.flags(fflags);
  return ostr;
}

} // namespace CurveFitting
} // namespace Mantid
