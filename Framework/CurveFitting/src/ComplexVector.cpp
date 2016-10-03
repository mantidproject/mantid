//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ComplexVector.h"

#include <cmath>
#include <gsl/gsl_blas.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace Mantid {
namespace CurveFitting {

/// Constructor
ComplexVector::ComplexVector() {
  m_vector = gsl_vector_complex_alloc(1);
  zero();
}

/// Destructor
ComplexVector::~ComplexVector() { gsl_vector_complex_free(m_vector); }

/// Constructor
/// @param n :: The length of the vector.
ComplexVector::ComplexVector(const size_t n) {
  m_vector = gsl_vector_complex_alloc(n);
}

/// Copy constructor.
/// @param v :: The other vector
ComplexVector::ComplexVector(const ComplexVector &v) {
  m_vector = gsl_vector_complex_alloc(v.size());
  gsl_vector_complex_memcpy(m_vector, v.gsl());
}

/// Move constructor.
/// @param v :: The other vector
ComplexVector::ComplexVector(ComplexVector &&v) {
  m_vector = v.m_vector;
  v.m_vector = gsl_vector_complex_alloc(1);
}

/// Copy from a gsl vector
/// @param v :: A vector to copy from.
ComplexVector::ComplexVector(const gsl_vector_complex *v) {
  m_vector = gsl_vector_complex_alloc(v->size);
  gsl_vector_complex_memcpy(m_vector, v);
}

/// Move from a gsl vector
/// @param v :: A vector to move.
ComplexVector::ComplexVector(gsl_vector_complex *&&v) {
  m_vector = v;
  v = nullptr;
}

/// Copy assignment operator
/// @param v :: The other vector
ComplexVector &ComplexVector::operator=(const ComplexVector &v) {
  gsl_vector_complex_free(m_vector);
  m_vector = gsl_vector_complex_alloc(v.size());
  gsl_vector_complex_memcpy(m_vector, v.gsl());
  return *this;
}

/// Move assignment operator
/// @param v :: The other vector
ComplexVector &ComplexVector::operator=(ComplexVector &&v) {
  std::swap(m_vector, v.m_vector);
  return *this;
}

/// Get the pointer to the GSL vector
gsl_vector_complex *ComplexVector::gsl() { return m_vector; }

/// Get the pointer to the GSL vector
const gsl_vector_complex *ComplexVector::gsl() const { return m_vector; }

/// Resize the vector
/// @param n :: The new length
void ComplexVector::resize(const size_t n) {
  auto oldVector = m_vector;
  m_vector = gsl_vector_complex_alloc(n);
  size_t m = oldVector->size < n ? oldVector->size : n;
  for (size_t i = 0; i < m; ++i) {
    gsl_vector_complex_set(m_vector, i, gsl_vector_complex_get(oldVector, i));
  }
  for (size_t i = m; i < n; ++i) {
    gsl_vector_complex_set(m_vector, i, gsl_complex{{0, 0}});
  }
  gsl_vector_complex_free(oldVector);
}

/// Size of the vector
size_t ComplexVector::size() const { return m_vector->size; }

/// set an element
/// @param i :: The element index
/// @param value :: The new value
void ComplexVector::set(size_t i, const ComplexType &value) {
  if (i < m_vector->size) {
    gsl_vector_complex_set(m_vector, i,
                           gsl_complex{{value.real(), value.imag()}});

  } else {
    std::stringstream errmsg;
    errmsg << "ComplexVector index = " << i
           << " is out of range = " << m_vector->size
           << " in ComplexVector.set()";
    throw std::out_of_range(errmsg.str());
  }
}
/// get an element
/// @param i :: The element index
ComplexType ComplexVector::get(size_t i) const {
  if (i < m_vector->size) {
    auto value = gsl_vector_complex_get(m_vector, i);
    return ComplexType(GSL_REAL(value), GSL_IMAG(value));
  }

  std::stringstream errmsg;
  errmsg << "ComplexVector index = " << i
         << " is out of range = " << m_vector->size
         << " in ComplexVector.get()";
  throw std::out_of_range(errmsg.str());
}

// Set all elements to zero
void ComplexVector::zero() { gsl_vector_complex_set_zero(m_vector); }

/// Add a vector
/// @param v :: The other vector
ComplexVector &ComplexVector::operator+=(const ComplexVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("ComplexVectors have different sizes.");
  }
  gsl_vector_complex_add(gsl(), v.gsl());
  return *this;
}

/// Subtract a vector
/// @param v :: The other vector
ComplexVector &ComplexVector::operator-=(const ComplexVector &v) {
  if (size() != v.size()) {
    throw std::runtime_error("ComplexVectors have different sizes.");
  }
  gsl_vector_complex_sub(gsl(), v.gsl());
  return *this;
}

/// Multiply by a number
/// @param d :: The number
ComplexVector &ComplexVector::operator*=(const ComplexType d) {
  gsl_vector_complex_scale(gsl(), gsl_complex{{d.real(), d.imag()}});
  return *this;
}

/// Create a new ComplexVector and move all data to it.
/// Destroys this vector.
ComplexVector ComplexVector::move() {
  return ComplexVector(std::move(m_vector));
}

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

} // namespace CurveFitting
} // namespace Mantid
