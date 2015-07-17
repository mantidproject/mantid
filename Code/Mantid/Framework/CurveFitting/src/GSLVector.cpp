//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GSLVector.h"

#include <gsl/gsl_blas.h>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <cmath>

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

/// Copy assignment operator
/// @param v :: The other vector
GSLVector &GSLVector::operator=(const GSLVector &v) {
  m_data = v.m_data;
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
void GSLVector::zero() {
  m_data.assign(m_data.size(), 0.0);
}

/// Add a vector
/// @param v :: The other vector
GSLVector &GSLVector::operator+=(const GSLVector &v) {
  if (size() != v.size()){
    throw std::runtime_error("GSLVectors have different sizes.");
  }
  gsl_vector_add(gsl(), v.gsl());
  return *this;
}

/// Subtract a vector
/// @param v :: The other vector
GSLVector &GSLVector::operator-=(const GSLVector &v) {
  if (size() != v.size()){
    throw std::runtime_error("GSLVectors have different sizes.");
  }
  gsl_vector_sub(gsl(), v.gsl());
  return *this;
}

/// Multiply by a number
/// @param d :: The number
GSLVector &GSLVector::operator*=(const double d) {
  gsl_vector_scale(gsl(), d);
  return *this;
}

/// Normalise this vector
void GSLVector::normalize(){
  double N = norm();
  if (N == 0.0)
  {
    throw std::runtime_error("Cannot normalize null vector.");
  }
  *this *= 1.0 / N;
}

/// Get vector norm (length)
double GSLVector::norm() const
{
  return sqrt(norm2());
}

/// Get vector's norm squared
double GSLVector::norm2() const
{
  double res = 0.0;
  for(auto el = m_data.begin(); el != m_data.end(); ++el) {
    res += (*el) * (*el);
  }
  return res;
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

/// The << operator.
std::ostream &operator<<(std::ostream &ostr, const GSLVector &v) {
  ostr << std::scientific << std::setprecision(6);
  for (size_t j = 0; j < v.size(); ++j) {
    ostr << std::setw(13) << v[j] << ' ';
  }
  return ostr;
}

} // namespace CurveFitting
} // namespace Mantid
